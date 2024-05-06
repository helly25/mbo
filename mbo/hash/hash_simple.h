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
namespace mbo::hash::hash_internal {

inline constexpr uint64_t GetSimpleHash(std::string_view data) {
  // NOLINTBEGIN(*-magic-numbers)
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
    uint64_t result = kArbitrary + len;
    std::size_t pos = 0;
    while (len >= 4) {
      uint64_t add = 0;
      add += static_cast<uint64_t>(data[pos++]);
      add += static_cast<uint64_t>(data[pos++]) << 8U;
      add += static_cast<uint64_t>(data[pos++]) << 16U;
      add += static_cast<uint64_t>(data[pos++]) << 24U;
      result = result * 6'571 ^ ((17 * add + (add >> 16U)) ^ (result >> 32U));
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
    uint64_t result = kArbitrary + data.length();
    uint64_t add = 0;
    while (str < end) {
      std::memcpy(&add, str, 4);
      result = result * 6'571 ^ ((17 * add + (add >> 16U)) ^ (result >> 32U));
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

}  // namespace mbo::hash::hash_internal

namespace mbo::hash::simple {

// NOLINTBEGIN(*-macro-usage)
#define MBO_CONSTANT_P(expression, fallback) (fallback)
#ifdef __has_builtin
# if __has_builtin(__builtin_constant_p)
#  undef MBO_CONSTANT_P
#  define MBO_CONSTANT_P(expression, fallback) __builtin_constant_p(expression)
# endif
#endif
// NOLINTEND(*-macro-usage)

// A simple constexpr safe hash function.
//
// This function uses an optimized implementation at run-time and a constexpr safe implementation
// that yields the exact same values for compile-time usage. It uses compiler provided macros as a
// seed generator. This means that the seed **may** vary from run to run, but there is no guarantee
// as a dev environment (e.g. Bazel) may set these macros always to the same value.
inline constexpr uint64_t GetHash(std::string_view data) {
  using mbo::hash::hash_internal::GetSimpleHash;
  // In theory this should be a random number or at least an arbitray number that
  // changes on every program start. Howeverm this number must be constexpr.
  constexpr uint64_t kNotSoRandom =  //
      5'008'709'998'333'326'415ULL
#if defined(__DATE__)
      ^ (MBO_CONSTANT_P(__DATE__, 1) == 0 ? 0 : GetSimpleHash(std::string_view(__DATE__)))
#endif
#if defined(__TIME__)
      ^ (MBO_CONSTANT_P(__TIME__, 1) == 0 ? 0 : GetSimpleHash(std::string_view(__TIME__)))
#endif
#if defined(__TIMESTAMP__)
      ^ (MBO_CONSTANT_P(__TIMESTAMP__, 1) == 0 ? 0 : GetSimpleHash(std::string_view(__TIMESTAMP__)))
#endif
      ;
  return GetSimpleHash(data) ^ kNotSoRandom;
}

#undef MBO_CONSTANT_P  // Be gone.

}  // namespace mbo::hash::simple

#endif  // MBO_HASH_SIMPLE_HASH_H_
