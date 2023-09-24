// Copyright 2023 M. Boerger (helly25.com)
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

#ifndef MBO_STRINGS_PARSE_H_
#define MBO_STRINGS_PARSE_H_

#include <string>
#include <string_view>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"

namespace mbo::strings {

inline char PopChar(std::string_view& data) {
  ABSL_CHECK(!data.empty());
  const char result = data[0];
  data.remove_prefix(1);
  return result;
}

// ParseString follows: https://en.cppreference.com/w/cpp/language/escape
//
// Parsing has various configurable options as described below.
//
// Escaping:
//   * Support for C++ "simple" escaping.
//   * Support for C++ "numeric" escaping (limited to 8 bit).
//   * Additional "custom" escaping is supported through `custom_escapes` which defaults to: "(){}[]<>,;&".
//   * NO SUPPORT for C++ "Universal character names", so no unicode.
//
struct ParseOptions {
  // Any character in this list will stop `ParseString` and `ParseStringList`.
  std::string_view stop_at_any_of;

  // Similar to `stop_at_any_of`, but an actual single sequence. If this sequence is found, then the parsing stops.
  std::string_view stop_at_str;

  // A character in this list will split `ParseStringList` or stop `ParseString`.
  // It will default to ',' for `ParseStringList`.
  std::string_view split_at_any_of;

  // Enables doble quotes (") which means the input is parsed until the next unescaped double quotes are
  // found. This makes it easy to write stop chars and other special characters without needing to quote them.
  bool enable_double_quotes = true;

  // Same as for double quotes.
  bool enable_single_quotes = true;

  // If quotes are enabled and then they can also be removed.
  bool remove_quotes = true;

  // If disabled, then parsing will stop at any unquoted character (e.g. in <"foo"bad"bar"> 'bad' is unquoted).
  bool allow_unquoted = true;

  // Custom escape support: A backslash followed by one of these characters will be replaced with that character.
  std::string_view custom_escapes = "(){}[]<>,;&";
};

// Parses a single string according to `options`.
//
// If parsing succeeds and a stop character is hit, then that character will not be removed.
// This allows callers to check on what (if any) Character parsing stops. But the caller mustthen
// drop that character.
absl::StatusOr<std::string> ParseString(const ParseOptions& options, std::string_view& data);

// Parse `data` into multiple strings as configured by `options`.
absl::StatusOr<std::vector<std::string>> ParseStringList(const ParseOptions& options, std::string_view& data);

}  // namespace mbo::strings

#endif  // MBO_STRINGS_PARSE_H_
