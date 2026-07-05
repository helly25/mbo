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

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

// The `mbo::hash::mh` ("mbo hash") implementation -- this library's own
// algorithm. No longer the default behind `mbo::hash::GetHash*` (see
// README.md's SMHasher3 section); fast in hot-loop throughput benchmarks, constexpr-safe, and
// streaming-capable.
//
// WORK IN PROGRESS: `mh` is under active development. It may be made faster,
// have its rounds strengthened (values would change, as the stability
// contract allows), be replaced entirely, or be renamed -- do not depend on
// the `mh` namespace or its values beyond a single build.

// Codegen-only hint (does not affect constant evaluation); see GetHash64Large.
// NOLINTBEGIN(*-macro-usage)
#if defined(__clang__) || defined(__GNUC__)
# define MBO_HASH_NOINLINE [[gnu::noinline]]
#else
# define MBO_HASH_NOINLINE
#endif
// NOLINTEND(*-macro-usage)

namespace mbo::hash::mh {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

// Full-width odd multipliers (golden-ratio / mix constants); distinct per lane
// and per stripe accumulator so the lanes stay decorrelated. Odd => invertible
// mod 2^64 (no bit loss); full width => fast diffusion into the high bits.
inline constexpr uint64_t kMul1 = 0x9E3779B97F4A7C15ULL;
inline constexpr uint64_t kMul2 = 0xC2B2AE3D27D4EB4FULL;
inline constexpr uint64_t kMul3 = 0x165667B19E3779F9ULL;
inline constexpr uint64_t kMul4 = 0xFF51AFD7ED558CCDULL;

// Input premultiplier: every absorbed block is first multiplied by this
// full-width odd constant so a single-bit input becomes multi-bit *before* it
// meets the accumulator. Without it, an input bit that lands on bit 63 after
// the rotation survives the lane multiply as a single-bit state difference and
// cancels against the matching bit of the next block (structural collisions
// among sparse keys, e.g. bit32@block_k == bit63@block_{k+1}; caught by the
// StructuredKeysAreDistinct test). The residual single-bit case (input bit 63)
// is diffused by the rotate-then-multiply of the lane itself.
inline constexpr uint64_t kMulIn = 0x27D4EB2F165667C5ULL;

// Inputs of at most this many bytes use the cheaper single-lane GetHash64 path
// (see below). Tuned via //mbo/hash:hash_benchmark.
inline constexpr std::size_t kSmallInputCutoff = 32;

// Inputs of at least this many bytes use the 4-accumulator stripe loop in
// GetHash128 (see below). Tuned via //mbo/hash:hash_benchmark.
inline constexpr std::size_t kStripeInputCutoff = 64;

// Default seed; alias of the shared `mbo::hash::kDefaultSeed`.
inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

// 128-bit variant: dual-lane rotate-multiply state, with a 4-accumulator stripe
// loop for large inputs.
//
// Core round: the lane absorbs the block (xor in one lane, add in the other),
// is rotated *before* the multiply so the block's low bits reach the high bits
// within a single round (a plain multiply-then-xor diffuses far too slowly),
// then multiplied by a full-width odd constant.
//
// The round's xor/rotl/mul dependency chain (~6 cycles) caps a 2-lane loop at
// 8 bytes per chain. Inputs >= kStripeInputCutoff therefore run 4 independent
// accumulators over 32-byte stripes (xxh64-style instruction parallelism,
// ~2.5-3x throughput) and merge them into the two lanes. The finalizer
// cross-mixes the lanes (murmur-style) so every output bit depends on every
// accumulator -- and thus on every input byte.
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();
  std::size_t remaining = len;

  // Finalize the seed before it derives any lane: structured seeds (e.g.
  // seeds that mirror input blocks) must not correlate with the input.
  // SMHasher3's SeedBlockLen/SeedBlockOffset/SeedBIC families fail without
  // this (see README.md).
  seed = hash_internal::Fmix64(seed);
  uint64_t hash1 = seed;
  uint64_t hash2 = seed ^ 0xAA55AA55AA55AA55ULL;

  if (remaining >= kStripeInputCutoff) {
    uint64_t acc1 = hash1 + kMul1;
    uint64_t acc2 = hash2 + kMul2;
    uint64_t acc3 = (hash1 * kMul3) + 1;
    uint64_t acc4 = std::rotl(hash1, 32) ^ kMul4;
    while (remaining >= 32) {
      acc1 = std::rotl(acc1 ^ (hash_internal::Load64(ptr) * kMulIn), 31) * kMul1;
      acc2 = std::rotl(acc2 + (hash_internal::Load64(ptr + 8) * kMulIn), 27) * kMul2;
      acc3 = std::rotl(acc3 ^ (hash_internal::Load64(ptr + 16) * kMulIn), 29) * kMul3;
      acc4 = std::rotl(acc4 + (hash_internal::Load64(ptr + 24) * kMulIn), 25) * kMul4;
      ptr += 32;
      remaining -= 32;
    }
    hash1 = ((std::rotl(acc1, 1) + std::rotl(acc3, 7)) * kMul1) + acc2;
    hash2 = ((std::rotl(acc2, 12) + std::rotl(acc4, 18)) * kMul2) + acc3;
  }

