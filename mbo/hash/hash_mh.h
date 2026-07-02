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

#ifndef MBO_HASH_HASH_MH_H_
#define MBO_HASH_HASH_MH_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

// The `mbo::hash::mh` ("mbo hash") implementation -- the current default behind
// `mbo::hash::GetHash*`.
namespace mbo::hash::mh {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

// Full-width odd multipliers (golden-ratio / mix constants); distinct per lane so
// the two 128-bit lanes stay decorrelated. Odd => invertible mod 2^64 (no bit
// loss); full width => fast diffusion into the high bits.
inline constexpr uint64_t kMul1 = 0x9E3779B97F4A7C15ULL;
inline constexpr uint64_t kMul2 = 0xC2B2AE3D27D4EB4FULL;

// Inputs of at most this many bytes use the cheaper single-lane GetHash64 path
// (see below). Tuned via //mbo/hash:hash_benchmark.
inline constexpr std::size_t kSmallInputCutoff = 32;

// Default seed (djb2's initial value). Shared so `mbo::hash::GetHash` folds the
// same value the bare `GetHash*` functions use by default.
inline constexpr uint64_t kDefaultSeed = 5'381;

// 128-bit variant: dual-lane rotate-multiply state.
//
// Each 8-byte block is folded into both lanes, then the lane is rotated *before*
// the multiply so the block's low bits reach the high bits within a single round
// (a plain multiply-then-xor with a small multiplier diffuses far too slowly).
// The two lanes combine the block differently (xor vs add) and use distinct
// multipliers so they stay decorrelated.
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  Hash128 hash = {.h1 = seed, .h2 = seed ^ 0xAA55AA55AA55AA55ULL};

  const char* ptr = str.data();
  const std::size_t len = str.size();
  const std::size_t blocks = len / 8;

  for (std::size_t i = 0; i < blocks; ++i) {
    const uint64_t block = hash_internal::Load64(ptr);
    hash.h1 = std::rotl(hash.h1 ^ block, 31) * kMul1;
    hash.h2 = std::rotl(hash.h2 + block, 27) * kMul2;
    ptr += 8;
  }

  const std::size_t remaining = len % 8;
  if (remaining > 0) {
    const uint64_t tail = hash_internal::LoadTail(ptr, remaining);
    hash.h1 = std::rotl(hash.h1 ^ tail, 31) * kMul1;
    hash.h2 = std::rotl(hash.h2 + tail, 27) * kMul2;
  }

  hash.h1 ^= len;
  hash.h2 += len;

  return {
      .h1 = hash_internal::Fmix64(hash.h1),
      .h2 = hash_internal::Fmix64(hash.h2),
  };
}

// 64-bit variant.
//
// Small inputs (<= kSmallInputCutoff bytes) take a cheaper single-lane path with
// a single finalize: the 128-bit dual-lane state plus its 128->64 fold add fixed
// overhead that dominates for short keys. Larger inputs fold the full 128-bit
// state, which wins on throughput once the block loop dominates. (The two paths
// deliberately produce different values -- GetHash64 is not required to equal a
// fold of GetHash128.)
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const std::size_t len = str.size();
  if (len > kSmallInputCutoff) {
    return hash_internal::Hash128To64(GetHash128(str, seed));
  }

  const char* ptr = str.data();
  uint64_t hash = seed ^ (static_cast<uint64_t>(len) * kMul2);
  std::size_t pos = 0;
  for (; pos + 8 <= len; pos += 8) {
    hash ^= hash_internal::Load64(ptr + pos);
    hash = std::rotl(hash, 31) * kMul1;
  }
  if (pos < len) {
    hash ^= hash_internal::LoadTail(ptr + pos, len - pos);
    hash = std::rotl(hash, 27) * kMul2;
  }
  return hash_internal::Fmix64(hash);
}

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::mh

#endif  // MBO_HASH_HASH_MH_H_
