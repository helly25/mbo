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

// Number of distinct per-build mangle seeds.
//
// The mangle mixes a build-derived seed into a hash so precomputed inputs cannot
// be relied on across builds. Deriving the seed directly from `__DATE__` /
// `__TIME__` yields a *full 64-bit*, effectively unbounded seed that changes
// every compile -- which bakes a different constant into every build and so
// defeats reproducible builds and build caches. Bounding the seed to a small,
// fixed set of buckets keeps a little per-build variation while capping the
// number of possible outputs (so caches converge). Use a power of two in
// {4, 8, 16}; set to 1 to disable variation entirely.
inline constexpr uint64_t kMangleSeedCount = 16;

// Mixes `data` with one of `kMangleSeedCount` build-derived seeds (constexpr safe).
//
// The `__DATE__` / `__TIME__` hash only *selects* a bucket via `% kMangleSeedCount`;
// the bucket is then expanded into a full-width constant so the whole hash is
// mangled, not just its low bits. A hermetic build (e.g. Bazel pinning these
// macros) collapses to a single, stable bucket.
inline constexpr uint64_t HashMangle(uint64_t data) {
  constexpr uint64_t kBase = 5'008'709'998'333'326'415ULL;
  constexpr uint64_t kSpread = 0x9E3779B97F4A7C15ULL;  // full-width odd: spreads a bucket across all bits

  constexpr uint64_t kDateTime =  //
      0ULL
#if defined(__DATE__)
      ^ (MBO_CONSTANT_P(__DATE__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__DATE__)))
#endif
#if defined(__TIME__)
      ^ (MBO_CONSTANT_P(__TIME__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__TIME__)))
#elif defined(__TIMESTAMP__)
      ^ (MBO_CONSTANT_P(__TIMESTAMP__, 1) == 0 ? 0 : simple::GetHash64(std::string_view(__TIMESTAMP__)))
#endif
      ;
  constexpr uint64_t kBucket = kDateTime % kMangleSeedCount;  // 0 .. kMangleSeedCount-1
  constexpr uint64_t kSeed = kBase ^ (kBucket * kSpread);
  return data ^ kSeed;
}

#undef MBO_CONSTANT_P  // Be gone.

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_MANGLE_H_
