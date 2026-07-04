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

#ifndef MBO_HASH_HASH_XXH3_H_
#define MBO_HASH_HASH_XXH3_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_xxh64.h"

// Upstream attribution: xxHash - Copyright (C) 2019-present Yann Collet,
// BSD 2-Clause License (https://github.com/Cyan4973/xxHash). This file is a
// constexpr transcription of the
// algorithm; the full upstream license text is reproduced in the
// repository-root NOTICE file.

// XXH3 (64- and 128-bit): a constexpr-safe scalar implementation of the modern xxHash
// generation (https://xxhash.com, spec BSD-2-Clause; transcribed from the
// reference implementation v0.8.2). Produces the canonical `XXH3_64bits[_withSeed]`
// and `XXH3_128bits[_withSeed]` values on every platform (little-endian defined),
// verified against reference vectors. Scalar only -- the reference's SIMD
// kernels produce the same values but are not constexpr-compatible.
namespace mbo::hash::xxh3 {

// The 192-byte secret is kept as a plain char array so `hash_internal::Load64`
// reads it constexpr-safely and memcpy-foldably; hence the c-array/decay waivers.
// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-avoid-c-arrays,*-array-to-pointer-decay,*-constant-array-index,*-easily-swappable-parameters)

namespace xxh3_internal {

// The default 192-byte secret (XXH3_kSecret), byte-for-byte.
inline constexpr char kSecret[] =
    "\xb8\xfe\x6c\x39\x23\xa4\x4b\xbe\x7c\x01\x81\x2c\xf7\x21\xad\x1c"
    "\xde\xd4\x6d\xe9\x83\x90\x97\xdb\x72\x40\xa4\xa4\xb7\xb3\x67\x1f"
    "\xcb\x79\xe6\x4e\xcc\xc0\xe5\x78\x82\x5a\xd0\x7d\xcc\xff\x72\x21"
    "\xb8\x08\x46\x74\xf7\x43\x24\x8e\xe0\x35\x90\xe6\x81\x3a\x26\x4c"
    "\x3c\x28\x52\xbb\x91\xc3\x00\xcb\x88\xd0\x65\x8b\x1b\x53\x2e\xa3"
    "\x71\x64\x48\x97\xa2\x0d\xf9\x4e\x38\x19\xef\x46\xa9\xde\xac\xd8"
    "\xa8\xfa\x76\x3f\xe3\x9c\x34\x3f\xf9\xdc\xbb\xc7\xc7\x0b\x4f\x1d"
    "\x8a\x51\xe0\x4b\xcd\xb4\x59\x31\xc8\x9f\x7e\xc9\xd9\x78\x73\x64"
    "\xea\xc5\xac\x83\x34\xd3\xeb\xc3\xc5\x81\xa0\xff\xfa\x13\x63\xeb"
    "\x17\x0d\xdd\x51\xb7\xf0\xda\x49\xd3\x16\x55\x26\x29\xd4\x68\x9e"
    "\x2b\x16\xbe\x58\x7d\x47\xa1\xfc\x8f\xf8\xb8\xd1\x7a\xd0\x31\xce"
    "\x45\xcb\x3a\x8f\x95\x16\x04\x28\xaf\xd7\xfb\xca\xbb\x4b\x40\x7e";

inline constexpr std::size_t kSecretSize = 192;
inline constexpr std::size_t kStripeLen = 64;
inline constexpr std::size_t kAccNb = 8;
inline constexpr std::size_t kStripesPerBlock = (kSecretSize - kStripeLen) / 8;  // 16
inline constexpr std::size_t kBlockLen = kStripeLen * kStripesPerBlock;          // 1024
inline constexpr std::size_t kMidSizeMax = 240;

inline constexpr uint64_t kPrimeMx1 = 0x165667919E3779F9ULL;
inline constexpr uint64_t kPrimeMx2 = 0x9FB21C651E98DF25ULL;
inline constexpr uint64_t kPrime32_1 = 0x9E3779B1ULL;
inline constexpr uint64_t kPrime32_2 = 0x85EBCA77ULL;
inline constexpr uint64_t kPrime32_3 = 0xC2B2AE3DULL;

constexpr uint32_t Swap32(uint32_t value) noexcept {
  return ((value << 24U) & 0xFF000000U) | ((value << 8U) & 0x00FF0000U) | ((value >> 8U) & 0x0000FF00U)
         | ((value >> 24U) & 0x000000FFU);
}

constexpr uint64_t Swap64(uint64_t value) noexcept {
  return (static_cast<uint64_t>(Swap32(static_cast<uint32_t>(value))) << 32U)
         | static_cast<uint64_t>(Swap32(static_cast<uint32_t>(value >> 32U)));
}

// The classic XXH64 finalizer (XXH64_avalanche).
constexpr uint64_t Xxh64Avalanche(uint64_t hash) noexcept {
  hash ^= hash >> 33U;
  hash *= xxh64::kPrime2;
  hash ^= hash >> 29U;
  hash *= xxh64::kPrime3;
  hash ^= hash >> 32U;
  return hash;
}

// XXH3_avalanche: fast mixer for inputs with already-good entropy.
constexpr uint64_t Avalanche(uint64_t hash) noexcept {
  hash ^= hash >> 37U;
  hash *= kPrimeMx1;
  hash ^= hash >> 32U;
  return hash;
}

// XXH3_rrmxmx: stronger finalizer for the 4..8 byte class.
constexpr uint64_t Rrmxmx(uint64_t hash, uint64_t len) noexcept {
  hash ^= std::rotl(hash, 49) ^ std::rotl(hash, 24);
  hash *= kPrimeMx2;
  hash ^= (hash >> 35U) + len;
  hash *= kPrimeMx2;
  return hash ^ (hash >> 28U);
}

constexpr uint64_t Len0To16(const char* input, std::size_t len, uint64_t seed) noexcept {
  if (len > 8) {  // 9..16
    const uint64_t bitflip1 = (hash_internal::Load64(kSecret + 24) ^ hash_internal::Load64(kSecret + 32)) + seed;
    const uint64_t bitflip2 = (hash_internal::Load64(kSecret + 40) ^ hash_internal::Load64(kSecret + 48)) - seed;
    const uint64_t input_lo = hash_internal::Load64(input) ^ bitflip1;
    const uint64_t input_hi = hash_internal::Load64(input + len - 8) ^ bitflip2;
    const uint64_t acc = len + Swap64(input_lo) + input_hi + hash_internal::Mul128Fold64(input_lo, input_hi);
    return Avalanche(acc);
  }
  if (len >= 4) {  // 4..8
    seed ^= static_cast<uint64_t>(Swap32(static_cast<uint32_t>(seed))) << 32U;
    const uint64_t input1 = hash_internal::Load32(input);
    const uint64_t input2 = hash_internal::Load32(input + len - 4);
    const uint64_t bitflip = (hash_internal::Load64(kSecret + 8) ^ hash_internal::Load64(kSecret + 16)) - seed;
    const uint64_t input64 = input2 + (input1 << 32U);
    return Rrmxmx(input64 ^ bitflip, len);
  }
  if (len > 0) {  // 1..3
    const auto chr1 = static_cast<uint8_t>(input[0]);
    const auto chr2 = static_cast<uint8_t>(input[len >> 1U]);
    const auto chr3 = static_cast<uint8_t>(input[len - 1]);
    const uint32_t combined = (static_cast<uint32_t>(chr1) << 16U) | (static_cast<uint32_t>(chr2) << 24U)
                              | static_cast<uint32_t>(chr3) | (static_cast<uint32_t>(len) << 8U);
    const uint64_t bitflip = (static_cast<uint64_t>(hash_internal::Load32(kSecret))
                              ^ static_cast<uint64_t>(hash_internal::Load32(kSecret + 4)))
                             + seed;
    return Xxh64Avalanche(static_cast<uint64_t>(combined) ^ bitflip);
  }
  return Xxh64Avalanche(seed ^ hash_internal::Load64(kSecret + 56) ^ hash_internal::Load64(kSecret + 64));
}

constexpr uint64_t Mix16B(const char* input, const char* secret, uint64_t seed) noexcept {
  return hash_internal::Mul128Fold64(
      hash_internal::Load64(input) ^ (hash_internal::Load64(secret) + seed),
      hash_internal::Load64(input + 8) ^ (hash_internal::Load64(secret + 8) - seed));
}

constexpr uint64_t Len17To128(const char* input, std::size_t len, uint64_t seed) noexcept {
  uint64_t acc = len * xxh64::kPrime1;
  if (len > 32) {
    if (len > 64) {
      if (len > 96) {
        acc += Mix16B(input + 48, kSecret + 96, seed);
        acc += Mix16B(input + len - 64, kSecret + 112, seed);
      }
      acc += Mix16B(input + 32, kSecret + 64, seed);
      acc += Mix16B(input + len - 48, kSecret + 80, seed);
    }
    acc += Mix16B(input + 16, kSecret + 32, seed);
    acc += Mix16B(input + len - 32, kSecret + 48, seed);
  }
  acc += Mix16B(input, kSecret, seed);
  acc += Mix16B(input + len - 16, kSecret + 16, seed);
  return Avalanche(acc);
}

constexpr uint64_t Len129To240(const char* input, std::size_t len, uint64_t seed) noexcept {
  constexpr std::size_t kStartOffset = 3;  // XXH3_MIDSIZE_STARTOFFSET
  constexpr std::size_t kLastOffset = 17;  // XXH3_MIDSIZE_LASTOFFSET
  constexpr std::size_t kSecretSizeMin = 136;

  uint64_t acc = len * xxh64::kPrime1;
  const std::size_t rounds = len / 16;
  for (std::size_t i = 0; i < 8; ++i) {
    acc += Mix16B(input + (16 * i), kSecret + (16 * i), seed);
  }
  uint64_t acc_end = Mix16B(input + len - 16, kSecret + kSecretSizeMin - kLastOffset, seed);
  acc = Avalanche(acc);
  for (std::size_t i = 8; i < rounds; ++i) {
    acc_end += Mix16B(input + (16 * i), kSecret + (16 * (i - 8)) + kStartOffset, seed);
  }
  return Avalanche(acc + acc_end);
}

// One 64-byte stripe accumulated against `secret` (XXH3_accumulate_512, scalar).
constexpr void Accumulate512(std::array<uint64_t, kAccNb>& acc, const char* input, const char* secret) noexcept {
  for (std::size_t lane = 0; lane < kAccNb; ++lane) {
    const uint64_t data_val = hash_internal::Load64(input + (lane * 8));
    const uint64_t data_key = data_val ^ hash_internal::Load64(secret + (lane * 8));
    acc[lane ^ 1U] += data_val;
    acc[lane] += (data_key & 0xFFFFFFFFULL) * (data_key >> 32U);
  }
}

constexpr void ScrambleAcc(std::array<uint64_t, kAccNb>& acc, const char* secret) noexcept {
  for (std::size_t lane = 0; lane < kAccNb; ++lane) {
    const uint64_t key64 = hash_internal::Load64(secret + (lane * 8));
    uint64_t acc64 = acc[lane];
    acc64 ^= acc64 >> 47U;
    acc64 ^= key64;
    acc64 *= kPrime32_1;
    acc[lane] = acc64;
  }
}

constexpr uint64_t MergeAccs(const std::array<uint64_t, kAccNb>& acc, const char* secret, uint64_t start) noexcept {
  uint64_t result = start;
  for (std::size_t i = 0; i < 4; ++i) {
    result += hash_internal::Mul128Fold64(
        acc[2 * i] ^ hash_internal::Load64(secret + (16 * i)),
        acc[(2 * i) + 1] ^ hash_internal::Load64(secret + (16 * i) + 8));
  }
  return Avalanche(result);
}

constexpr uint64_t HashLong(const char* input, std::size_t len, const char* secret) noexcept {
  std::array<uint64_t, kAccNb> acc = {
      kPrime32_3,     xxh64::kPrime1, xxh64::kPrime2, xxh64::kPrime3,
      xxh64::kPrime4, kPrime32_2,     xxh64::kPrime5, kPrime32_1,
  };

  const std::size_t nb_blocks = (len - 1) / kBlockLen;
  for (std::size_t block = 0; block < nb_blocks; ++block) {
    for (std::size_t stripe = 0; stripe < kStripesPerBlock; ++stripe) {
      Accumulate512(acc, input + (block * kBlockLen) + (stripe * kStripeLen), secret + (stripe * 8));
    }
    ScrambleAcc(acc, secret + kSecretSize - kStripeLen);
  }

  const std::size_t stripes = ((len - 1) - (kBlockLen * nb_blocks)) / kStripeLen;
  for (std::size_t stripe = 0; stripe < stripes; ++stripe) {
    Accumulate512(acc, input + (nb_blocks * kBlockLen) + (stripe * kStripeLen), secret + (stripe * 8));
  }
  // Last stripe (overlaps; secret offset -7 so it differs from the loop's).
  Accumulate512(acc, input + len - kStripeLen, secret + kSecretSize - kStripeLen - 7);

  return MergeAccs(acc, secret + 11, len * xxh64::kPrime1);
}

// XXH3_initCustomSecret: the seeded secret for the long-input path.
constexpr std::array<char, kSecretSize> InitCustomSecret(uint64_t seed) noexcept {
  std::array<char, kSecretSize> secret = {};
  for (std::size_t i = 0; i < kSecretSize / 16; ++i) {
    const uint64_t low = hash_internal::Load64(kSecret + (16 * i)) + seed;
    const uint64_t high = hash_internal::Load64(kSecret + (16 * i) + 8) - seed;
    for (std::size_t byte = 0; byte < 8; ++byte) {
      secret[(16 * i) + byte] = static_cast<char>((low >> (8 * byte)) & 0xFFU);
      secret[(16 * i) + 8 + byte] = static_cast<char>((high >> (8 * byte)) & 0xFFU);
    }
  }
  return secret;
}

constexpr uint64_t Mult32To64(uint64_t lhs, uint64_t rhs) noexcept {
  return (lhs & 0xFFFFFFFFULL) * (rhs & 0xFFFFFFFFULL);
}

constexpr Hash128 Len0To16_128(const char* input, std::size_t len, uint64_t seed) noexcept {
  if (len > 8) {  // 9..16
    const uint64_t bitflip_lo = (hash_internal::Load64(kSecret + 32) ^ hash_internal::Load64(kSecret + 40)) - seed;
    const uint64_t bitflip_hi = (hash_internal::Load64(kSecret + 48) ^ hash_internal::Load64(kSecret + 56)) + seed;
    const uint64_t input_lo = hash_internal::Load64(input);
    uint64_t input_hi = hash_internal::Load64(input + len - 8);
    Hash128 m128 = hash_internal::Mult128(input_lo ^ input_hi ^ bitflip_lo, xxh64::kPrime1);
    m128.h1 += static_cast<uint64_t>(len - 1) << 54U;
    input_hi ^= bitflip_hi;
    m128.h2 += input_hi + Mult32To64(static_cast<uint32_t>(input_hi), kPrime32_2 - 1);
    m128.h1 ^= Swap64(m128.h2);
    Hash128 h128 = hash_internal::Mult128(m128.h1, xxh64::kPrime2);
    h128.h2 += m128.h2 * xxh64::kPrime2;
    h128.h1 = Avalanche(h128.h1);
    h128.h2 = Avalanche(h128.h2);
    return h128;
  }
  if (len >= 4) {  // 4..8
    seed ^= static_cast<uint64_t>(Swap32(static_cast<uint32_t>(seed))) << 32U;
    const uint64_t input_lo = hash_internal::Load32(input);
    const uint64_t input_hi = hash_internal::Load32(input + len - 4);
    const uint64_t input64 = input_lo + (input_hi << 32U);
    const uint64_t bitflip = (hash_internal::Load64(kSecret + 16) ^ hash_internal::Load64(kSecret + 24)) + seed;
    Hash128 m128 = hash_internal::Mult128(input64 ^ bitflip, xxh64::kPrime1 + (len << 2U));
    m128.h2 += m128.h1 << 1U;
    m128.h1 ^= m128.h2 >> 3U;
    m128.h1 ^= m128.h1 >> 35U;
    m128.h1 *= kPrimeMx2;
    m128.h1 ^= m128.h1 >> 28U;
    m128.h2 = Avalanche(m128.h2);
    return m128;
  }
  if (len > 0) {  // 1..3
    const auto chr1 = static_cast<uint8_t>(input[0]);
    const auto chr2 = static_cast<uint8_t>(input[len >> 1U]);
    const auto chr3 = static_cast<uint8_t>(input[len - 1]);
    const uint32_t combined_lo = (static_cast<uint32_t>(chr1) << 16U) | (static_cast<uint32_t>(chr2) << 24U)
                                 | static_cast<uint32_t>(chr3) | (static_cast<uint32_t>(len) << 8U);
    const uint32_t combined_hi = std::rotl(Swap32(combined_lo), 13);
    const uint64_t bitflip_lo = (static_cast<uint64_t>(hash_internal::Load32(kSecret))
                                 ^ static_cast<uint64_t>(hash_internal::Load32(kSecret + 4)))
                                + seed;
    const uint64_t bitflip_hi = (static_cast<uint64_t>(hash_internal::Load32(kSecret + 8))
                                 ^ static_cast<uint64_t>(hash_internal::Load32(kSecret + 12)))
                                - seed;
    return {
        .h1 = Xxh64Avalanche(static_cast<uint64_t>(combined_lo) ^ bitflip_lo),
        .h2 = Xxh64Avalanche(static_cast<uint64_t>(combined_hi) ^ bitflip_hi),
    };
  }
  return {
      .h1 = Xxh64Avalanche(seed ^ hash_internal::Load64(kSecret + 64) ^ hash_internal::Load64(kSecret + 72)),
      .h2 = Xxh64Avalanche(seed ^ hash_internal::Load64(kSecret + 80) ^ hash_internal::Load64(kSecret + 88)),
  };
}

constexpr Hash128 Mix32B(
    Hash128 acc,
    const char* input1,
    const char* input2,
    const char* secret,
    uint64_t seed) noexcept {
  acc.h1 += Mix16B(input1, secret, seed);
  acc.h1 ^= hash_internal::Load64(input2) + hash_internal::Load64(input2 + 8);
  acc.h2 += Mix16B(input2, secret + 16, seed);
  acc.h2 ^= hash_internal::Load64(input1) + hash_internal::Load64(input1 + 8);
  return acc;
}

constexpr Hash128 Finalize128(Hash128 acc, std::size_t len, uint64_t seed) noexcept {
  Hash128 h128 = {
      .h1 = acc.h1 + acc.h2,
      .h2 = (acc.h1 * xxh64::kPrime1) + (acc.h2 * xxh64::kPrime4) + ((len - seed) * xxh64::kPrime2),
  };
  h128.h1 = Avalanche(h128.h1);
  h128.h2 = 0ULL - Avalanche(h128.h2);
  return h128;
}

constexpr Hash128 Len17To128_128(const char* input, std::size_t len, uint64_t seed) noexcept {
  Hash128 acc = {.h1 = len * xxh64::kPrime1, .h2 = 0};
  if (len > 32) {
    if (len > 64) {
      if (len > 96) {
        acc = Mix32B(acc, input + 48, input + len - 64, kSecret + 96, seed);
      }
      acc = Mix32B(acc, input + 32, input + len - 48, kSecret + 64, seed);
    }
    acc = Mix32B(acc, input + 16, input + len - 32, kSecret + 32, seed);
  }
  acc = Mix32B(acc, input, input + len - 16, kSecret, seed);
  return Finalize128(acc, len, seed);
}

constexpr Hash128 Len129To240_128(const char* input, std::size_t len, uint64_t seed) noexcept {
  constexpr std::size_t kStartOffset = 3;  // XXH3_MIDSIZE_STARTOFFSET
  constexpr std::size_t kLastOffset = 17;  // XXH3_MIDSIZE_LASTOFFSET
  constexpr std::size_t kSecretSizeMin = 136;

  Hash128 acc = {.h1 = len * xxh64::kPrime1, .h2 = 0};
  for (std::size_t i = 32; i < 160; i += 32) {
    acc = Mix32B(acc, input + i - 32, input + i - 16, kSecret + i - 32, seed);
  }
  acc.h1 = Avalanche(acc.h1);
  acc.h2 = Avalanche(acc.h2);
  for (std::size_t i = 160; i <= len; i += 32) {
    acc = Mix32B(acc, input + i - 32, input + i - 16, kSecret + kStartOffset + i - 160, seed);
  }
  acc = Mix32B(acc, input + len - 16, input + len - 32, kSecret + kSecretSizeMin - kLastOffset - 16, 0ULL - seed);
  return Finalize128(acc, len, seed);
}

constexpr Hash128 HashLong128(const char* input, std::size_t len, const char* secret) noexcept {
  std::array<uint64_t, kAccNb> acc = {
      kPrime32_3,     xxh64::kPrime1, xxh64::kPrime2, xxh64::kPrime3,
      xxh64::kPrime4, kPrime32_2,     xxh64::kPrime5, kPrime32_1,
  };

  const std::size_t nb_blocks = (len - 1) / kBlockLen;
  for (std::size_t block = 0; block < nb_blocks; ++block) {
    for (std::size_t stripe = 0; stripe < kStripesPerBlock; ++stripe) {
      Accumulate512(acc, input + (block * kBlockLen) + (stripe * kStripeLen), secret + (stripe * 8));
    }
    ScrambleAcc(acc, secret + kSecretSize - kStripeLen);
  }
  const std::size_t stripes = ((len - 1) - (kBlockLen * nb_blocks)) / kStripeLen;
  for (std::size_t stripe = 0; stripe < stripes; ++stripe) {
    Accumulate512(acc, input + (nb_blocks * kBlockLen) + (stripe * kStripeLen), secret + (stripe * 8));
  }
  Accumulate512(acc, input + len - kStripeLen, secret + kSecretSize - kStripeLen - 7);

  return {
      .h1 = MergeAccs(acc, secret + 11, len * xxh64::kPrime1),
      .h2 = MergeAccs(acc, secret + kSecretSize - 64 - 11, ~(len * xxh64::kPrime2)),
  };
}

}  // namespace xxh3_internal

// Canonical XXH3 64-bit hash (`XXH3_64bits` for seed 0, `XXH3_64bits_withSeed`
// otherwise; default seed 0 as in the reference implementation).
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = 0) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();
  if (len <= 16) {
    return xxh3_internal::Len0To16(ptr, len, seed);
  }
  if (len <= 128) {
    return xxh3_internal::Len17To128(ptr, len, seed);
  }
  if (len <= xxh3_internal::kMidSizeMax) {
    return xxh3_internal::Len129To240(ptr, len, seed);
  }
  if (seed == 0) {
    return xxh3_internal::HashLong(ptr, len, xxh3_internal::kSecret);
  }
  const std::array<char, xxh3_internal::kSecretSize> secret = xxh3_internal::InitCustomSecret(seed);
  return xxh3_internal::HashLong(ptr, len, secret.data());
}

