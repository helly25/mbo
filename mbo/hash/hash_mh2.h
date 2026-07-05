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

#ifndef MBO_HASH_HASH_MH2_H_
#define MBO_HASH_HASH_MH2_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

// mh2 -- EXPERIMENTAL successor of `mh` (no value-stability guarantee
// whatsoever; may change, be renamed, or become the default between
// commits). SMHasher3: PASS 188/188 in BOTH the 64-bit and the native
// 128-bit form - the only clean 128-bit result we have measured (see
// README.md for quality and performance data).
//
// Design: everything is built on the widening 64x64->128 multiply folded to
// 64 bits (`hash_internal::Mul128Fold64`, the wyhash-family "MUM" mixer) --
// one multiply absorbs 16 input bytes and diffuses full-width in both
// directions, which is simultaneously the fastest and the strongest known
// scalar round (rapidhash passes SMHasher3 188/188 on it). Structure:
//
// - <= 16 bytes: a fully unrolled `switch` -- every length 0..8 has its own
//   one-go terminal action (no byte loops), 9..16 use two overlapping loads.
// - 17..64 bytes: one sequential MUM chain, 16 bytes per step; the final <=16
//   bytes are read as two loads overlapping the end (no tail loop).
// - >= 128 bytes: eight independent MUM chains over a 128-byte fetch window
//   (per-chain secret constants keep identical stripes from cancelling on
//   the XOR merge), then the sequential chain for the rest.
// - 128-bit: two chains with different secrets and swapped operand roles run
//   over the same input (a shared 4-chain bulk tier feeds both lanes through
//   different nonlinear merges); both halves therefore cover every input
//   byte and decorrelate through the secrets (measured by SMHasher3, see
//   README.md).
//
// The secret constants are nothing-up-my-sleeve numbers: the 64-bit
// fractional parts of the square roots of the first sixteen primes (the
// SHA-512 and SHA-384 initial hash values, FIPS 180-4 sections 5.3.5/5.3.4).
namespace mbo::hash::mh2 {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters)

inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

namespace mh2_internal {

using hash_internal::Load32;
using hash_internal::Load64;
using hash_internal::Mul128Fold64;
using hash_internal::Mult128;

inline constexpr std::array<uint64_t, 16> kSecret = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179,
    0xCBBB9D5DC1059ED8, 0x629A292A367CD507, 0x9159015A3070DD17, 0x152FECD8F70E5939,
    0x67332667FFC00B31, 0x8EB44A8768581511, 0xDB0C2E0D64F98FA7, 0x47B5481DBEFA4FA4,
};

// Loads the (1..16 byte) small-key input into two words. Every length 0..8
// is its own unrolled terminal action (single expressions, no loops); 9..16
// use two overlapping 8-byte loads. `data` must have `len` readable bytes.
struct SmallInput {
  uint64_t a = 0;
  uint64_t b = 0;
};

// Every length loads the key into BOTH operands (overlapping reads), so the
// first widening product is quadratic in the data - with data in only one
// operand, permuted keys correlate pairwise through the shared seed operand
// (v2's SMHasher3 Permutation failures).
constexpr SmallInput LoadSmall(const char* ptr, std::size_t len) noexcept {
  // NOLINTNEXTLINE(readability-identifier-length): byte-widening helper.
  const auto u8 = [](char chr) constexpr { return static_cast<uint64_t>(static_cast<uint8_t>(chr)); };
  switch (len) {
    case 0: return {.a = 0, .b = 0};
    case 1: {
      const uint64_t val = u8(ptr[0]);
      return {.a = (val << 45U) | val, .b = (val << 45U) | val};
    }
    case 2: {
      const uint64_t val = (u8(ptr[0]) << 45U) | (u8(ptr[1]) << 8U) | u8(ptr[0]);
      return {.a = val, .b = val};
    }
    case 3: {
      const uint64_t val = (u8(ptr[0]) << 45U) | (u8(ptr[1]) << 8U) | u8(ptr[2]);
      return {.a = val, .b = val};
    }
    case 4: {
      const uint64_t val = Load32(ptr);
      return {.a = val, .b = val};
    }
    case 5: return {.a = Load32(ptr), .b = Load32(ptr + 1)};
    case 6: return {.a = Load32(ptr), .b = Load32(ptr + 2)};
    case 7: return {.a = Load32(ptr), .b = Load32(ptr + 3)};
    case 8: {
      const uint64_t val = Load64(ptr);
      return {.a = val, .b = val};
    }
    default:  // 9..16: two loads overlapping the end.
      return {.a = Load64(ptr), .b = Load64(ptr + len - 8)};
  }
}

// The shared two-multiply finalizer (length and seed folded in). Keeps BOTH
// halves of the first widening product and mixes them against each other so
// every final-multiply operand carries input/seed entropy - folding early
// and multiplying against a constant leaks quasi-linear deltas on sparse
// keys (v1's SMHasher3 Zeroes/Permutation/TwoBytes failures).
constexpr uint64_t Finish(uint64_t val_a, uint64_t val_b, uint64_t seed, uint64_t len) noexcept {
  const Hash128 product = Mult128(val_a ^ kSecret[2], val_b ^ seed);
  return Mul128Fold64(product.h1 ^ kSecret[3] ^ len, product.h2 ^ kSecret[1]);
}

