// Copyright M. Boerger (helly25.com)
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

#include "mbo/strings/parse.h"

#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "mbo/status/status_macros.h"

namespace mbo::strings {

absl::StatusOr<char> ParseOctal(char first_char, std::string_view& data) {
  const bool octal_23 = first_char == 'o';
  if (octal_23) {
    if (data.size() < 3 || data[0] != '{') {
      return absl::InvalidArgumentError("ParseString input has bad octal C++23 sequence.");
    }
    data.remove_prefix(1);
    first_char = PopChar(data);
  }
  static constexpr int kOctalDigit = 8;
  int next_chr = first_char - '0';
  if (!data.empty() && data[0] >= '0' && data[0] <= '7') {
    next_chr = next_chr * kOctalDigit + (PopChar(data) - '0');
    if (!data.empty() && data[0] >= '0' && data[0] <= '7') {
      next_chr = next_chr * kOctalDigit + (PopChar(data) - '0');
    }
  }
  if (octal_23) {
    if (data.empty() || data[0] != '}') {
      return absl::InvalidArgumentError("ParseString input has bad octal C++23 sequence.");
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
      return absl::InvalidArgumentError("ParseString input has bad hex C++23 sequence.");
    }
    data.remove_prefix(1);
  }
  int next_chr = 0;
  if (NextHexChar(data, next_chr)) {
    NextHexChar(data, next_chr);
  } else {
    return absl::InvalidArgumentError("ParseString input has bad hex sequence.");
  }
  if (hex_23) {
    if (data.empty() || data[0] != '}') {
      return absl::InvalidArgumentError("ParseString input has bad hex C++23 sequence.");
    }
    data.remove_prefix(1);
  }
  return static_cast<char>(next_chr);
}

enum class Quotes { kNone, kSingle, kDouble };

void HandleQuotes(const ParseOptions& options, char chr, Quotes& quotes, std::string& result) {
  if (options.enable_double_quotes) {
    switch (quotes) {
      case Quotes::kNone: quotes = chr == '"' ? Quotes::kDouble : Quotes::kSingle; break;
      case Quotes::kSingle:
        if (chr == '\'') {
          quotes = Quotes::kNone;
        } else {
          result += chr;
          return;
        }
        break;
      case Quotes::kDouble:
        if (chr == '"') {
          quotes = Quotes::kNone;
        } else {
          result += chr;
          return;
        }
        break;
    }
    if (!options.remove_quotes) {
      result += chr;
    }
  }
}

namespace {
bool StopParsing(const ParseOptions& options, std::string_view data) {
  const char chr = data[0];
  return absl::StrContains(options.stop_at_any_of, chr) //
         || absl::StrContains(options.split_at_any_of, chr)
         || (!options.stop_at_str.empty() && data.starts_with(options.stop_at_str));
}
}  // namespace

absl::StatusOr<std::string> ParseString(const ParseOptions& options, std::string_view& data) {
  std::string result;
  Quotes quotes = Quotes::kNone;
  while (!data.empty()) {
    char chr = data[0];  // Cannot use `PopChar`, need the char left in-place
    if (quotes == Quotes::kNone
        && (StopParsing(options, data) || (!options.allow_unquoted && chr != '\'' && chr != '"'))) {
      return result;
    }
    data.remove_prefix(1);
    if (chr == '"' || chr == '\'') {
      HandleQuotes(options, chr, quotes, result);
      continue;
    } else if (chr != '\\') {
      result += chr;
      continue;
    } else if (data.empty()) {
      return absl::InvalidArgumentError("ParseString input ends in '\\'.");
    }
    chr = PopChar(data);
    // Direct escapes, just the next char.
    if (absl::StrContains(options.custom_escapes, chr)) {
      result += chr;
      continue;
    }
    switch (chr) {
      // all other escape sequences follow
      // https://en.cppreference.com/w/cpp/language/escape
      // "Simple"
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
      // "Numeric", octal
      case '0':    // octal first digit
      case '1':    // octal first digit
      case '2':    // octal first digit
      case '3':    // octal first digit
      case '4':    // octal first digit
      case '5':    // octal first digit
      case '6':    // octal first digit
      case '7':    // octal first digit
      case 'o': {  // octal \o{n...}, C++23
        MBO_ASSIGN_OR_RETURN(const char next_chr, ParseOctal(chr, data));
        result += static_cast<char>(next_chr);
        continue;
      }
      // "Numeric", hex
      case 'x': {  // \x...: hex, (\x{n...}: C++23
        MBO_ASSIGN_OR_RETURN(const char next_chr, ParseHex(data));
        result += static_cast<char>(next_chr);
        continue;
      }
      // UNSUPPORTED FOLLOW:
      case 'u':  // \u{nn..}: unicode 4 hex C++23
      case 'U':  // Unicode 8-hex
        return absl::UnimplementedError("ParseString input has not yet supported unicode escape sequence.");
      case 'N':  // \N{Name}: Named unicode char
        return absl::UnimplementedError("ParseString input has not yet supported named unicode char escape sequence.");
      default: return absl::InvalidArgumentError("ParseString input has unsupported escape sequence.");
    }  // switch for escape
  }
  if (quotes == Quotes::kNone) {
    return result;
  }
  return absl::InvalidArgumentError(absl::StrFormat(
      "ParseString input has unterminated %s quotes (%s).",  //
      quotes == Quotes::kSingle ? "single" : "double",       //
      quotes == Quotes::kSingle ? "'" : "\""));
}

absl::StatusOr<std::vector<std::string>> ParseStringList(const ParseOptions& options, std::string_view& data) {
  std::vector<std::string> result;
  if (data.empty()) {
    return result;
  }
  ParseOptions str_options = options;
  if (str_options.split_at_any_of.empty()) {
    str_options.split_at_any_of = ",";
  }
  if (absl::StrContains(options.stop_at_any_of, data[0])) {
    return result;
  }
  if (data.size() == 1 && absl::StrContains(str_options.split_at_any_of, data[0])) {
    result.emplace_back("");
    result.emplace_back("");
    data.remove_prefix(1);
    return result;
  }
  while (true) {
    MBO_ASSIGN_OR_RETURN(std::string curr, ParseString(str_options, data));
    result.emplace_back(std::move(curr));
    if (data.empty()) {
      return result;
    }
    if (absl::StrContains(options.stop_at_any_of, data[0])) {
      return result;
    }
    if (data.size() == 1 && absl::StrContains(str_options.split_at_any_of, data[0])) {
      result.emplace_back("");
      data.remove_prefix(1);
      return result;
    }
    data.remove_prefix(1);
  }
  return result;
}

}  // namespace mbo::strings
