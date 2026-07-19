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

#ifndef MBO_HASH_HASH_MUMBO_H_
#define MBO_HASH_HASH_MUMBO_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

// mumbo & jumbo -- this library's own hash family and the defaults behind
// `mbo::hash::GetHash*`: `mumbo` fronts the 64-bit form (plus streaming),
// `jumbo` fronts the native 128-bit form; both `Algorithm` structs are
// complete (each delegates the other width), so either plugs in anywhere.
// The name is MUM + mbo: everything is built on the widening 64x64->128
// multiply folded to 64 bits (`hash_internal::Mul128Fold64`, the "MUM"
// mixer) -- one multiply absorbs 16 input bytes and diffuses full-width in
// both directions.
//
// Quality: SMHasher3 PASS 188/188 in BOTH the 64-bit and the native 128-bit
// form -- the only clean 128-bit result measured on our rig; performance and
// quality data plus the measured design iterations: see README.md. Values are
// NOT stable across library versions (library-wide policy) and not
// cryptographic.
//
// Structure:
// - <= 16 bytes: a fully unrolled `switch` -- every length 0..8 has its own
//   one-go terminal action (no byte loops), 9..16 use two loads overlapping
//   the end. The key loads into BOTH operands of the widening product, so the
//   product is quadratic in the data (a single-operand load correlates
//   permuted keys pairwise through the shared seed operand).
// - 17..127 bytes: one sequential MUM chain, 16 bytes per step; the final
//   <= 16 bytes are read as two loads overlapping the end (no tail loops).
// - >= 128 bytes: eight independent MUM chains over a 128-byte fetch window,
//   manually unrolled and interleaved to eliminate execution stalls. Each
//   starts from a distinct secret so identical stripes cannot cancel on the XOR
//   merge.
// - Finalizer: keeps BOTH halves of a widening product and mixes them against
//   each other, with the length folded into the product operands so it
//   modulates the result multiplicatively (and only at finalize -- which is
//   what makes streaming possible).
// - 128-bit: two lanes with distinct secret banks and swapped operand roles
//   run over the same input; an interleaved 4-chain bulk tier feeds both lanes
//   through different nonlinear merges where each lane covers every input byte.
// - Streaming (64-bit): eagerly consumes full 128-byte blocks, keeps a
//   rolling window of the last 16 bytes for the overlapping tail reads;
//   chunked updates produce exactly the one-shot value.
//
// The secret constants are nothing-up-my-sleeve numbers: the 64-bit
// fractional parts of the square roots of the first sixteen primes (the
// SHA-512 and SHA-384 initial hash values, FIPS 180-4 sections 5.3.5/5.3.4).

#if defined(__GNUC__) || defined(__clang__)
# define MBO_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MBO_FORCE_INLINE __forceinline
#else
# define MBO_FORCE_INLINE inline
#endif

