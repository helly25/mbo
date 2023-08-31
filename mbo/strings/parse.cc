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

#include "mbo/strings/parse.h"

#include <string>
#include <string_view>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "mbo/status/status_macros.h"

namespace mbo::strings {
namespace {

inline char PopChar(std::string_view& data) {
  ABSL_CHECK(!data.empty());
  const char result = data[0];
  data.remove_prefix(1);
  return result;
}

}  // namespace

absl::StatusOr<char> ParseOctal(char first_char, std::string_view& data) {
  const bool octal_23 = first_char == 'o';
  if (octal_23) {
    if (data.size() < 3 || data[0] != '{') {
      return absl::InvalidArgumentError("StringList has bad octal C++23 sequence.");
    }
    data.remove_prefix(1);
    first_char = PopChar(data);
  }
  static constexpr int kOctalDigit = 8;
  int next_chr = first_char - '0';
  if (!data.empty() && data[0] >= '0' && data[0] <= '7') {
    next_chr = next_chr * kOctalDigit + (PopChar(data) - '0');
    if (!data.empty() && data[0] >= '0' && data[0] <= '7') {
      next_chr = next_chr * kOctalDigit + (PopChar(data)  - '0');
    }
  }
  if (octal_23) {
    if (data.empty() || data[0] != '}') {
      return absl::InvalidArgumentError("StringList has bad octal C++23 sequence.");
    }
    data.remove_prefix(1);
  }
  return static_cast<char>(next_chr);
}

bool NextHexChar(std::string_view& data, int& hex) {
  if (data.empty()) {
    return false;
  }
  static constexpr int kHexDigit = 16;
  if (data[0] >= '0' && data[0] <= '9') {
    hex = hex * kHexDigit + (PopChar(data) - '0');
    return true;
  }
  if (data[0] >= 'a' && data[0] <= 'f') {
    hex = hex * kHexDigit + (PopChar(data) - 'a');
    return true;
  }
  if (data[0] >= 'A' && data[0] <= 'F') {
    hex = hex * kHexDigit + (PopChar(data) - 'A');
    return true;
  }
  return false;
}

absl::StatusOr<char> ParseHex(std::string_view& data) {
  const bool hex_23 = data[0] == '{';
  if (hex_23) {
    if (data.size() < 3) {
      return absl::InvalidArgumentError("StringList has bad hex C++23 sequence.");
    }
    data.remove_prefix(1);
  }
  int next_chr = 0;
  if (NextHexChar(data, next_chr)) {
    NextHexChar(data, next_chr);
  } else {
      return absl::InvalidArgumentError("StringList has bad hex sequence.");
  }
  if (hex_23) {
    if (data.empty() || data[0] != '}') {
      return absl::InvalidArgumentError("StringList has bad hex C++23 sequence.");
    }
    data.remove_prefix(1);
  }
  return static_cast<char>(next_chr);
}

absl::StatusOr<std::string> ParseString(const ParseOptions& options, std::string_view& data) {
  std::string result;
  bool quote = false;
  while (!data.empty()) {
    char chr = data[0];  // Cannot use `PopChar`, need the char left in-place
    if (!quote && absl::StrContains(options.stop_at_any_of, chr)) {
      return result;
    }
    data.remove_prefix(1);
    switch (chr) {
      default: break;  // normal character, just add.
      case '"':
        if (options.enable_double_quotes) {
          quote = !quote;
          continue;
        }
        break;
      case '\\': {
        if (data.empty()) {
          return absl::InvalidArgumentError("StringList ends in '\\'.");
        }
        chr = PopChar(data);
        switch (chr) {
          // Direct escapes, just the next char.
          case '{':  // CUSTOM
          case '}':  // CUSTOM
          case ',':  // CUSTOM
          // all other escape sequences follow
          // https://en.cppreference.com/w/cpp/language/escape
          case '\'':
          case '"':
          case '?':
          case '\\': result += chr; continue;
          case 'a': result += '\a'; continue;
          case 'b': result += '\b'; continue;
          case 'f': result += '\f'; continue;
          case 'n': result += '\n'; continue;
          case 'r': result += '\r'; continue;
          case 't': result += '\t'; continue;
          case 'v': result += '\v'; continue;
          // octal
          case '0':  // octal first digit
          case '1':  // octal first digit
          case '2':  // octal first digit
          case '3':  // octal first digit
          case '4':  // octal first digit
          case '5':  // octal first digit
          case '6':  // octal first digit
          case '7':  // octal first digit
          case 'o': { // octal \o{n...}, C++23
            MBO_STATUS_ASSIGN_OR_RETURN(const char next_chr, ParseOctal(chr, data));
            result += static_cast<char>(next_chr);
            continue;
          }
          case 'x': { // \x...: hex, (\x{n...}: C++23
            MBO_STATUS_ASSIGN_OR_RETURN(const char next_chr, ParseHex(data));
            result += static_cast<char>(next_chr);
            continue;
          }
          // UNSUPPORTED FOLLOW:
          case 'u':  // \u{nn..}: unicode 4 hex C++23
          case 'U':  // Unicode 8-hex
            return absl::UnimplementedError("StringList has not yet supported unicode escape sequence.");
          case 'N':  // \N{Name}: Named unicode char
            return absl::UnimplementedError("StringList has not yet supported named unicode char escape sequence.");
          default: return absl::InvalidArgumentError("StringList has unsupported escape sequence.");
        }  // switch for escape
      }    // case escape
    }      // switch char
    result += chr;
  }
  return result;
}

absl::StatusOr<std::vector<std::string>> ParseStringList(const ParseOptions& options, std::string_view& data) {
  std::vector<std::string> result;
  if (data.empty()) {
    return result;
  }
  ParseOptions str_options = options;
  if (options.stop_at_any_of.empty()) {
    str_options.stop_at_any_of = ",";
  }
  if (data.size() == 1 && absl::StrContains(str_options.stop_at_any_of, data[0])) {
    result.emplace_back("");
    result.emplace_back("");
    data.remove_prefix(1);
    return result;
  }
  while (true) {
    MBO_STATUS_ASSIGN_OR_RETURN(std::string curr, ParseString(str_options, data));
    result.emplace_back(std::move(curr));
    if (data.empty()) {
      return result;
    }
    if (data.size() == 1 && absl::StrContains(str_options.stop_at_any_of, data[0])) {
      result.emplace_back("");
      data.remove_prefix(1);
      return result;
    }
    data.remove_prefix(1);
  }
  return result;
}

}  // namespace mbo::strings
