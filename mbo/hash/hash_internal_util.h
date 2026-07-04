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

#ifndef MBO_HASH_HASH_INTERNAL_UTIL_H_
#define MBO_HASH_HASH_INTERNAL_UTIL_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "mbo/hash/hash_types.h"

namespace mbo::hash::hash_internal {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

// Final mixer (MurmurHash3 `fmix64`) that spreads entropy across all 64 bits.
constexpr uint64_t Fmix64(uint64_t val) noexcept {
  val ^= val >> 33U;
  val *= 0xff51afd7ed558ccdULL;
  val ^= val >> 33U;
  val *= 0xc4ceb9fe1a85ec53ULL;
  val ^= val >> 33U;
  return val;
}

// Loads 8 bytes as a **little-endian** `uint64_t`, endian-independently, so hash
// values match across platforms.
//
// At run time on little-endian targets this is a plain `memcpy`, which every
// compiler folds into a single load. The byte-assembly loop remains for constant
// evaluation (where `memcpy` is not allowed) and for big-endian targets. Clang
// folds the loop into a single load as well, but gcc does NOT (verified on
// gcc 13: 8x movzx+or, ~3x slower hashing) -- hence the explicit `memcpy` path.
// Both paths produce identical values; the ConstexprMatchesRuntime test guards
// this equality.
constexpr uint64_t Load64(const char* ptr) noexcept {
  if (!std::is_constant_evaluated()) {
    if constexpr (std::endian::native == std::endian::little) {
      uint64_t result = 0;
      std::memcpy(&result, ptr, 8);
      return result;
    }
  }
  uint64_t result = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    result |= static_cast<uint64_t>(static_cast<uint8_t>(ptr[i])) << (i * 8U);
  }
  return result;
}

// Loads 4 bytes as a **little-endian** `uint32_t` (same rationale as `Load64`).
constexpr uint32_t Load32(const char* ptr) noexcept {
  if (!std::is_constant_evaluated()) {
    if constexpr (std::endian::native == std::endian::little) {
      uint32_t result = 0;
      std::memcpy(&result, ptr, 4);
      return result;
    }
  }
  uint32_t result = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    result |= static_cast<uint32_t>(static_cast<uint8_t>(ptr[i])) << (i * 8U);
  }
  return result;
}

// Loads the 1..8 remaining tail bytes as a little-endian `uint64_t` without any
// out-of-bounds reads.
//
// The runtime path for >= 4 bytes uses two overlapping 4-byte loads instead of a
// per-byte loop: `lo` covers bytes [0..3], `hi << ((n-4)*8)` covers bytes
// [n-4..n-1], and in the overlap both operands carry the same byte at the same
// bit position, so the OR reproduces the exact little-endian value (the
// canonical known-answer tests verify path equality).
constexpr uint64_t LoadTail(const char* ptr, std::size_t remaining) noexcept {
  if (!std::is_constant_evaluated()) {
    if constexpr (std::endian::native == std::endian::little) {
      if (remaining >= 4) {
        const uint64_t low = Load32(ptr);
        const uint64_t high = Load32(ptr + remaining - 4);
        return low | (high << ((remaining - 4) * 8U));
      }
    }
  }
  uint64_t result = 0;
  for (std::size_t i = 0; i < remaining; ++i) {
    result |= static_cast<uint64_t>(static_cast<uint8_t>(ptr[i])) << (i * 8U);
  }
  return result;
}

// Folds the two 64-bit lanes into one (offsetting `h2` so equal lanes do not
// cancel) and finalizes the result.
constexpr uint64_t Hash128To64(Hash128 hash) noexcept {
  const uint64_t combined = hash.h1 ^ (hash.h2 + 0x9e3779b97f4a7c15ULL);
  return Fmix64(combined);
}

// Full 64x64->128 multiply; returns the low half in `h1` and the high half in
// `h2`. Uses `__uint128_t` where the compiler provides it (gcc/clang,
// constexpr-legal); the portable 32-bit schoolbook multiply otherwise.
constexpr Hash128 Mult128(uint64_t lhs, uint64_t rhs) noexcept {
#if defined(__SIZEOF_INT128__)
  const auto product = static_cast<unsigned __int128>(lhs) * static_cast<unsigned __int128>(rhs);
  return {.h1 = static_cast<uint64_t>(product), .h2 = static_cast<uint64_t>(product >> 64U)};
#else   // defined(__SIZEOF_INT128__)
  const uint64_t lo_lo = (lhs & 0xFFFFFFFFULL) * (rhs & 0xFFFFFFFFULL);
  const uint64_t hi_lo = (lhs >> 32U) * (rhs & 0xFFFFFFFFULL);
  const uint64_t lo_hi = (lhs & 0xFFFFFFFFULL) * (rhs >> 32U);
  const uint64_t hi_hi = (lhs >> 32U) * (rhs >> 32U);
  const uint64_t cross = (lo_lo >> 32U) + (hi_lo & 0xFFFFFFFFULL) + lo_hi;
  return {
      .h1 = (cross << 32U) | (lo_lo & 0xFFFFFFFFULL),
      .h2 = (hi_lo >> 32U) + (cross >> 32U) + hi_hi,
  };
#endif  // defined(__SIZEOF_INT128__)
}

// Full 64x64->128 multiply folded to 64 bits by XORing the halves (the core
// mixer of the xxh3/wyhash algorithm family). Uses `__uint128_t` where the
// compiler provides it (gcc/clang, constexpr-legal); the portable 32-bit
// schoolbook multiply otherwise.
constexpr uint64_t Mul128Fold64(uint64_t lhs, uint64_t rhs) noexcept {
#if defined(__SIZEOF_INT128__)
  const auto product = static_cast<unsigned __int128>(lhs) * static_cast<unsigned __int128>(rhs);
  return static_cast<uint64_t>(product) ^ static_cast<uint64_t>(product >> 64U);
#else   // defined(__SIZEOF_INT128__)
  const uint64_t lo_lo = (lhs & 0xFFFFFFFFULL) * (rhs & 0xFFFFFFFFULL);
  const uint64_t hi_lo = (lhs >> 32U) * (rhs & 0xFFFFFFFFULL);
  const uint64_t lo_hi = (lhs & 0xFFFFFFFFULL) * (rhs >> 32U);
  const uint64_t hi_hi = (lhs >> 32U) * (rhs >> 32U);
  const uint64_t cross = (lo_lo >> 32U) + (hi_lo & 0xFFFFFFFFULL) + lo_hi;
  const uint64_t upper = (hi_lo >> 32U) + (cross >> 32U) + hi_hi;
  const uint64_t lower = (cross << 32U) | (lo_lo & 0xFFFFFFFFULL);
  return lower ^ upper;
#endif  // defined(__SIZEOF_INT128__)
}

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::hash_internal

#endif  // MBO_HASH_HASH_INTERNAL_UTIL_H_
