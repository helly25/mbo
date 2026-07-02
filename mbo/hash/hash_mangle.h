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

#ifndef MBO_HASH_HASH_MANGLE_H_
#define MBO_HASH_HASH_MANGLE_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_simple.h"

namespace mbo::hash {

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
inline constexpr uint64_t HashMangle(uint64_t data) {
  // In theory this should be a random number or at least an arbitray number that
  // changes on every program start. Howeverm this number must be constexpr.
  constexpr uint64_t kNotSoRandom =  //
      5'008'709'998'333'326'415ULL
#if defined(__DATE__)
      ^ (MBO_CONSTANT_P(__DATE__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__DATE__)))
#endif
#if defined(__TIME__)
      ^ (MBO_CONSTANT_P(__TIME__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__TIME__)))
#elif defined(__TIMESTAMP__)
      ^ (MBO_CONSTANT_P(__TIMESTAMP__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__TIMESTAMP__)))
#endif
      ;
  return data ^ kNotSoRandom;
}

#undef MBO_CONSTANT_P  // Be gone.

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_MANGLE_H_