// Canonical XXH3 128-bit hash (`XXH3_128bits` / `XXH3_128bits_withSeed`;
// default seed 0). `h1` is the canonical low 64 bits, `h2` the high 64 bits.
// For inputs > 240 bytes `h1` equals `GetHash64` by construction.
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = 0) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();
  if (len <= 16) {
    return xxh3_internal::Len0To16_128(ptr, len, seed);
  }
  if (len <= 128) {
    return xxh3_internal::Len17To128_128(ptr, len, seed);
  }
  if (len <= xxh3_internal::kMidSizeMax) {
    return xxh3_internal::Len129To240_128(ptr, len, seed);
  }
  if (seed == 0) {
    return xxh3_internal::HashLong128(ptr, len, xxh3_internal::kSecret);
  }
  const std::array<char, xxh3_internal::kSecretSize> secret = xxh3_internal::InitCustomSecret(seed);
  return xxh3_internal::HashLong128(ptr, len, secret.data());
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Note that
// the canonical XXH3 default seed is 0, not `kDefaultSeed`.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::xxh3::GetHash64(data, seed);
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = 0) noexcept {
    return ::mbo::hash::xxh3::GetHash128(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-avoid-c-arrays,*-array-to-pointer-decay,*-constant-array-index,*-easily-swappable-parameters)

}  // namespace mbo::hash::xxh3

#endif  // MBO_HASH_HASH_XXH3_H_
