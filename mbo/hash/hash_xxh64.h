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

#ifndef MBO_HASH_HASH_XXH64_H_
#define MBO_HASH_HASH_XXH64_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// XXH64: a constexpr-safe implementation of the xxHash 64-bit algorithm
// (https://xxhash.com, algorithm spec is BSD-2-Clause). Produces the canonical
// XXH64 values (the algorithm is little-endian defined; loads here are
// little-endian on every platform), so results can be compared against other
// xxHash implementations and tools.
namespace mbo::hash::xxh64 {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

inline constexpr uint64_t kPrime1 = 0x9E3779B185EBCA87ULL;
inline constexpr uint64_t kPrime2 = 0xC2B2AE3D27D4EB4FULL;
inline constexpr uint64_t kPrime3 = 0x165667B19E3779F9ULL;
inline constexpr uint64_t kPrime4 = 0x85EBCA77C2B2AE63ULL;
inline constexpr uint64_t kPrime5 = 0x27D4EB2F165667C5ULL;

namespace xxh64_internal {

constexpr uint64_t Round(uint64_t acc, uint64_t input) noexcept {
  return std::rotl(acc + (input * kPrime2), 31) * kPrime1;
}

constexpr uint64_t MergeRound(uint64_t hash, uint64_t lane) noexcept {
  return ((hash ^ Round(0, lane)) * kPrime1) + kPrime4;
}

}  // namespace xxh64_internal

// Canonical XXH64 (default seed 0, as in the reference implementation).
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = 0) noexcept {
  const std::size_t len = str.size();
  const char* ptr = str.data();
  std::size_t remaining = len;

  uint64_t hash = 0;
  if (remaining >= 32) {
    uint64_t acc1 = seed + kPrime1 + kPrime2;
    uint64_t acc2 = seed + kPrime2;
    uint64_t acc3 = seed;
    uint64_t acc4 = seed - kPrime1;
    while (remaining >= 32) {
      acc1 = xxh64_internal::Round(acc1, hash_internal::Load64(ptr));
      acc2 = xxh64_internal::Round(acc2, hash_internal::Load64(ptr + 8));
      acc3 = xxh64_internal::Round(acc3, hash_internal::Load64(ptr + 16));
      acc4 = xxh64_internal::Round(acc4, hash_internal::Load64(ptr + 24));
      ptr += 32;
      remaining -= 32;
    }
    hash = std::rotl(acc1, 1) + std::rotl(acc2, 7) + std::rotl(acc3, 12) + std::rotl(acc4, 18);
    hash = xxh64_internal::MergeRound(hash, acc1);
    hash = xxh64_internal::MergeRound(hash, acc2);
    hash = xxh64_internal::MergeRound(hash, acc3);
    hash = xxh64_internal::MergeRound(hash, acc4);
  } else {
    hash = seed + kPrime5;
  }

  hash += len;

  while (remaining >= 8) {
    hash ^= xxh64_internal::Round(0, hash_internal::Load64(ptr));
    hash = (std::rotl(hash, 27) * kPrime1) + kPrime4;
    ptr += 8;
    remaining -= 8;
  }
  if (remaining >= 4) {
    hash ^= static_cast<uint64_t>(hash_internal::Load32(ptr)) * kPrime1;
    hash = (std::rotl(hash, 23) * kPrime2) + kPrime3;
    ptr += 4;
    remaining -= 4;
  }
  while (remaining > 0) {
    hash ^= static_cast<uint64_t>(static_cast<uint8_t>(*ptr)) * kPrime5;
    hash = std::rotl(hash, 11) * kPrime1;
    ++ptr;
    --remaining;
  }

  hash ^= hash >> 33U;
  hash *= kPrime2;
  hash ^= hash >> 29U;
  hash *= kPrime3;
  hash ^= hash >> 32U;
  return hash;
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Note that
// the canonical XXH64 default seed is 0, not `kDefaultSeed`.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::xxh64::GetHash64(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::xxh64

#endif  // MBO_HASH_HASH_XXH64_H_