namespace mbo::hash::mumbo {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

namespace mumbo_internal {

using hash_internal::Load32;
using hash_internal::Load64;
using hash_internal::LoadSmall;
using hash_internal::Mul128Fold64;
using hash_internal::Mult128;
using hash_internal::SmallInput;

inline constexpr std::array<uint64_t, 16> kSecret = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179,
    0xCBBB9D5DC1059ED8, 0x629A292A367CD507, 0x9159015A3070DD17, 0x152FECD8F70E5939,
    0x67332667FFC00B31, 0x8EB44A8768581511, 0xDB0C2E0D64F98FA7, 0x47B5481DBEFA4FA4,
};

// The bulk tier's fetch window (eight 16-byte chains).
inline constexpr std::size_t kBulkWindow = 128;

// The shared two-multiply finalizer. Keeps BOTH halves of the first widening
// product and mixes them against each other so every final-multiply operand
// carries input/seed entropy; the length sits in a product operand so it
// modulates the result multiplicatively (zero-entropy keys of different
// lengths must not correlate).
constexpr uint64_t Finish(uint64_t val_a, uint64_t val_b, uint64_t seed, uint64_t len) noexcept {
  const Hash128 product = Mult128(val_a ^ kSecret[2] ^ len, val_b ^ seed);
  return Mul128Fold64(product.h1 ^ kSecret[3] ^ len, product.h2 ^ kSecret[1]);
}

// The second-lane finalizer of the 128-bit form (distinct secrets, swapped
// operand roles).
constexpr uint64_t Finish2(uint64_t val_a, uint64_t val_b, uint64_t seed, uint64_t len) noexcept {
  const Hash128 product = Mult128(val_b ^ kSecret[10] ^ len, val_a ^ seed);
  return Mul128Fold64(product.h1 ^ kSecret[11] ^ len, product.h2 ^ kSecret[9]);
}

// One 128-byte bulk block over the eight chains. Manually unrolled and
// interleaved to maximize instruction-level parallelism (ILP) and prevent
// execution pipeline stalls while retaining identical state output.
MBO_FORCE_INLINE constexpr void BulkBlock(std::array<uint64_t, 8>& chain, const char* ptr) noexcept {
  const uint64_t a0 = Load64(ptr + 0) ^ kSecret[4];
  const uint64_t b0 = Load64(ptr + 8) ^ chain[0];
  const uint64_t a1 = Load64(ptr + 16) ^ kSecret[5];
  const uint64_t b1 = Load64(ptr + 24) ^ chain[1];
  const uint64_t a2 = Load64(ptr + 32) ^ kSecret[6];
  const uint64_t b2 = Load64(ptr + 40) ^ chain[2];
  const uint64_t a3 = Load64(ptr + 48) ^ kSecret[7];
  const uint64_t b3 = Load64(ptr + 56) ^ chain[3];

  const uint64_t a4 = Load64(ptr + 64) ^ kSecret[8];
  const uint64_t b4 = Load64(ptr + 72) ^ chain[4];
  const uint64_t a5 = Load64(ptr + 80) ^ kSecret[9];
  const uint64_t b5 = Load64(ptr + 88) ^ chain[5];
  const uint64_t a6 = Load64(ptr + 96) ^ kSecret[10];
  const uint64_t b6 = Load64(ptr + 104) ^ chain[6];
  const uint64_t a7 = Load64(ptr + 112) ^ kSecret[11];
  const uint64_t b7 = Load64(ptr + 120) ^ chain[7];

  chain[0] = Mul128Fold64(a0, b0);
  chain[1] = Mul128Fold64(a1, b1);
  chain[2] = Mul128Fold64(a2, b2);
  chain[3] = Mul128Fold64(a3, b3);
  chain[4] = Mul128Fold64(a4, b4);
  chain[5] = Mul128Fold64(a5, b5);
  chain[6] = Mul128Fold64(a6, b6);
  chain[7] = Mul128Fold64(a7, b7);
}

constexpr std::array<uint64_t, 8> BulkInit(uint64_t seed) noexcept {
  return {
      seed ^ kSecret[8],  seed ^ kSecret[9],  seed ^ kSecret[10], seed ^ kSecret[11],
      seed ^ kSecret[12], seed ^ kSecret[13], seed ^ kSecret[14], seed ^ kSecret[15],
  };
}

constexpr uint64_t BulkMerge(const std::array<uint64_t, 8>& chain) noexcept {
  return chain[0] ^ chain[1] ^ chain[2] ^ chain[3] ^ chain[4] ^ chain[5] ^ chain[6] ^ chain[7];
}

}  // namespace mumbo_internal

using hash_internal::Load64;
using hash_internal::Mul128Fold64;
using mumbo_internal::kSecret;