  while (remaining >= 8) {
    const uint64_t block = hash_internal::Load64(ptr) * kMulIn;
    hash1 = std::rotl(hash1 ^ block, 31) * kMul1;
    hash2 = std::rotl(hash2 + block, 27) * kMul2;
    ptr += 8;
    remaining -= 8;
  }
  if (remaining > 0) {
    // No premul needed: a <8-byte tail can never carry bits 62/63, the only
    // positions where a single-bit state difference survives a lane multiply.
    const uint64_t tail = hash_internal::LoadTail(ptr, remaining);
    hash1 = std::rotl(hash1 ^ tail, 31) * kMul1;
    hash2 = std::rotl(hash2 + tail, 27) * kMul2;
  }

  hash1 ^= len;
  hash2 += len;
  hash1 += hash2;  // Cross-mix so both output lanes depend on all accumulators.
  hash2 += hash1;

  return {
      .h1 = hash_internal::Fmix64(hash1),
      .h2 = hash_internal::Fmix64(hash2),
  };
}

// 64-bit variant, two tiers (each tuned via //mbo/hash:hash_benchmark; the
// tiers deliberately produce different values -- GetHash64 is not required to
// equal a fold of GetHash128):
//
// - Small (<= kSmallInputCutoff): single-lane rotate-multiply loop with one
//   finalize; the dual-lane state plus its 128->64 fold would add fixed
//   overhead that dominates at these sizes. Sub-8-byte inputs run zero loop
//   iterations plus one `LoadTail` round, which the overlap loads keep cheap.
// - Large: fold of the full 128-bit state (which stripes at 4 accumulators).
//   Kept out-of-line so GetHash64 stays small enough to inline at call sites
//   (inlining the stripe loop measurably de-optimized the small path).
namespace mh_internal {

MBO_HASH_NOINLINE constexpr uint64_t GetHash64Large(std::string_view str, uint64_t seed) noexcept {
  return hash_internal::Hash128To64(GetHash128(str, seed));
}

}  // namespace mh_internal

constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const std::size_t len = str.size();
  if (len > kSmallInputCutoff) [[unlikely]] {
    return mh_internal::GetHash64Large(str, seed);
  }

  const char* ptr = str.data();
  // Seed finalization: see GetHash128.
  uint64_t hash = hash_internal::Fmix64(seed) ^ (static_cast<uint64_t>(len) * kMul2);
  std::size_t pos = 0;
  for (; pos + 8 <= len; pos += 8) {
    hash ^= hash_internal::Load64(ptr + pos) * kMulIn;
    hash = std::rotl(hash, 31) * kMul1;
  }
  if (pos < len) {
    // No premul needed (see GetHash128's tail comment).
    hash ^= hash_internal::LoadTail(ptr + pos, len - pos);
    hash = std::rotl(hash, 27) * kMul2;
  }
  return hash_internal::Fmix64(hash);
}

