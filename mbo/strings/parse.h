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

#include "absl/status/statusor.h"

namespace mbo::strings {

// ParseString follows: https://en.cppreference.com/w/cpp/language/escape
//
// The parser stops as soon as it hits a non quoted, non escaped character listed in `stop_at_any_of`.
//
// Escaping: Beyond C++ "simple" and "numeric"" escaping the following "custom" escapes are supported:
// '\,' -> ','
// '\{' -> '{'
// '\}' -> '}'
//
// The parser is currently limited to 8-bit chars, so octal/hex encoding are limited accordingly.
// The parser does not support "Universal character names", so no unicode.
//
struct ParseOptions {
  std::string_view stop_at_any_of;
  bool enable_double_quotes = true;
};

// Parses a single string according to `options`.
//
// If parsing is succeeds and a stop character is hit, then that character will not have been removed. This allows
// callers to check on what (if any) Character parsing stops. But the caller mustthen drop that character.
absl::StatusOr<std::string> ParseString(const ParseOptions& options, std::string_view& data);

absl::StatusOr<std::vector<std::string>> ParseStringList(const ParseOptions& options, std::string_view& data);

}  // namespace mbo::strings

#endif  // MBO_STRINGS_PARSE_H_
