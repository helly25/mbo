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

#ifndef MBO_HASH_HASH_SIMPLE_H_
#define MBO_HASH_HASH_SIMPLE_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace mbo::hash::simple {

// NOLINTBEGIN(*-identifier-naming,*-magic-numbers,*-constant-array-index,*-pointer-arithmetic,*-signed-bitwise)

// Namespace `mbo::hash::simple` offers a `GetHash` a constexpr safe simple hash function.
namespace hash_internal {

inline constexpr uint64_t GetSimpleHash(std::string_view data) {
  constexpr uint64_t kArbitrary = 5'008'709'998'333'326'415ULL;
  constexpr uint64_t kPrimeNum10k = 104'729ULL;
  if (data.empty()) {
    return 0x892df5cfULL ^ kArbitrary;  // Arbitrary number (neither 0 nor -1).
  }
  // Below we use an optimized form that directly reads from memory (using`memcpy`). However, that
  // is not allowed in a constant evaluated expression (assign result to constexpr var). In order to
  // work around that limitation, we check `std::is_constant_evaluated()` and provide a slower, yet
  // compile-time evaluated variant. Now we could think that the function needs to be checked in
  // `if constexpr ()`, but that leads to nice errors as it would seemingly always be true.
  // Clang: error: 'std::is_constant_evaluated' will always evaluate to 'true' in a manifestly
  //               constant-evaluated expression [-Werror,-Wconstant-evaluated]
  // GCC:   error: 'std::is_constant_evaluated' always evaluates to true in 'if constexpr'
  //               [-Werror=tautological-compare]
  if (std::is_constant_evaluated()) {
    std::size_t len = data.length();
    std::size_t pos = 0;
    uint64_t result = kArbitrary + len;
    while (len >= 4) {
      uint64_t add = 0;
      add += data[pos++];
      add += data[pos++] << 8U;
      add += data[pos++] << 16U;
      add += data[pos++] << 24U;
      result = (result * 6'571) ^ ((17 * add + (add >> 16U)) ^ (result >> 32U));
      len -= 4;
    }
    uint64_t add = 0;
    switch (len) {
      case 3: add += data[pos + 2] << 16U;
      case 2: add += data[pos + 1] << 8U;
      case 1: add += data[pos]; return (result * 193) + (kPrimeNum10k * add);
      default: break;
    }
    return result;
  } else {
    const char* str = data.data();
    const char* end = data.end() - 3;
    uint64_t result = kArbitrary + data.length();
    uint64_t add = 0;
    while (str < end) {
      std::memcpy(&add, str, 4);
      result = (result * 6'571) ^ ((17 * add + (add >> 16U)) ^ (result >> 32U));
      str += 4;
    }
    switch (data.length() % 4) {  // NOLINT(*-default-case)
      case 3:
        add = 0;
        std::memcpy(&add, str, 3);
        return (result * 193) + (kPrimeNum10k * add);
      case 2:
        add = 0;
        std::memcpy(&add, str, 2);
        return (result * 193) + (kPrimeNum10k * add);
      case 1:
        add = 0;
        std::memcpy(&add, str, 1);
        return (result * 193) + (kPrimeNum10k * add);
      case 0: return result;
    }
    return result;
  }
}

}  // namespace hash_internal

inline constexpr uint64_t GetHash64(std::string_view data) {
  return hash_internal::GetSimpleHash(data);
}

// Deprecated in version 0.13.0
[[deprecated("Use ::mbo::hash::GetHash64() instead.")]] inline constexpr uint64_t GetHash(std::string_view data) {
  return hash_internal::GetSimpleHash(data);
}

// NOLINTEND(*-identifier-naming,*-magic-numbers,*-constant-array-index,*-pointer-arithmetic,*-signed-bitwise)

}  // namespace mbo::hash::simple

#endif  // MBO_HASH_HASH_SIMPLE_H_
