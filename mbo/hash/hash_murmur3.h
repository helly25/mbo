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

#ifndef MBO_HASH_HASH_MURMUR3_H_
#define MBO_HASH_HASH_MURMUR3_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

// Upstream attribution: MurmurHash3 by Austin Appleby, placed in the public
// domain (https://github.com/aappleby/smhasher). This file is a constexpr
// transcription of the algorithm.

// MurmurHash3 x64 128-bit (Austin Appleby; public domain): a constexpr-safe
// implementation producing the canonical MurmurHash3_x64_128 values (`h1` is the
// first 8 output bytes, `h2` the second; the algorithm is little-endian defined,
// and loads here are little-endian on every platform). `GetHash64` returns `h1`,
// the customary 64-bit truncation.
namespace mbo::hash::murmur3 {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

// 128-bit variant, canonical MurmurHash3_x64_128 (default seed 0 as in the
// reference implementation; the reference takes a 32-bit seed -- pass seeds
// within uint32_t range for value compatibility).
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = 0) noexcept {
  constexpr uint64_t kMul1 = 0x87C37B91114253D5ULL;
  constexpr uint64_t kMul2 = 0x4CF5AD432745937FULL;

  const std::size_t len = str.size();
  const char* ptr = str.data();

  uint64_t hash1 = seed;
  uint64_t hash2 = seed;

  const std::size_t blocks = len / 16;
  for (std::size_t i = 0; i < blocks; ++i) {
    uint64_t key1 = hash_internal::Load64(ptr);
    uint64_t key2 = hash_internal::Load64(ptr + 8);
    ptr += 16;

    key1 *= kMul1;
    key1 = std::rotl(key1, 31);
    key1 *= kMul2;
    hash1 ^= key1;
    hash1 = std::rotl(hash1, 27);
    hash1 += hash2;
    hash1 = (hash1 * 5) + 0x52DCE729ULL;

    key2 *= kMul2;
    key2 = std::rotl(key2, 33);
    key2 *= kMul1;
    hash2 ^= key2;
    hash2 = std::rotl(hash2, 31);
    hash2 += hash1;
    hash2 = (hash2 * 5) + 0x38495AB5ULL;
  }

  const std::size_t tail = len % 16;
  if (tail > 8) {
    uint64_t key2 = hash_internal::LoadTail(ptr + 8, tail - 8);
    key2 *= kMul2;
    key2 = std::rotl(key2, 33);
    key2 *= kMul1;
    hash2 ^= key2;
  }
  if (tail > 0) {
    uint64_t key1 = hash_internal::LoadTail(ptr, tail < 8 ? tail : 8);
    key1 *= kMul1;
    key1 = std::rotl(key1, 31);
    key1 *= kMul2;
    hash1 ^= key1;
  }

  hash1 ^= len;
  hash2 ^= len;
  hash1 += hash2;
  hash2 += hash1;
  hash1 = hash_internal::Fmix64(hash1);  // Fmix64 *is* MurmurHash3's fmix64.
  hash2 = hash_internal::Fmix64(hash2);
  hash1 += hash2;
  hash2 += hash1;

  return {.h1 = hash1, .h2 = hash2};
}

// 64-bit variant: the first 64 output bits (`h1`), the customary truncation.
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = 0) noexcept {
  return GetHash128(str, seed).h1;
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Note that
// the canonical MurmurHash3 default seed is 0, not `kDefaultSeed`.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::murmur3::GetHash64(data, seed);
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::murmur3::GetHash128(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::murmur3

#endif  // MBO_HASH_HASH_MURMUR3_H_
