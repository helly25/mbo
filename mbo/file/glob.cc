// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "mbo/status/status_macros.h"
#include "mbo/types/extend.h"
#include "re2/re2.h"

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

// Find the initial part of the range and return whether the reange is negative.
// Handles special case characters at the beginning.
MBO_ALWAYS_INLINE bool GlobFindRangePrefix(std::string_view& pattern, std::string& re2_pattern) {
  if (pattern.starts_with("[!]")) {
    absl::StrAppend(&re2_pattern, "[^\\]");
    pattern.remove_prefix(3);
    return true;
  } else if (pattern.starts_with("[!")) {
    absl::StrAppend(&re2_pattern, "[^");
    pattern.remove_prefix(2);
    return true;
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
  return false;
}

bool CopytNextChar(std::string_view& pattern, std::string& re2_pattern) {
  if (pattern.empty()) {
    return false;
  }
  if (pattern[0] != '\\') {
    re2_pattern += pattern[0];
    pattern.remove_prefix(1);
    return true;
  }
  if (pattern.size() < 2) {
    return false;
  }
  re2_pattern.append(pattern.substr(0, 2));
  pattern.remove_prefix(2);
  return true;
}

bool RangeRangeContainsSlash(char last, char next) {
  return last < next && last <= '/' && '/' <= next;
}

struct GlobRangeInfo {
  bool negative = false;
  bool has_slash = false;
};

// Find the range pattern end (including ']') and remove it from pattern.
// Convert it into a RE2 pattern and append that to `re2_pattern`.
// On success return whether the pattern is a negative pattern.
MBO_ALWAYS_INLINE absl::StatusOr<GlobRangeInfo> GlobFindRange(std::string_view& pattern, std::string& re2_pattern) {
  GlobRangeInfo result{
      .negative = GlobFindRangePrefix(pattern, re2_pattern),
      .has_slash = false,
  };
  std::size_t pos = 0;
  while (!pattern.empty()) {
    ++pos;
    const char chr = pattern.front();
    switch (chr) {
      default: {
        if (!CopytNextChar(pattern, re2_pattern)) {
          return absl::InvalidArgumentError("Unterminated range expression ending in back-slash.");
        }
        continue;
      }
      case '/':
        re2_pattern += chr;
        pattern.remove_prefix(1);
        result.has_slash = true;
        continue;
      case '-': {
        char last = re2_pattern.back();
        re2_pattern += chr;
        pattern.remove_prefix(1);
        if (pattern.front() == ']') {
          re2_pattern += ']';
          pattern.remove_prefix(1);
          return result;
        }
        if (!CopytNextChar(pattern, re2_pattern)) {
          return absl::InvalidArgumentError("Unterminated range expression ending in back-slash.");
        }
        if (pos > 1) {
          result.has_slash |= RangeRangeContainsSlash(last, re2_pattern.back());
        } else {
          // Case [-X].
          result.has_slash |= re2_pattern.back() == '/';
        }
        continue;
      }
      case '[': {
        MBO_RETURN_IF_ERROR(MaybeCharacterClass(pattern, re2_pattern));
        // TODO(helly25): Does it contain a '/'?
        continue;
      }
      case ']':
        re2_pattern += chr;
        pattern.remove_prefix(1);
        return result;
    }
  }
  return absl::InvalidArgumentError("Unterminated range expression.");
}

struct GlobData : mbo::types::Extend<GlobData> {
  std::string pattern;
  std::optional<std::size_t> path_len;
  std::size_t root_len = 0;
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
  MBO_MOVE_TO_OR_RETURN(GlobNormalizeStr(glob_pattern, options), const std::string pattern);
  char chr = '\0';
  std::size_t slash_0 = std::string_view::npos;
  std::size_t slash_1 = std::string_view::npos;
  bool found_pattern = false;
  bool range_with_slash = false;
  glob_pattern = pattern;  // Operate on the normalized pattern
  GlobData result{
      .pattern = pattern,
  };
  while (!glob_pattern.empty()) {
    chr = glob_pattern.front();
    switch (chr) {
      case '\\': {
        glob_pattern.remove_prefix(2);
        continue;
      }
      case '*':
      case '?':
        glob_pattern.remove_prefix(1);
        found_pattern = true;
        continue;
      case '[': {
        found_pattern = true;
        if (!options.allow_ranges) {
          glob_pattern.remove_prefix(1);
          continue;
        }
        std::string tmp_re2;  //  Here we only care whether a slash is allowed.
        MBO_ASSIGN_OR_RETURN(const GlobRangeInfo info, GlobFindRange(glob_pattern, tmp_re2));
        range_with_slash |= !info.negative && info.has_slash;
        continue;
      }
      case '/':
        range_with_slash = false;
        slash_0 = slash_1;
        slash_1 = result.pattern.size() - glob_pattern.size();
        if (!found_pattern) {
          result.root_len = result.pattern.size() - glob_pattern.size();
        }
        glob_pattern.remove_prefix(1);
        continue;
      default: glob_pattern.remove_prefix(1); continue;
    }
  }
  if (!found_pattern) {
    result.root_len = result.pattern.size();
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
  const bool star_star = result.pattern.find("**", slash_1) != std::string::npos;
  result.path_len = star_star ? result.pattern.size() : slash_1;
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
    const char chr = pattern.front();
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

absl::StatusOr<GlobParts> GlobSplitParts(std::string_view pattern, const Glob2Re2Options& options) {
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
  MBO_MOVE_TO_OR_RETURN(Glob2Re2Expression(pattern, re2_convert_options), const std::string re2_pattern);
  auto re2 = std::make_unique<RE2>(re2_pattern, re2_convert_options.re2_options);
  if (re2->error_code() != RE2::NoError) {
    return absl::InvalidArgumentError(absl::StrFormat("Could not compile re2: '%s': %s.", pattern, re2->error()));
  }
  return re2;
}

absl::StatusOr<RootAndPattern> GlobSplit(std::string_view pattern, const Glob2Re2Options& options) {
  MBO_MOVE_TO_OR_RETURN(GlobNormalizeData(pattern, options), GlobData data);
  std::string_view root(data.pattern);
  root.remove_suffix(root.size() - data.root_len);
  std::string_view patt(data.pattern);
  patt.remove_prefix(data.root_len);
  if (patt.starts_with('/')) {
    patt.remove_prefix(1);
    if (root.empty()) {
      root = "/";
    }
  }
  return RootAndPattern{
      .root = std::string{root},
      .pattern = std::string{patt},
  };
}

}  // namespace file_internal

namespace {

template<typename FileIterator>
int FileIteratorDepth(const FileIterator& file_iterator) {
  if constexpr (std::same_as<FileIterator, std::filesystem::recursive_directory_iterator>) {
    return file_iterator.depth();
  } else {
    return 0;
  }
}

template<typename FileIterator>
void FileIteratorDisableRecursionPending(FileIterator& file_iterator) {
  if constexpr (std::same_as<FileIterator, std::filesystem::recursive_directory_iterator>) {
    file_iterator.disable_recursion_pending();
  }
}

template<typename FileIterator>
absl::Status GlobLoopImpl(const fs::path& root, const GlobOptions& options, const GlobEntryFunc& func) {
  const fs::path normalized_root = (root.empty() || root == ".") ? fs::current_path() : root.lexically_normal();
  std::error_code error_code;
  if (!fs::exists(normalized_root, error_code)) {
    return absl::NotFoundError(
        error_code ? error_code.message() : absl::StrFormat("Path does not exist: '%s'.", normalized_root));
  }
  auto it = FileIterator(normalized_root, options.dir_options, error_code);
  if (error_code) {
    return absl::CancelledError(error_code.message());
  }
  for (const fs::directory_entry& entry : it) {
    const GlobEntry glob_entry{
        .rel_path =
            options.use_rel_path ? std::optional<fs::path>{entry.path().lexically_relative(root)} : std::nullopt,
        .entry = entry,
        .depth = FileIteratorDepth(it),
    };
    MBO_ASSIGN_OR_RETURN(const GlobEntryAction action, func(glob_entry));
    switch (action) {
      case GlobEntryAction::kContinue: continue;
      case GlobEntryAction::kStop: return absl::OkStatus();
      case GlobEntryAction::kDoNotRecurse: {
        FileIteratorDisableRecursionPending(it);
        continue;
      }
    }
  }
  return absl::OkStatus();
}

absl::Status GlobLoop(const fs::path& root, const GlobOptions& options, const GlobEntryFunc& func) {
  if (options.recursive) {
    return GlobLoopImpl<fs::recursive_directory_iterator>(root, options, func);
  } else {
    return GlobLoopImpl<fs::directory_iterator>(root, options, func);
  }
}

}  // namespace

absl::Status GlobRe2(fs::path root, const RE2& regex, const GlobOptions& options, const GlobEntryFunc& func) {
  if (options.use_current_dir) {
    if (root.empty() || root == ".") {
      root = std::filesystem::current_path();
    } else if (root.is_relative()) {
      root = std::filesystem::current_path() / root;
    }
  }
  const auto wrap_func = [&](const GlobEntry& entry) -> absl::StatusOr<GlobEntryAction> {
    if (!regex.pattern().empty()) {
      if (options.use_rel_path) {
        if (!RE2::FullMatch(entry.MaybeRelativePath().native(), regex)) {
          return GlobEntryAction::kContinue;  // Path/Filename rejected.
        }
      } else {
        // The only difference is that we are not actually saving the `rel_path` anywhere.
        const fs::path rel_path = entry.entry.path().lexically_relative(root);
        if (!RE2::FullMatch(rel_path.native(), regex)) {
          return GlobEntryAction::kContinue;  // Path/Filename rejected.
        }
      }
    }
    return func(entry);
  };
  return GlobLoop(root, options, wrap_func);
}

absl::Status GlobRe2(
    const absl::StatusOr<RootAndPattern>& pattern,
    const GlobOptions& options,
    const GlobEntryFunc& func) {
  MBO_RETURN_IF_ERROR(pattern);
  const RE2 re2(pattern->pattern);
  if (re2.error_code() != RE2::NoError) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Could not compile re2: '%s': %s.", pattern->pattern, re2.error()));
  }
  return GlobRe2(pattern->root, re2, options, func);
}

absl::Status Glob(
    const fs::path& root,
    std::string_view pattern,
    const Glob2Re2Options& re2_convert_options,
    const GlobOptions& options,
    const GlobEntryFunc& func) {
  MBO_MOVE_TO_OR_RETURN(file_internal::Glob2Re2(pattern, re2_convert_options), const std::unique_ptr<const RE2> regex);
  return GlobRe2(root, *regex, options, func);
}

absl::Status Glob(
    const absl::StatusOr<RootAndPattern>& pattern,
    const Glob2Re2Options& re2_convert_options,
    const GlobOptions& options,
    const GlobEntryFunc& func) {
  MBO_RETURN_IF_ERROR(pattern);
  return Glob(pattern->root, pattern->pattern, re2_convert_options, options, func);
}

}  // namespace mbo::file
