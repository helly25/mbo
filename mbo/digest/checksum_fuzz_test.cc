// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/digest/checksum.h"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  // Hex lengths used by the CLI's supported fixed-size digest algorithms.
  constexpr std::array<std::size_t, 6> kHexLengths = {32, 40, 56, 64, 96, 128};
  if (size == 0) {
    return 0;
  }

  // libFuzzer exposes bytes as uint8_t; a char view preserves the same object representation.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const std::string_view input(reinterpret_cast<const char*>(data), size);
  for (const std::size_t hex_length : kHexLengths) {
    std::size_t begin = 0;
    while (begin <= input.size()) {
      const std::size_t end = input.find('\n', begin);
      (void)mbo::digest::internal::ParseChecksumLine(input.substr(begin, end - begin), hex_length);
      if (end == std::string_view::npos) {
        break;
      }
      begin = end + 1;
    }
  }
  return 0;
}
