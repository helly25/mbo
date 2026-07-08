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

#ifndef MBO_HASH_HASH_DUMBO_H_
#define MBO_HASH_HASH_DUMBO_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

namespace mbo::hash::dumbo {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

// `dumbo` is the small/simple member of the -umbo family: a single-accumulator
// 64-bit MUM hash. It shares mumbo's mixing primitive -- the widening
// 64x64->128 multiply folded to 64 bits (`hash_internal::Mul128Fold64`, "MUM")
// -- but stays deliberately minimal: ONE 64-bit state, ONE 8-byte word folded
// per step, no small-key switch, no parallel lanes, no streaming, no 128-bit
// form, and the stock MurmurHash3 `fmix64` finalizer rather than mumbo's
// two-multiply one. That makes it the readable, compact MUM hash next to the
// throughput-tuned mumbo. constexpr; weakly seeded (the seed folds into the
// initial state). Values are NOT stable across library versions and are not
// cryptographic.
//
// Each step multiplies the input word against the running state (both operands
// XORed with a secret first), so the product is quadratic in the data: a
// constant multiplier is linear and collides badly (SMHasher3 collision and
// distribution failures) -- the state-dependent multiply is what earns the
// quality. The constants are nothing-up-my-sleeve numbers so there is no room
// for a hidden weakness: `kInit` is the golden-ratio reciprocal (2^64 / phi,
// the canonical multiplicative-hash constant, also used by
// hash_internal::Hash128To64); `kWord` and `kState` are the 64-bit fractional
// parts of the square roots of 3 and 5 (SHA-512 initial hash values, FIPS
// 180-4, the same source as mumbo's secret bank).
namespace dumbo_internal {

inline constexpr uint64_t kInit = 0x9E3779B97F4A7C15ULL;
inline constexpr uint64_t kWord = 0xBB67AE8584CAA73BULL;
inline constexpr uint64_t kState = 0x3C6EF372FE94F82BULL;

// Folds one 8-byte word into the state with a single widening MUM: both
// operands are state/data dependent, so the mixing is nonlinear.
constexpr uint64_t MumStep(uint64_t state, uint64_t word) noexcept {
  return hash_internal::Mul128Fold64(word ^ kWord, state ^ kState);
}

}  // namespace dumbo_internal

inline constexpr uint64_t GetDumboHash(std::string_view data, uint64_t seed) noexcept {
  const char* ptr = data.data();
  const char* const end = ptr + data.size();
  // The seed folds into the initial state and then rides the whole MUM chain --
  // weak seeding, but free. The length is folded in at the *end* (below), after
  // every input bit is diffused, rather than at init where it would share raw
  // low bits with the first word.
  uint64_t hash = seed ^ dumbo_internal::kInit;
  // 8 bytes per step. The loop bound is the end pointer rather than a
  // decremented counter. Marked likely: keys longer than a single 8-byte word
  // are the common case; shorter keys skip straight to the overlapping tail
  // read below.
  while (end - ptr > 8) [[likely]] {
    hash = dumbo_internal::MumStep(hash, hash_internal::Load64(ptr));
    ptr += 8;
  }
  // The final 1..8 bytes (also the whole key when it is <= 8) read as one
  // overlapping load -- no tail loop, no out-of-bounds access.
  hash = dumbo_internal::MumStep(hash, hash_internal::LoadTail(ptr, static_cast<std::size_t>(end - ptr)));
  // Fold the length now that every input bit is diffused, then avalanche.
  hash ^= data.size();
  return hash_internal::Fmix64(hash);
}

inline constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
  return GetDumboHash(data, seed);
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Weakly
// seeded: the seed folds into the initial state and rides the MUM chain.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return GetDumboHash(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::dumbo

#endif  // MBO_HASH_HASH_DUMBO_H_
