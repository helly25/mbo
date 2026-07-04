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

#ifndef MBO_HASH_HASH_RAPIDHASH_H_
#define MBO_HASH_HASH_RAPIDHASH_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// rapidhash V3 (Nicolas De Carli; MIT-licensed, based on wyhash): the modern
// wyhash-family small-key latency champion and SMHasher3-clean. This is a
// constexpr-safe transcription of the reference implementation's default
// ("FAST", non-compact) variant, producing the canonical `rapidhash_withSeed`
// values on every platform (little-endian defined), verified against vectors
// generated with the reference header.
namespace mbo::hash::rapidhash {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

namespace rapidhash_internal {

inline constexpr std::array<uint64_t, 8> kSecret = {
    0x2d358dccaa6c78a5ULL, 0x8bb84b93962eacc9ULL, 0x4b33a62ed433d4a3ULL, 0x4d5a2da51de1aa47ULL,
    0xa0761d6478bd642fULL, 0xe7037ed1a0b428dbULL, 0x90ed1765281c388cULL, 0xaaaaaaaaaaaaaaaaULL,
};

// rapid_mix: 64x64->128 multiply folded by XOR (the "FAST" variant).
constexpr uint64_t Mix(uint64_t lhs, uint64_t rhs) noexcept {
  return hash_internal::Mul128Fold64(lhs, rhs);
}

}  // namespace rapidhash_internal

// Canonical rapidhash V3 (`rapidhash_withSeed`; default seed 0 as in the
// reference implementation).
// NOLINTNEXTLINE(readability-function-cognitive-complexity): faithful transcription of the reference.
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = 0) noexcept {
  using rapidhash_internal::kSecret;
  using rapidhash_internal::Mix;

  const char* ptr = str.data();
  const std::size_t len = str.size();

  seed ^= Mix(seed ^ kSecret[2], kSecret[1]);
  uint64_t val_a = 0;
  uint64_t val_b = 0;
  std::size_t remaining = len;

  if (len <= 16) {
    if (len >= 4) {
      seed ^= len;
      if (len >= 8) {
        val_a = hash_internal::Load64(ptr);
        val_b = hash_internal::Load64(ptr + len - 8);
      } else {
        val_a = hash_internal::Load32(ptr);
        val_b = hash_internal::Load32(ptr + len - 4);
      }
    } else if (len > 0) {
      val_a = (static_cast<uint64_t>(static_cast<uint8_t>(ptr[0])) << 45U)
              | static_cast<uint64_t>(static_cast<uint8_t>(ptr[len - 1]));
      val_b = static_cast<uint64_t>(static_cast<uint8_t>(ptr[len >> 1U]));
    }
  } else {
    if (len > 112) {
      uint64_t see1 = seed;
      uint64_t see2 = seed;
      uint64_t see3 = seed;
      uint64_t see4 = seed;
      uint64_t see5 = seed;
      uint64_t see6 = seed;
      while (remaining > 224) {
        seed = Mix(hash_internal::Load64(ptr) ^ kSecret[0], hash_internal::Load64(ptr + 8) ^ seed);
        see1 = Mix(hash_internal::Load64(ptr + 16) ^ kSecret[1], hash_internal::Load64(ptr + 24) ^ see1);
        see2 = Mix(hash_internal::Load64(ptr + 32) ^ kSecret[2], hash_internal::Load64(ptr + 40) ^ see2);
        see3 = Mix(hash_internal::Load64(ptr + 48) ^ kSecret[3], hash_internal::Load64(ptr + 56) ^ see3);
        see4 = Mix(hash_internal::Load64(ptr + 64) ^ kSecret[4], hash_internal::Load64(ptr + 72) ^ see4);
        see5 = Mix(hash_internal::Load64(ptr + 80) ^ kSecret[5], hash_internal::Load64(ptr + 88) ^ see5);
        see6 = Mix(hash_internal::Load64(ptr + 96) ^ kSecret[6], hash_internal::Load64(ptr + 104) ^ see6);
        seed = Mix(hash_internal::Load64(ptr + 112) ^ kSecret[0], hash_internal::Load64(ptr + 120) ^ seed);
        see1 = Mix(hash_internal::Load64(ptr + 128) ^ kSecret[1], hash_internal::Load64(ptr + 136) ^ see1);
        see2 = Mix(hash_internal::Load64(ptr + 144) ^ kSecret[2], hash_internal::Load64(ptr + 152) ^ see2);
        see3 = Mix(hash_internal::Load64(ptr + 160) ^ kSecret[3], hash_internal::Load64(ptr + 168) ^ see3);
        see4 = Mix(hash_internal::Load64(ptr + 176) ^ kSecret[4], hash_internal::Load64(ptr + 184) ^ see4);
        see5 = Mix(hash_internal::Load64(ptr + 192) ^ kSecret[5], hash_internal::Load64(ptr + 200) ^ see5);
        see6 = Mix(hash_internal::Load64(ptr + 208) ^ kSecret[6], hash_internal::Load64(ptr + 216) ^ see6);
        ptr += 224;
        remaining -= 224;
      }
      if (remaining > 112) {
        seed = Mix(hash_internal::Load64(ptr) ^ kSecret[0], hash_internal::Load64(ptr + 8) ^ seed);
        see1 = Mix(hash_internal::Load64(ptr + 16) ^ kSecret[1], hash_internal::Load64(ptr + 24) ^ see1);
        see2 = Mix(hash_internal::Load64(ptr + 32) ^ kSecret[2], hash_internal::Load64(ptr + 40) ^ see2);
        see3 = Mix(hash_internal::Load64(ptr + 48) ^ kSecret[3], hash_internal::Load64(ptr + 56) ^ see3);
        see4 = Mix(hash_internal::Load64(ptr + 64) ^ kSecret[4], hash_internal::Load64(ptr + 72) ^ see4);
        see5 = Mix(hash_internal::Load64(ptr + 80) ^ kSecret[5], hash_internal::Load64(ptr + 88) ^ see5);
        see6 = Mix(hash_internal::Load64(ptr + 96) ^ kSecret[6], hash_internal::Load64(ptr + 104) ^ see6);
        ptr += 112;
        remaining -= 112;
      }
      seed ^= see1;
      see2 ^= see3;
      see4 ^= see5;
      seed ^= see6;
      see2 ^= see4;
      seed ^= see2;
    }
    if (remaining > 16) {
      seed = Mix(hash_internal::Load64(ptr) ^ kSecret[2], hash_internal::Load64(ptr + 8) ^ seed);
      if (remaining > 32) {
        seed = Mix(hash_internal::Load64(ptr + 16) ^ kSecret[2], hash_internal::Load64(ptr + 24) ^ seed);
        if (remaining > 48) {
          seed = Mix(hash_internal::Load64(ptr + 32) ^ kSecret[1], hash_internal::Load64(ptr + 40) ^ seed);
          if (remaining > 64) {
            seed = Mix(hash_internal::Load64(ptr + 48) ^ kSecret[1], hash_internal::Load64(ptr + 56) ^ seed);
            if (remaining > 80) {
              seed = Mix(hash_internal::Load64(ptr + 64) ^ kSecret[2], hash_internal::Load64(ptr + 72) ^ seed);
              if (remaining > 96) {
                seed = Mix(hash_internal::Load64(ptr + 80) ^ kSecret[1], hash_internal::Load64(ptr + 88) ^ seed);
              }
            }
          }
        }
      }
    }
    val_a = hash_internal::Load64(ptr + remaining - 16) ^ remaining;
    val_b = hash_internal::Load64(ptr + remaining - 8);
  }

  val_a ^= kSecret[1];
  val_b ^= seed;
  const Hash128 product = hash_internal::Mult128(val_a, val_b);
  return Mix(product.h1 ^ kSecret[7], product.h2 ^ kSecret[1] ^ remaining);
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Note that
// the canonical rapidhash default seed is 0, not `kDefaultSeed`.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::rapidhash::GetHash64(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::rapidhash

#endif  // MBO_HASH_HASH_RAPIDHASH_H_
