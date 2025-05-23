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

#ifndef MBO_STRINGS_STRIP_H_
#define MBO_STRINGS_STRIP_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "mbo/strings/parse.h"

namespace mbo::strings {

// Like `absl::ConsumePrefix` but works for `std::string`.
[[nodiscard]] inline bool ConsumePrefix(std::string& text, std::string_view prefix) {
  if (!text.starts_with(prefix)) {
    return false;
  }
  text.erase(0, prefix.size());
  return true;
}

[[nodiscard]] inline std::string StripPrefix(std::string&& text, std::string_view prefix) {
  if (text.starts_with(prefix)) {
    text.erase(0, prefix.size());
  }
  return std::move(text);
}

// Like `absl::ConsumeSuffix` but works for `std::string`.
[[nodiscard]] inline bool ConsumeSuffix(std::string& text, std::string_view suffix) {
  if (!text.ends_with(suffix)) {
    return false;
  }
  text.resize(text.size() - suffix.size());
  return true;
}

[[nodiscard]] inline std::string StripSuffix(std::string&& text, std::string_view suffix) {
  if (text.ends_with(suffix)) {
    text.resize(text.size() - suffix.size());
  }
  return std::move(text);
}

// Splits the given `input` into multiple lines and then per line removes anything starting from
// `comment_start` to the end of the line.
//
// In particular this allows to provide comparison data with commoents that can be removed
// prior to executing the comparison, but then shown to the user in a diff output anyway.
//
//  ```c++
// bool Diff(std::string_view, std::string_view { return false; }  // Assume some diff helper.
//
// bool Compare(std::string_view input, std::string_view expected) {
//   const std::string no_comments = StripComments({.input = expected, .comment_starts = "#"});
//   if (input == no_comments) {
//     return true;
//   }
//   return Diff(input, expected);
// }
//
// assert(Compare("foo", "foo # bar"));
// ```
//
// The function can use more complicated parsing, in order to support single or double quotes. This
// can be enabled by providing options for `parse` (see `mbo::strings::ParseString`).
struct StripCommentArgs {
  std::string_view comment_start;
  bool strip_trailing_whitespace = true;
};

std::string StripComments(std::string_view input, const StripCommentArgs& args);

// This is the single line version of `StripComments`.
std::string_view StripLineComments(std::string_view line, const StripCommentArgs& args);

// Similar to `StripComment`, this function can strip out comments. However, this variant supports
// per line parsing to in order to support single or double quotes. This can be enabled by providing
// options for `parse` (see `mbo::strings::ParseString`).
//
// In this version any character listed in `parse.split_at_any_of` or `parse.stop_at_any_of` will
// function as a comment start.
//
// In this form it is potentially a good idea to:
struct StripParsedCommentArgs {
  ParseOptions parse;
  bool strip_trailing_whitespace = true;
};

absl::StatusOr<std::string> StripParsedComments(std::string_view input, const StripParsedCommentArgs& args);

// This is the single line version of `StripParsedComments`.
absl::StatusOr<std::string> StripParsedLineComments(std::string_view line, const StripParsedCommentArgs& args);

}  // namespace mbo::strings

#endif  // MBO_STRINGS_STRIP_H_