// The second-lane finalizer of the 128-bit form (distinct secrets, swapped
// operand roles).
constexpr uint64_t Finish2(uint64_t val_a, uint64_t val_b, uint64_t seed, uint64_t len) noexcept {
  const Hash128 product = Mult128(val_b ^ kSecret[10], val_a ^ seed);
  return Mul128Fold64(product.h1 ^ kSecret[11] ^ len, product.h2 ^ kSecret[9]);
}

}  // namespace mh2_internal

using hash_internal::Load64;
using hash_internal::Mul128Fold64;
using mh2_internal::kSecret;

// NOLINTNEXTLINE(readability-function-cognitive-complexity): tiered by design.
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();

  // Absorb + finalize the seed (structured seeds must not correlate with
  // input; see README.md's Seed* families), then fold in the length so it
  // modulates every product multiplicatively (not just the final xor).
  seed = Mul128Fold64(seed ^ kSecret[0], kSecret[1]) ^ len;

  if (len <= 16) {
    const mh2_internal::SmallInput input = mh2_internal::LoadSmall(ptr, len);
    return mh2_internal::Finish(input.a, input.b, seed, len);
  }

  std::size_t remaining = len;
  if (len >= 128) {
    // Eight independent chains over a 128-byte fetch window (instruction-level
    // parallelism); per-chain secrets prevent identical-stripe cancellation.
    std::array<uint64_t, 8> chain = {
        seed ^ kSecret[8],  seed ^ kSecret[9],  seed ^ kSecret[10], seed ^ kSecret[11],
        seed ^ kSecret[12], seed ^ kSecret[13], seed ^ kSecret[14], seed ^ kSecret[15],
    };
    while (remaining >= 128) {
      for (std::size_t i = 0; i < 8; ++i) {  // NOLINT(*-constant-array-index)
        chain[i] = Mul128Fold64(Load64(ptr + (16 * i)) ^ kSecret[4 + i], Load64(ptr + (16 * i) + 8) ^ chain[i]);
      }
      ptr += 128;
      remaining -= 128;
    }
    seed = chain[0] ^ chain[1] ^ chain[2] ^ chain[3] ^ chain[4] ^ chain[5] ^ chain[6] ^ chain[7];
  }
  while (remaining > 16) {
    seed = Mul128Fold64(Load64(ptr) ^ kSecret[1], Load64(ptr + 8) ^ seed);
    ptr += 16;
    remaining -= 16;
  }
  // Final 1..16 bytes: two loads overlapping the end of the key (always
  // in-bounds because len > 16).
  const uint64_t val_a = Load64(ptr + remaining - 16);
  const uint64_t val_b = Load64(ptr + remaining - 8);
  return mh2_internal::Finish(val_a, val_b, seed, len);
}

// Native 128-bit form: two chains with different secrets and swapped operand
// roles cover the same input; see the header comment.
// NOLINTNEXTLINE(readability-function-cognitive-complexity): tiered by design.
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();

  uint64_t seed1 = Mul128Fold64(seed ^ kSecret[0], kSecret[1]) ^ len;
  uint64_t seed2 = Mul128Fold64(seed ^ kSecret[8], kSecret[9]) ^ len;

  uint64_t val_a = 0;
  uint64_t val_b = 0;
  if (len <= 16) {
    const mh2_internal::SmallInput input = mh2_internal::LoadSmall(ptr, len);
    val_a = input.a;
    val_b = input.b;
  } else {
    std::size_t remaining = len;
    if (len >= 64) {
      // Shared 4-chain bulk tier: each lane derives from ALL chains, through
      // different (nonlinear) merges, so both halves cover every input byte.
      uint64_t chain0 = seed1 ^ kSecret[8];
      uint64_t chain1 = seed1 ^ kSecret[9];
      uint64_t chain2 = seed2 ^ kSecret[10];
      uint64_t chain3 = seed2 ^ kSecret[11];
      while (remaining >= 64) {
        chain0 = Mul128Fold64(Load64(ptr) ^ kSecret[4], Load64(ptr + 8) ^ chain0);
        chain1 = Mul128Fold64(Load64(ptr + 16) ^ kSecret[5], Load64(ptr + 24) ^ chain1);
        chain2 = Mul128Fold64(Load64(ptr + 32) ^ kSecret[6], Load64(ptr + 40) ^ chain2);
        chain3 = Mul128Fold64(Load64(ptr + 48) ^ kSecret[7], Load64(ptr + 56) ^ chain3);
        ptr += 64;
        remaining -= 64;
      }
      seed1 = chain0 ^ chain1 ^ chain2 ^ chain3;
      seed2 = Mul128Fold64(chain0 ^ kSecret[12], chain2 ^ kSecret[13])
              ^ Mul128Fold64(chain1 ^ kSecret[14], chain3 ^ kSecret[15]);
    }
    while (remaining > 16) {
      const uint64_t block0 = Load64(ptr);
      const uint64_t block1 = Load64(ptr + 8);
      seed1 = Mul128Fold64(block0 ^ kSecret[1], block1 ^ seed1);
      seed2 = Mul128Fold64(block1 ^ kSecret[9], block0 ^ seed2);  // Swapped roles, different secret.
      ptr += 16;
      remaining -= 16;
    }
    val_a = Load64(ptr + remaining - 16);
    val_b = Load64(ptr + remaining - 8);
  }
  return {
      .h1 = mh2_internal::Finish(val_a, val_b, seed1, len),
      .h2 = mh2_internal::Finish2(val_a, val_b, seed2, len),
  };
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h).
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mh2::GetHash64(data, seed);
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mh2::GetHash128(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters)

}  // namespace mbo::hash::mh2

#endif  // MBO_HASH_HASH_MH2_H_
