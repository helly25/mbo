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

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>

#include "mbo/file/glob.h"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  // libFuzzer exposes bytes as uint8_t; a char view preserves the same object representation.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const std::string_view pattern(reinterpret_cast<const char*>(data), size);
  for (const bool allow_star_star : {false, true}) {
    for (const bool allow_ranges : {false, true}) {
      const mbo::file::Glob2Re2Options options{
          .allow_star_star = allow_star_star,
          .allow_ranges = allow_ranges,
      };
      (void)mbo::file::file_internal::Glob2Re2Expression(pattern, options);
      (void)mbo::file::file_internal::Glob2Re2(pattern, options);
      (void)mbo::file::file_internal::GlobSplitParts(pattern, options);
      (void)mbo::file::file_internal::GlobSplit(pattern, options);
    }
  }
  return 0;
}
