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

#include <cstddef>
#include <cstdint>

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

// Loads 8 bytes as a **little-endian** `uint64_t`. Assembling the word from bytes
// (rather than `std::bit_cast`) makes the result endian-independent, so hashes
// match across platforms; it is constexpr-safe and compilers fold it to a single
// load on little-endian targets (a load + byte-swap on big-endian).
constexpr uint64_t Load64(const char* ptr) noexcept {
  uint64_t result = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    result |= static_cast<uint64_t>(static_cast<uint8_t>(ptr[i])) << (i * 8U);
  }
  return result;
}

// Loads 4 bytes as a **little-endian** `uint32_t` (same rationale as `Load64`).
constexpr uint32_t Load32(const char* ptr) noexcept {
  uint32_t result = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    result |= static_cast<uint32_t>(static_cast<uint8_t>(ptr[i])) << (i * 8U);
  }
  return result;
}

// Loads the 1..8 remaining tail bytes as a little-endian `uint64_t` without any
// out-of-bounds reads.
constexpr uint64_t LoadTail(const char* ptr, std::size_t remaining) noexcept {
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

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::hash_internal

#endif  // MBO_HASH_HASH_INTERNAL_UTIL_H_