// Streaming interface on the algorithm struct (see `mbo::hash::HasStreaming`).
//
// Chunked updates produce exactly the one-shot GetHash64 value. The state
// buffers at most 64 bytes: the one-shot dispatch depends on the total length
// (single-lane <= kSmallInputCutoff; 4-accumulator stripes from
// kStripeInputCutoff), so processing starts only once 64 bytes have been seen
// -- from then on every full 32-byte stripe is consumed exactly as the
// one-shot stripe loop would, and Finalize replays the remaining (< 32
// buffered) bytes through the same 2-lane block/tail/finalizer code.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mh::GetHash64(data, seed);
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mh::GetHash128(data, seed);
  }

  struct StreamState {
    std::array<uint64_t, 4> acc = {};
    std::array<char, 64> buffer = {};
    std::size_t buffered = 0;
    std::size_t total = 0;
    uint64_t seed = 0;
    bool striping = false;
  };

  static constexpr StreamState StreamInit(uint64_t seed) noexcept {
    // Stores the finalized seed (see GetHash128's seed finalization).
    return {.acc = {}, .buffer = {}, .buffered = 0, .total = 0, .seed = hash_internal::Fmix64(seed), .striping = false};
  }

  static constexpr void StreamUpdate(StreamState& stream, std::string_view data) noexcept {
    stream.total += data.size();
    const char* ptr = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
      while (stream.buffered < 64 && remaining > 0) {
        stream.buffer[stream.buffered++] = *ptr++;  // NOLINT(*-constant-array-index,*-pointer-arithmetic)
        --remaining;
      }
      if (stream.buffered < 64) {
        return;  // Everything buffered; dispatch not decided yet or stripes drained.
      }
      if (!stream.striping) {  // Total input is >= 64: enter the stripe tier.
        const uint64_t hash1 = stream.seed;
        stream.acc = {
            hash1 + kMul1,
            (stream.seed ^ 0xAA55AA55AA55AA55ULL) + kMul2,
            (hash1 * kMul3) + 1,
            std::rotl(hash1, 32) ^ kMul4,
        };
        stream.striping = true;
      }
      std::size_t consumed = 0;
      while (stream.buffered - consumed >= 32) {
        const char* block = stream.buffer.data() + consumed;
        stream.acc[0] = std::rotl(stream.acc[0] ^ (hash_internal::Load64(block) * kMulIn), 31) * kMul1;
        stream.acc[1] = std::rotl(stream.acc[1] + (hash_internal::Load64(block + 8) * kMulIn), 27) * kMul2;
        stream.acc[2] = std::rotl(stream.acc[2] ^ (hash_internal::Load64(block + 16) * kMulIn), 29) * kMul3;
        stream.acc[3] = std::rotl(stream.acc[3] + (hash_internal::Load64(block + 24) * kMulIn), 25) * kMul4;
        consumed += 32;
      }
      for (std::size_t i = 0; consumed + i < stream.buffered; ++i) {  // Keep the < 32-byte rest.
        stream.buffer[i] = stream.buffer[consumed + i];               // NOLINT(*-constant-array-index)
      }
      stream.buffered -= consumed;
    }
  }

  static constexpr uint64_t StreamFinalize(StreamState stream) noexcept {
    const std::string_view rest(stream.buffer.data(), stream.buffered);
    if (stream.total <= kSmallInputCutoff) {
      // Whole input is still buffered; replay the small path (stream.seed is
      // already finalized, so inline the body rather than re-finalizing).
      const char* small_ptr = rest.data();
      uint64_t hash = stream.seed ^ (static_cast<uint64_t>(stream.total) * kMul2);
      std::size_t pos = 0;
      for (; pos + 8 <= rest.size(); pos += 8) {
        hash ^= hash_internal::Load64(small_ptr + pos) * kMulIn;
        hash = std::rotl(hash, 31) * kMul1;
      }
      if (pos < rest.size()) {
        hash ^= hash_internal::LoadTail(small_ptr + pos, rest.size() - pos);
        hash = std::rotl(hash, 27) * kMul2;
      }
      return hash_internal::Fmix64(hash);
    }
    uint64_t hash1 = stream.seed;
    uint64_t hash2 = stream.seed ^ 0xAA55AA55AA55AA55ULL;
    const char* ptr = rest.data();
    std::size_t remaining = rest.size();
    if (stream.striping) {  // The one-shot stripe loop also eats a final full stripe.
      while (remaining >= 32) {
        stream.acc[0] = std::rotl(stream.acc[0] ^ (hash_internal::Load64(ptr) * kMulIn), 31) * kMul1;
        stream.acc[1] = std::rotl(stream.acc[1] + (hash_internal::Load64(ptr + 8) * kMulIn), 27) * kMul2;
        stream.acc[2] = std::rotl(stream.acc[2] ^ (hash_internal::Load64(ptr + 16) * kMulIn), 29) * kMul3;
        stream.acc[3] = std::rotl(stream.acc[3] + (hash_internal::Load64(ptr + 24) * kMulIn), 25) * kMul4;
        ptr += 32;
        remaining -= 32;
      }
      hash1 = ((std::rotl(stream.acc[0], 1) + std::rotl(stream.acc[2], 7)) * kMul1) + stream.acc[1];
      hash2 = ((std::rotl(stream.acc[1], 12) + std::rotl(stream.acc[3], 18)) * kMul2) + stream.acc[2];
    }
    while (remaining >= 8) {
      const uint64_t block = hash_internal::Load64(ptr) * kMulIn;
      hash1 = std::rotl(hash1 ^ block, 31) * kMul1;
      hash2 = std::rotl(hash2 + block, 27) * kMul2;
      ptr += 8;
      remaining -= 8;
    }
    if (remaining > 0) {
      const uint64_t tail = hash_internal::LoadTail(ptr, remaining);
      hash1 = std::rotl(hash1 ^ tail, 31) * kMul1;
      hash2 = std::rotl(hash2 + tail, 27) * kMul2;
    }
    hash1 ^= stream.total;
    hash2 += stream.total;
    hash1 += hash2;
    hash2 += hash1;
    hash1 = hash_internal::Fmix64(hash1);
    hash2 = hash_internal::Fmix64(hash2);
    return hash_internal::Hash128To64({.h1 = hash1, .h2 = hash2});
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::mh

#endif  // MBO_HASH_HASH_MH_H_
