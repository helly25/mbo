// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mbo/file/glob.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "absl/algorithm/container.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "mbo/status/status_macros.h"
#include "mbo/types/extend.h"

#ifdef MBO_ALWAYS_INLINE
# undef MBO_ALWAYS_INLINE
#endif
#ifdef NDEBUG
# define MBO_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
# define MBO_ALWAYS_INLINE
#endif

namespace fs = std::filesystem;

namespace mbo::file {
namespace {

MBO_ALWAYS_INLINE absl::Status ValidateCharacterClass(std::string_view text) {
  if (text.empty()) {
    return absl::InvalidArgumentError("Invalid empty character-class name.");
  }
  if (!absl::c_all_of(text, [](char chr) { return absl::ascii_isalpha(chr); })) {
    return absl::InvalidArgumentError(absl::StrFormat("Invalid character-class name '%s'.", text));
  }
  return absl::OkStatus();
}

MBO_ALWAYS_INLINE absl::Status MaybeCharacterClass(std::string_view& glob_pattern, std::string& re2_pattern) {
  if (!glob_pattern.starts_with("[:")) {
    re2_pattern += glob_pattern.front();
    glob_pattern.remove_prefix(1);
    return absl::OkStatus();
  }
  std::size_t end = glob_pattern.find(":]", 2);
  if (end == std::string_view::npos) {
    return absl::InvalidArgumentError("Unterminated character-class.");
  }
  MBO_RETURN_IF_ERROR(ValidateCharacterClass(glob_pattern.substr(2, end - 2)));
  ++end;
  absl::StrAppend(&re2_pattern, glob_pattern.substr(0, end));
  glob_pattern.remove_prefix(end);
  return absl::OkStatus();
}

MBO_ALWAYS_INLINE void GlobFindRangePrefix(std::string_view& pattern, std::string& re2_pattern) {
  if (pattern.starts_with("[!]")) {
    absl::StrAppend(&re2_pattern, "[^\\]");
    pattern.remove_prefix(3);
  } else if (pattern.starts_with("[!")) {
    absl::StrAppend(&re2_pattern, "[^");
    pattern.remove_prefix(2);
  } else if (pattern.starts_with("[]")) {
    absl::StrAppend(&re2_pattern, "[\\]");
    pattern.remove_prefix(2);
  } else if (pattern.starts_with("[^")) {
    absl::StrAppend(&re2_pattern, "[\\^");
    pattern.remove_prefix(2);
  } else {
    absl::StrAppend(&re2_pattern, "[");
    pattern.remove_prefix(1);
  }
}

MBO_ALWAYS_INLINE absl::Status GlobFindRange(std::string_view& pattern, std::string& re2_pattern) {
  GlobFindRangePrefix(pattern, re2_pattern);
  while (true) {
    if (pattern.empty()) {
      return absl::InvalidArgumentError("Unterminated range expression.");
    }
    char chr = pattern.front();
    switch (chr) {
      default:
        re2_pattern += chr;
        pattern.remove_prefix(1);
        continue;
      case '[': {
        MBO_RETURN_IF_ERROR(MaybeCharacterClass(pattern, re2_pattern));
        continue;
      }
      case ']':
        re2_pattern += chr;
        pattern.remove_prefix(1);
        break;
      case '\\':
        if (pattern.size() < 2) {
          return absl::InvalidArgumentError("Unterminated range expression ending in back-slash.");
        }
        absl::StrAppend(&re2_pattern, pattern.substr(0, 2));
        continue;
    }
    break;
  }
  if (!re2_pattern.ends_with(']')) {
    return absl::InvalidArgumentError("Unterminated range expression.");
  }
  return absl::OkStatus();
}

struct GlobData : mbo::types::Extend<GlobData> {
  std::string pattern;
  std::optional<std::size_t> path_len;
  bool mixed = false;
};

bool AllowApendStar(std::string_view pattern, std::size_t past_last_escape, const Glob2Re2Options& options) {
  // We consider whether the previous chars were stars, but they could have been escaped.
  // The escape char itself could be escaped, so we forward search for escape chars and drop those
  // nad the next charater. What remains is an unescaped sequence.
  pattern.remove_prefix(past_last_escape);
  if (options.allow_star_star) {
    return !pattern.ends_with("**");
  } else {
    return !pattern.ends_with("*");
  }
}

bool AllowAppendSlash(std::string_view pattern, std::size_t past_last_escape) {
  pattern.remove_prefix(past_last_escape);
  return !pattern.ends_with("/") || pattern.ends_with(":/");
}

// Deduplicate '/'s, remove trailing '/' and reduce '*' sequences to at most `options.allow_star_star ? 2 : 1`.
MBO_ALWAYS_INLINE absl::StatusOr<std::string> GlobNormalizeStr(
    std::string_view glob_pattern,
    const Glob2Re2Options& options) {
  std::string result;
  result.reserve(glob_pattern.size());
  std::size_t past_last_escape = 0;
  while (!glob_pattern.empty()) {
    char chr = glob_pattern.front();
    glob_pattern.remove_prefix(1);
    if (chr == '\\') {
      if (glob_pattern.empty()) {
        return absl::InvalidArgumentError("No character left to escape at end of pattern.");
      }
      chr = glob_pattern.front();
      glob_pattern.remove_prefix(1);
      if (chr != '/') {
        result += '\\';
        result += chr;
        past_last_escape = result.size();
        continue;
      }
      // No need to escape the forward slash, but rather prevent duplicates even if escaped.
    }
    switch (chr) {
      case '*':
        if (AllowApendStar(result, past_last_escape, options)) {
          result += chr;
        }
        continue;
      case '[':
        if (!options.allow_ranges || !glob_pattern.starts_with("/]")) {
          result += chr;  // replace '[/]' with '/'.
          continue;
        }
        glob_pattern.remove_prefix(2);
        chr = '/';
        [[fallthrough]];
      case '/':
        if (AllowAppendSlash(result, past_last_escape)) {
          result += chr;
        }
        continue;
      default: {
        result += chr;
        continue;
      }
    }
  }
  if (result.size() > 1 && result.ends_with('/')) {
    result.pop_back();
  }
  return result;
}

MBO_ALWAYS_INLINE absl::StatusOr<GlobData> GlobNormalizeData(
    std::string_view glob_pattern,
    const Glob2Re2Options& options) {
  MBO_MOVE_TO_OR_RETURN(GlobNormalizeStr(glob_pattern, options), std::string pattern);
  char chr = '\0';
  std::size_t slash_0 = std::string_view::npos;
  std::size_t slash_1 = std::string_view::npos;
  bool range_with_slash = false;
  glob_pattern = pattern;  // Operate on the normalized pattern
  GlobData result;
  result.pattern = pattern;
  while (!glob_pattern.empty()) {
    chr = glob_pattern.front();
    switch (chr) {
      case '\\': {
        glob_pattern.remove_prefix(2);
        continue;
      }
      case '[': {
        if (!options.allow_ranges) {
          glob_pattern.remove_prefix(1);
          continue;
        }
        std::string tmp_re2;  //  We do not care about the translated pattern here.
        const bool negative = glob_pattern.size() > 1 && glob_pattern[1] == '!';
        MBO_RETURN_IF_ERROR(GlobFindRange(glob_pattern, tmp_re2));
        range_with_slash |= !negative && absl::StrContains(tmp_re2, '/');
        continue;
      }
      case '/':
        range_with_slash = false;
        slash_0 = slash_1;
        slash_1 = result.pattern.size() - glob_pattern.size();
        glob_pattern.remove_prefix(1);
        continue;
      default: glob_pattern.remove_prefix(1); continue;
    }
  }
  if (range_with_slash) {
    result.path_len = result.pattern.size();
    result.mixed = true;
    return result;
  }
  if (result.pattern.size() > 1 && result.pattern.ends_with('/')) {
    result.pattern.pop_back();
    slash_1 = slash_0;
  }
  if (slash_1 == std::string_view::npos) {
    return result;
  }
  bool star_star = result.pattern.find("**", slash_1) != std::string::npos;
  if (star_star) {
    result.path_len = result.pattern.size();
  } else {
    result.path_len = slash_1;
  }
  return result;
}

MBO_ALWAYS_INLINE void Glob2Re2ExpressionImplStar(std::string& re2_pattern, std::string_view& pattern) {
  // Check for '**' if allowed, remove '**' or '*' otherwise and drop all following '*'.
  // Normalize also changed '**' to '*' if `!options.allow_star_star`.
  if (!pattern.starts_with("**")) {
    pattern.remove_prefix(1);
    absl::StrAppend(&re2_pattern, "[^/]*");
    return;
  }
  pattern.remove_prefix(2);
  if (re2_pattern.ends_with('/') && (pattern.starts_with('/') || pattern.empty())) {
    // We have '/\*\*(/|$)' so the preceeding '/'s are optional.
    re2_pattern.pop_back();  // Drop the last '/'.
    absl::StrAppend(&re2_pattern, "(/.+)?");
    return;
  }
  if (re2_pattern.empty() && pattern.starts_with('/')) {
    // We have '^\*\*(/|$)'
    pattern.remove_prefix(1);  // The next '/'
    if (pattern.starts_with("**/") || pattern == "**") {
      absl::StrAppend(&re2_pattern, "(.+/)+");
    } else {
      absl::StrAppend(&re2_pattern, "(.+/)?");
    }
    return;
  }
  absl::StrAppend(&re2_pattern, ".*");
}

MBO_ALWAYS_INLINE absl::StatusOr<std::string> Glob2Re2ExpressionImpl(
    std::string_view pattern,
    const Glob2Re2Options& options) {
  std::string re2_pattern;
  re2_pattern.reserve(pattern.size() * 2);
  while (!pattern.empty()) {
    char chr = pattern.front();
    switch (chr) {
      default:
        re2_pattern += chr;
        pattern.remove_prefix(1);
        continue;
      case '\\':
        // if (pattern.size() < 2) { already handled
        absl::StrAppend(&re2_pattern, pattern.substr(0, 2));
        pattern.remove_prefix(2);
        continue;
      case '*': {
        Glob2Re2ExpressionImplStar(re2_pattern, pattern);
        continue;
      }
      case '?':
        absl::StrAppend(&re2_pattern, "[^/]");
        pattern.remove_prefix(1);
        continue;
      case '{':
      case '}':
      case '(':
      case ')':
      case '|':
      case '+':
      case '.':
        // Characters that need to be escaped since they are literal in rglob but special in re2.
        re2_pattern += '\\';
        re2_pattern += chr;
        pattern.remove_prefix(1);
        continue;
      case '[':
        if (!options.allow_ranges) {
          re2_pattern += pattern.front();
          pattern.remove_prefix(1);
          continue;
        }
        MBO_RETURN_IF_ERROR(GlobFindRange(pattern, re2_pattern));
        continue;
    }
  }
  return re2_pattern;
}

}  // namespace

namespace file_internal {

absl::StatusOr<GlobParts> GlobSplit(std::string_view pattern, const Glob2Re2Options& options) {
  MBO_MOVE_TO_OR_RETURN(GlobNormalizeData(pattern, options), GlobData data);
  if (data.mixed) {
    return GlobParts{
        .path_pattern{data.pattern},
        .file_pattern{},
        .mixed = true,
    };
  }
  if (data.path_len.has_value()) {
    if (*data.path_len + 1 < data.pattern.size()) {
      return GlobParts{
          .path_pattern{data.pattern.substr(0, *data.path_len)},
          .file_pattern{data.pattern.substr(*data.path_len + 1)},
      };
    }
    return GlobParts{
        .path_pattern{data.pattern},
        .file_pattern{},
    };
  }
  return GlobParts{
      .path_pattern{},
      .file_pattern{data.pattern},
  };
}

absl::StatusOr<std::string> Glob2Re2Expression(std::string_view pattern, const Glob2Re2Options& re2_convert_options) {
  MBO_MOVE_TO_OR_RETURN(GlobNormalizeStr(pattern, re2_convert_options), const std::string normalized_pattern);
  return Glob2Re2ExpressionImpl(normalized_pattern, re2_convert_options);
}

absl::StatusOr<std::unique_ptr<const RE2>> Glob2Re2(
    std::string_view pattern,
    const Glob2Re2Options& re2_convert_options) {
  MBO_MOVE_TO_OR_RETURN(Glob2Re2Expression(pattern, re2_convert_options), std::string re2_pattern);
  return std::make_unique<RE2>(re2_pattern, re2_convert_options.re2_options);
}

absl::Status GlobLoop(const fs::path& root, const GlobOptions& options, const GlobEntryFunc& func) {
  const fs::path normalized_root = (root.empty() || root == ".") ? fs::current_path() : root.lexically_normal();
  std::error_code error_code;
  if (!fs::exists(normalized_root, error_code)) {
    return absl::NotFoundError(
        error_code ? error_code.message() : absl::StrFormat("Path does not exist: '%s'.", normalized_root));
  }
  auto it = fs::recursive_directory_iterator(normalized_root, options.dir_options, error_code);
  if (error_code) {
    return absl::CancelledError(error_code.message());
  }
  for (const fs::directory_entry& entry : it) {
    const GlobEntry glob_entry{
        .rel_path =
            options.use_rel_path ? std::optional<fs::path>{entry.path().lexically_relative(root)} : std::nullopt,
        .entry = entry,
        .depth = it.depth(),
    };
    MBO_ASSIGN_OR_RETURN(const GlobEntryAction action, func(glob_entry));
    switch (action) {
      case GlobEntryAction::kContinue: continue;
      case GlobEntryAction::kStop: break;
      case GlobEntryAction::kDoNotRecurse: {
        it.disable_recursion_pending();
        continue;
      }
    }
  }
  return absl::OkStatus();
}

}  // namespace file_internal

using namespace mbo::file::file_internal;

absl::Status GlobRe2(const fs::path& root, const RE2& regex, const GlobOptions& options, const GlobEntryFunc& func) {
  const auto wrap_func = [&](const GlobEntry& entry) -> absl::StatusOr<GlobEntryAction> {
    if (!RE2::FullMatch(entry.MaybeRelativePath().native(), regex)) {
      return GlobEntryAction::kContinue;  // Path/Filename rejected.
    }
    return func(entry);
  };
  return GlobLoop(root, options, wrap_func);
}

absl::Status Glob(
    const fs::path& root,
    std::string_view pattern,
    const Glob2Re2Options& re2_convert_options,
    const GlobOptions& options,
    const GlobEntryFunc& func) {
  MBO_MOVE_TO_OR_RETURN(Glob2Re2(pattern, re2_convert_options), const std::unique_ptr<const RE2> regex);
  return GlobRe2(root, *regex, options, func);
}

}  // namespace mbo::file
