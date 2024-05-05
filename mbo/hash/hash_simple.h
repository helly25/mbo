// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#ifndef MBO_HASH_SIMPLE_HASH_H_
#define MBO_HASH_SIMPLE_HASH_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

// Namespace `mbo::hash::simple` offers a `GetHash` a constexpr safe simple hash function.
namespace mbo::hash::simple {

// In theory this should be a random number or at least an arbitray number that
// changes on every program start. Howeverm this number must be constexpr.
static constexpr uint64_t kMaybeRandom = 5'008'709'998'333'326'415ULL;

// A simple constexpr safe hash function. This function uses an optimized implementation at run-time
// and a constexpr safe implementation that yields the exact same values for compile-time usage.
inline constexpr uint64_t GetHash(std::string_view data) {
  // NOLINTBEGIN(*-magic-numbers)
  if (data.empty()) {
    return 0x892df5cfULL ^ kMaybeRandom;  // Arbitrary number (neither 0 nor -1).
  }
  constexpr uint64_t kPrimeNum10k = 104'729ULL;
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
    uint64_t result = kMaybeRandom;
    std::size_t pos = 0;
    while (len >= 4) {
      uint64_t add = 0;
      add += static_cast<uint64_t>(data[pos++]);
      add += static_cast<uint64_t>(data[pos++]) << 8U;
      add += static_cast<uint64_t>(data[pos++]) << 16U;
      add += static_cast<uint64_t>(data[pos++]) << 24U;
      result = result * 6'571 + (17 * add + (add >> 16U));
      len -= 4;
    }
    if (len == 3) {
      uint64_t add = 0;
      add += static_cast<uint64_t>(data[pos++]);
      add += static_cast<uint64_t>(data[pos++]) << 8U;
      add += static_cast<uint64_t>(data[pos++]) << 16U;
      return result * 193 + kPrimeNum10k * add;
    } else if (len == 2) {
      uint64_t add = 0;
      add += static_cast<uint64_t>(data[pos++]);
      add += static_cast<uint64_t>(data[pos++]) << 8U;
      return result * 193 + kPrimeNum10k * add;
    } else if (len == 1) {
      uint64_t add = 0;
      add += static_cast<uint64_t>(data[pos++]);
      return result * 193 + kPrimeNum10k * add;
    }
    return result;
  } else {
    const char* str = data.data();
    const char* end = data.end() - 3;
    uint64_t result = kMaybeRandom;
    uint64_t add = 0;
    while (str < end) {
      std::memcpy(&add, str, 4);
      result = result * 6'571 + (17 * add + (add >> 16U));
      str += 4;  // NOLINT(*-pointer-arithmetic)
    }
    switch (data.length() % 4) {  // NOLINT(*-default-case)
      case 3:
        add = 0;
        std::memcpy(&add, str, 3);
        return result * 193 + kPrimeNum10k * add;
      case 2:
        add = 0;
        std::memcpy(&add, str, 2);
        return result * 193 + kPrimeNum10k * add;
      case 1:
        add = 0;
        std::memcpy(&add, str, 1);
        return result * 193 + kPrimeNum10k * add;
      case 0: return result;
    }
    return result;
  }
  // NOLINTEND(*-magic-numbers)
}

}  // namespace mbo::hash::simple

#endif  // MBO_HASH_SIMPLE_HASH_H_
