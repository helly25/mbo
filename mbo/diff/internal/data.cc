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

#include "mbo/diff/internal/data.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "mbo/diff/diff_options.h"
#include "mbo/strings/strip.h"
#include "re2/re2.h"

namespace mbo::diff::diff_internal {

namespace {
template<class... Args>
struct Select : Args... {
  explicit Select(Args... args) : Args(args)... {}

  using Args::operator()...;
};
}  // namespace

Data::Data(
    const DiffOptions& options,
    const std::optional<DiffOptions::RegexReplace>& regex_replace,
    std::string_view text)
    : options_(options),
      regex_replace_(regex_replace),
      got_nl_(absl::ConsumeSuffix(&text, "\n")),
      last_line_no_nl_(LastLineIfNoNewLine(text, got_nl_)),
      text_(SplitAndAdaptLastLine(options_, regex_replace_, text, got_nl_, last_line_no_nl_, owned_)) {}

std::string Data::LastLineIfNoNewLine(std::string_view text, bool got_nl) {
  if (got_nl) {
    return "";
  }
  auto pos = text.rfind('\n');
  if (pos == std::string_view::npos) {
    pos = 0;
  } else {
    pos += 1;  // skip '\n'
  }
  return absl::StrCat(text.substr(pos), "\n\\ No newline at end of file");
}

std::vector<Data::LineCache> Data::SplitAndAdaptLastLine(
    const DiffOptions& options,
    const std::optional<DiffOptions::RegexReplace>& regex_replace,
    std::string_view text,
    bool got_nl,
    std::string_view last_line,
    std::deque<std::string>& owned) {
  if (!got_nl && text.empty()) {
    // This means a zero-length input (not just a single new-line).
    // For that case `diff -du` does not show 'No newline at end of file'.
    return {};
  }
  const std::size_t count = std::count_if(text.begin(), text.end(), [](char chr) { return chr == '\n'; });
  std::vector<LineCache> result;
  result.reserve(count + 1);  // N newlines split into N + 1 lines.
  for (std::string_view line : absl::StrSplit(text, '\n')) {
    result.push_back(Process(options, regex_replace, line, owned));
  }
  if (!got_nl) {
    if (!result.empty()) {
      result.pop_back();
    }
    result.push_back({
        .line = last_line,
        .processed = last_line,
        .matches_ignore = false,
    });
  }
  return result;
}

Data::LineCache Data::Process(
    const DiffOptions& options,
    const std::optional<DiffOptions::RegexReplace>& regex_replace,
    std::string_view line,
    std::deque<std::string>& owned) {
  // `processed` stays a view of `line` (or a sub-range of it) through pure
  // trimming/truncation transformations. Only rebuilding transformations
  // materialize a string; the last one gets moved into `owned` (whose
  // elements never move) so the returned view stays valid.
  std::string_view processed = line;
  std::string storage;
  bool has_storage = false;
  const auto own = [&](std::string&& value) {
    storage = std::move(value);
    processed = storage;
    has_storage = true;
  };
  if (options.ignore_all_space) {
    std::string stripped;
    stripped.reserve(line.length());
    for (const char chr : line) {
      if (!absl::ascii_isspace(chr)) {
        stripped.push_back(chr);
      }
    }
    own(std::move(stripped));
  } else if (options.ignore_consecutive_space) {
    std::string collapsed{line};
    absl::RemoveExtraAsciiWhitespace(&collapsed);
    own(std::move(collapsed));
  } else if (options.ignore_trailing_space) {
    processed = absl::StripTrailingAsciiWhitespace(processed);  // A pure suffix trim.
  }
  std::visit(
      Select{
          [&](DiffOptions::NoCommentStripping) {},
          [&](const mbo::strings::StripCommentArgs& args) {
            // A pure truncation (plus trailing whitespace strip).
            processed = mbo::strings::StripLineComments(processed, args);
          },
          [&](const mbo::strings::StripParsedCommentArgs& args) {
            auto line_or = mbo::strings::StripParsedLineComments(processed, args);
            if (line_or.ok()) {
              own(*std::move(line_or));
            }
          }},
      options.strip_comments);
  if (regex_replace.has_value()) {
    std::string replaced{processed};
    if (RE2::Replace(&replaced, *regex_replace->regex, regex_replace->replace)) {
      own(std::move(replaced));
    }
  }
  if (has_storage) {
    processed = owned.emplace_back(std::move(storage));
  }
  return {
      .line = line,
      .processed = processed,
      .matches_ignore = options.ignore_matching_chunks && options.ignore_matching_lines.has_value()
                        && RE2::PartialMatch(processed, *options.ignore_matching_lines),
  };
}

}  // namespace mbo::diff::diff_internal