// NOLINTNEXTLINE(readability-function-cognitive-complexity): tiered by design.
constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();

  // Absorb + finalize the seed (structured seeds must not correlate with
  // input; see README.md's Seed* families).
  seed = Mul128Fold64(seed ^ kSecret[0], kSecret[1]);

  if (len <= 16) {
    const mumbo_internal::SmallInput input = mumbo_internal::LoadSmall(ptr, len);
    return mumbo_internal::Finish(input.a, input.b, seed, len);
  }

  std::size_t remaining = len;
  if (len >= mumbo_internal::kBulkWindow) {
    std::array<uint64_t, 8> chain = mumbo_internal::BulkInit(seed);
    while (remaining >= mumbo_internal::kBulkWindow) {
      mumbo_internal::BulkBlock(chain, ptr);
      ptr += mumbo_internal::kBulkWindow;
      remaining -= mumbo_internal::kBulkWindow;
    }
    seed = mumbo_internal::BulkMerge(chain);
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
  return mumbo_internal::Finish(val_a, val_b, seed, len);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): tiered by design.
constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  const char* ptr = str.data();
  const std::size_t len = str.size();

  // Lane seeds derive from secret pairs distinct from GetHash64's, so no
  // lane ever equals the 64-bit hash (at any length).
  uint64_t seed1 = Mul128Fold64(seed ^ kSecret[2], kSecret[3]);
  uint64_t seed2 = Mul128Fold64(seed ^ kSecret[8], kSecret[9]);

  uint64_t val_a = 0;
  uint64_t val_b = 0;
  if (len <= 16) {
    const mumbo_internal::SmallInput input = mumbo_internal::LoadSmall(ptr, len);
    val_a = input.a;
    val_b = input.b;
  } else {
    std::size_t remaining = len;
    if (len >= 64) {
      // Shared 4-chain bulk tier: each lane derives from ALL chains, through
      // different (nonlinear) merges, so both halves cover every input byte.
      // Interleaved manually here to eliminate execution pipeline bottlenecks.
      uint64_t chain0 = seed1 ^ kSecret[8];
      uint64_t chain1 = seed1 ^ kSecret[9];
      uint64_t chain2 = seed2 ^ kSecret[10];
      uint64_t chain3 = seed2 ^ kSecret[11];
      while (remaining >= 64) {
        const uint64_t a0 = Load64(ptr) ^ kSecret[4];
        const uint64_t b0 = Load64(ptr + 8) ^ chain0;
        const uint64_t a1 = Load64(ptr + 16) ^ kSecret[5];
        const uint64_t b1 = Load64(ptr + 24) ^ chain1;
        const uint64_t a2 = Load64(ptr + 32) ^ kSecret[6];
        const uint64_t b2 = Load64(ptr + 40) ^ chain2;
        const uint64_t a3 = Load64(ptr + 48) ^ kSecret[7];
        const uint64_t b3 = Load64(ptr + 56) ^ chain3;

        chain0 = Mul128Fold64(a0, b0);
        chain1 = Mul128Fold64(a1, b1);
        chain2 = Mul128Fold64(a2, b2);
        chain3 = Mul128Fold64(a3, b3);

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
      .h1 = mumbo_internal::Finish(val_a, val_b, seed1, len),
      .h2 = mumbo_internal::Finish2(val_a, val_b, seed2, len),
  };
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h), with
// streaming (see `mbo::hash::HasStreaming`): chunked updates produce exactly
// the one-shot `GetHash64` value.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mumbo::GetHash64(data, seed);
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return ::mbo::hash::mumbo::GetHash128(data, seed);
  }

  struct StreamState {
    uint64_t seed = 0;                   // Mixed seed (length-free).
    std::array<uint64_t, 8> chain = {};  // Bulk chains (once started).
    bool chains_started = false;
    std::array<char, mumbo_internal::kBulkWindow> buffer = {};  // Bytes not yet bulk-consumed.
    std::size_t buffered = 0;
    std::array<char, 16> last16 = {};  // Rolling window of the last 16 bytes.
    uint64_t total = 0;                // Bytes absorbed overall.
  };

  static constexpr StreamState StreamInit(uint64_t seed = kDefaultSeed) noexcept {
    StreamState state;
    state.seed = Mul128Fold64(seed ^ kSecret[0], kSecret[1]);
    return state;
  }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    const char* ptr = data.data();
    std::size_t remaining = data.size();
    state.total += remaining;
    while (remaining > 0) {
      const std::size_t space = mumbo_internal::kBulkWindow - state.buffered;
      const std::size_t take = remaining < space ? remaining : space;
      for (std::size_t i = 0; i < take; ++i) {
        state.buffer[state.buffered + i] = ptr[i];
      }
      state.buffered += take;
      ptr += take;
      remaining -= take;
      if (state.buffered == mumbo_internal::kBulkWindow) {
        // Eager consumption matches the one-shot bulk loop: a full 128-byte
        // block is always bulk-absorbed, even when it ends the input.
        if (!state.chains_started) {
          state.chain = mumbo_internal::BulkInit(state.seed);
          state.chains_started = true;
        }
        mumbo_internal::BulkBlock(state.chain, state.buffer.data());
        state.buffered = 0;
      }
    }
    // Maintain the rolling last-16 window (the finalize tail reads the last
    // 16 bytes of the stream, which may span consumed blocks).
    const std::size_t len = data.size();
    if (len >= 16) {
      for (std::size_t i = 0; i < 16; ++i) {
        state.last16[i] = data[len - 16 + i];
      }
    } else if (len > 0) {
      for (std::size_t i = 0; i + len < 16; ++i) {
        state.last16[i] = state.last16[i + len];
      }
      for (std::size_t i = 0; i < len; ++i) {
        state.last16[16 - len + i] = data[i];
      }
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
  }

  // Takes the state by value: the caller's stream stays valid (peekable).
  static constexpr uint64_t StreamFinalize(StreamState state) noexcept {
    uint64_t seed = state.chains_started ? mumbo_internal::BulkMerge(state.chain) : state.seed;
    if (state.total <= 16) {
      const mumbo_internal::SmallInput input =
          mumbo_internal::LoadSmall(state.buffer.data(), static_cast<std::size_t>(state.total));
      return mumbo_internal::Finish(input.a, input.b, seed, state.total);
    }
    const char* ptr = state.buffer.data();
    std::size_t remaining = state.buffered;
    while (remaining > 16) {
      seed = Mul128Fold64(Load64(ptr) ^ kSecret[1], Load64(ptr + 8) ^ seed);
      ptr += 16;
      remaining -= 16;
    }
    // The one-shot tail overlaps the end of the key; the rolling window holds
    // exactly those bytes [total - 16, total).
    const uint64_t val_a = Load64(state.last16.data());
    const uint64_t val_b = Load64(state.last16.data() + 8);
    return mumbo_internal::Finish(val_a, val_b, seed, state.total);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

}  // namespace mbo::hash::mumbo

// jumbo -- the native 128-bit face of the mumbo family ("mumbo jumbo"): the
// same implementation core, fronted for 128-bit use and serving as
// `Default128HashAlgorithm`. Both directions delegate, so `jumbo::Algorithm`
// is as complete as `mumbo::Algorithm` -- they differ only in name and role.
namespace mbo::hash::jumbo {

inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

constexpr Hash128 GetHash128(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  return ::mbo::hash::mumbo::GetHash128(str, seed);
}

constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  return ::mbo::hash::mumbo::GetHash64(str, seed);
}

// The algorithm struct: inherits the complete mumbo surface (64/128 and
// streaming) under the 128-bit-fronted name.
struct Algorithm : ::mbo::hash::mumbo::Algorithm {};

}  // namespace mbo::hash::jumbo

#undef MBO_FORCE_INLINE

#endif  // MBO_HASH_HASH_MUMBO_H_
