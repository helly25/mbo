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

#include "mbo/digest/checksum.h"

#include <cctype>
#include <cstddef>
#include <optional>
#include <string_view>

namespace mbo::digest::internal {

std::optional<ChecksumLine> ParseChecksumLine(std::string_view line, std::size_t hex_length) {
  // The suffix must contain two marker characters and a non-empty filename.
  if (hex_length > line.size() || line.size() - hex_length < 3 || line.at(hex_length) != ' '
      || (line.at(hex_length + 1) != ' ' && line.at(hex_length + 1) != '*')) {
    return std::nullopt;
  }
  const std::string_view hex = line.substr(0, hex_length);
  for (const char chr : hex) {
    if (std::isxdigit(static_cast<unsigned char>(chr)) == 0) {
      return std::nullopt;
    }
  }
  const std::string_view file_name = line.substr(hex_length + 2);
  return ChecksumLine{.hex = hex, .file_name = file_name};
}

}  // namespace mbo::digest::internal
