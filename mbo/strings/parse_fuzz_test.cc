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

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/strings/parse.h"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  if (size == 0) {
    return 0;
  }

  const std::uint8_t flags = data[0];
  // libFuzzer exposes bytes as uint8_t; a char view preserves the same object representation.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const std::string_view payload(reinterpret_cast<const char*>(data + 1), size - 1);
  const std::size_t part_size = payload.size() / 5;
  const mbo::strings::ParseOptions options{
      .stop_at_any_of = payload.substr(0 * part_size, part_size),
      .stop_at_str = payload.substr(1 * part_size, part_size),
      .split_at_any_of = payload.substr(2 * part_size, part_size),
      .enable_double_quotes = (flags & 0x01U) != 0,
      .enable_single_quotes = (flags & 0x02U) != 0,
      .remove_quotes = (flags & 0x04U) != 0,
      .allow_unquoted = (flags & 0x08U) != 0,
      .custom_escapes = payload.substr(3 * part_size, part_size),
  };
  const std::string_view input = payload.substr(4 * part_size);

  std::string_view single_input = input;
  (void)mbo::strings::ParseString(options, single_input);

  std::string_view list_input = input;
  (void)mbo::strings::ParseStringList(options, list_input);
  return 0;
}
