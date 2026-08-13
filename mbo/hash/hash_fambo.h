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

#ifndef MBO_HASH_HASH_FAMBO_H_
#define MBO_HASH_HASH_FAMBO_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_dumbo.h"
#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

namespace mbo::hash::fambo {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

namespace fambo_internal {

using hash_internal::Load64;
using hash_internal::Mul128Fold64;
using hash_internal::Mult128;

inline constexpr std::array<uint64_t, 4> kSecret = {
    // Blending & Small Key Secrets (Strictly Odd Primes)
    0x6A09E667F3BCC909,  // Prime
    0xBB67AE8584CAA73B,  // Prime
    0x3C6EF372FE94F82B,  // Prime
    0xA54FF53A5F1D36F1,  // Prime

    // Bulk Block Multiplier Secrets (Lanes 0..3 - Strictly Odd Primes)
    //0x510E527FADE682D1,  // Prime
    //0x9B05688C2B3E6C1F,  // Prime
    //0x1F83D9ABFB41BD6B,  // Prime
    //0x5BE0CD19137E2179,  // Prime

    // Distinct Initializer Secrets (Strictly Odd Primes)
    //0xCBBB9D5DC1059ED9,  // Prime
    //0x629A292A367CD50D,  // Prime
    //0x9159015A3070DD17,  // Prime
    //0x152FECD8F70E5939,  // Prime
};

// FAMBO Core Loop Tier: 4 completely independent lanes.
// The data reads are independent per lane, allowing the CPU to execute all
// four Mul128Fold64 operations concurrently before updating the state.
#if defined(__GNUC__) || defined(__clang__)
# define MBO_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MBO_FORCE_INLINE __forceinline
#else
# define MBO_FORCE_INLINE inline
#endif

}  // namespace fambo_internal

MBO_FORCE_INLINE constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  using namespace fambo_internal;
  using namespace hash_internal;

  size_t len = str.size();

  const char* ptr = str.data();

  // Seed mixing is completely independent of length for short keys.
  // Every bit of the seed reliably permutes the state without length-based cancellation.
  seed ^= Mul128Fold64(seed ^ kSecret[0], kSecret[1]);

  if (len <= 16) {
    const SmallInput input = LoadSmall(ptr, len);
    const Hash128 product = Mult128(input.a ^ kSecret[2], input.b ^ seed);
    // Length is injected exactly once here at the very end to protect
    // against trailing zero-byte variations.
    return Mul128Fold64(product.h1 ^ kSecret[1], product.h2 ^ len);
  }

  // Bulk window of 4 lanes (64 bytes) to completely eliminate ILP register starvation.
  constexpr std::size_t kBulkWindow = 64;

  // Bulk tier entry
  if (len >= kBulkWindow) {
    // Each lane is independent, so the compiler can schedule the four Mul128Fold64 operations concurrently, allowing
    // for better instruction-level parallelism and throughput. This works due to the temp vars.
    uint64_t lane0 = seed ^ len;  // kSecret[0] ^ len;  // Must use length at least once.
    uint64_t lane1 = seed;  // ^ kSecret[1];
    uint64_t lane2 = seed;  // ^ kSecret[2];
    uint64_t lane3 = seed;  // ^ kSecret[3];
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    do {
      lane0 = Mul128Fold64(Load64(ptr + 0) ^ kSecret[0], Load64(ptr + 8) ^ lane0);
      lane1 = Mul128Fold64(Load64(ptr + 16) ^ kSecret[1], Load64(ptr + 24) ^ lane1);
      lane2 = Mul128Fold64(Load64(ptr + 32) ^ kSecret[2], Load64(ptr + 40) ^ lane2);
      lane3 = Mul128Fold64(Load64(ptr + 48) ^ kSecret[3], Load64(ptr + 56) ^ lane3);
      ptr += kBulkWindow;
      len -= kBulkWindow;
    } while (len >= kBulkWindow);
    seed = Mul128Fold64(lane0 ^ lane1, lane2 ^ lane3);
  }

  // No longer update `ptr` or `len`. Limited to the length excluded at the top of the function (16).
  if (len > 16) {
    seed = Mul128Fold64(Load64(ptr) ^ kSecret[2], Load64(ptr + 8) ^ seed);
    if (len > 32) {
      seed = Mul128Fold64(Load64(ptr + 16) ^ kSecret[1], Load64(ptr + 24) ^ seed);
      if (len > 48) {
        seed = Mul128Fold64(Load64(ptr + 32) ^ kSecret[2], Load64(ptr + 40) ^ seed);
      }
    }
  }

  // The remaining offset (len) from before the if-ladder is needed for the trailing block computation.
  const uint64_t val_a = Load64(ptr + len - 16) ^ kSecret[0];
  const uint64_t val_b = Load64(ptr + len - 8) ^ seed;

  const Hash128 product = Mult128(val_a, val_b);
  return Mul128Fold64(product.h1 ^ kSecret[1], product.h2 ^ kSecret[2] ^ len);
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Weakly
// seeded: the seed folds into the initial state and rides the MUM chain.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return mbo::hash::fambo::GetHash64(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

}  // namespace mbo::hash::fambo

#undef MBO_FORCE_INLINE

#endif  // MBO_HASH_HASH_FAMBO_H_
