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

#ifndef MBO_HASH_HASH_TEMBO_H_
#define MBO_HASH_HASH_TEMBO_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"
#include "mbo/hash/hash_types.h"

namespace mbo::hash::tembo {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

inline constexpr uint64_t kDefaultSeed = ::mbo::hash::kDefaultSeed;

namespace tembo_internal {

using hash_internal::Load64;
using hash_internal::Mul128Fold64;
using hash_internal::Mult128;

// The below constants are the 64-bit fractional parts of the square roots of the first prime numbers,
// which are the standard "nothing-up-my-sleeve" numbers used in cryptographic hash functions like
// SHA-512 (primes 1–8) and SHA-384 (primes 9–16). We skip all even results as they are not useful.
inline constexpr std::array<uint64_t, 40> kSecretCodes = {
    0x6A09E667F3BCC909,  // Prime   2
    0xBB67AE8584CAA73B,  // Prime   3
    0x3C6EF372FE94F82B,  // Prime   5
    0xA54FF53A5F1D36F1,  // Prime   7
    0x510E527FADE682D1,  // Prime  11
    0x9B05688C2B3E6C1F,  // Prime  13
    0x1F83D9ABFB41BD6B,  // Prime  17
    0x5BE0CD19137E2179,  // Prime  19
    0xCBBB9D5DC1059ED9,  // Prime  23
    0x629A292A367CD50D,  // Prime  29
    0x9159015A3070DD17,  // Prime  31
    0x152FECD8F70E5939,  // Prime  37
    0x67332667FFC00B31,  // Prime  41
    0x8EB44A8768581511,  // Prime  43
    0xDB0C2E0D64F98FA7,  // Prime  47
    // 0x47B5481DBEFA4FA4,  // Prime  53 skipped: even
    // 0xCA60C24F5A7316C4,  // Prime  53 skipped: even
    // 0xD6A1D877B133A92A,  // Prime  61 skipped: even
    0x14A21A297A13B45B,  // Prime  67
    0x27B70A8546A14091,  // Prime  71
    // 0x2E1B21385C22722C,  // Prime  73 skipped: even
    // 0x4D20A101C0B3921E,  // Prime  79 skipped: even
    0x521A1A86A8D2A111,  // Prime  83
    // 0x7A2D73403A2E8098,  // Prime  89 skipped: even
    0x913A17B805C42171,  // Prime  97
    0x9B1A28A61C2053E9,  // Prime 101
    // 0xB0C383173167123A,  // Prime 103 skipped: even
    0xB8429B4D5D4E8193,  // Prime 107
    // 0xC3A1A2750C0C43B2,  // Prime 109 skipped: even
    // 0xD3B7A812C4653A2C,  // Prime 113 skipped: even
    // 0xE1C09A184D7183B0,  // Prime 127 skipped: even
    // 0xE7A927C15F2E1410,  // Prime 131 skipped: even
    0xF3B971485906D9E7,  // Prime 137
    0x0283E1A639B0C399,  // Prime 139
    0x22ADCE0A2126AD69,  // Prime 149
    0x2C1014382DF863B7,  // Prime 151
    0x4E74711D7B1B6195,  // Prime 157
    0x618E4E73A6C8BA15,  // Prime 163
    0x789230190539E111,  // Prime 167
    0x90993952D1FE8B05,  // Prime 173
    0xAF17E6B1D0E31C01,  // Prime 179
    0xC7289568B309A239,  // Prime 181
    0xD3033F531B5467E3,  // Prime 191
    0xF1E2D75102AD2B89,  // Prime 193
    0x0A97E981D92C979D,  // Prime 197
    0x18FCD50B737A47B1,  // Prime 199
    0x303E8AC7337B753D,  // Prime 211
    0x4DC3EF41539D2C3D,  // Prime 223
    0x643B2338A9C1B203,  // Prime 227
    0x43063A1D4EBB6BA5,  // Prime 233
    0x717316A3DF5D26E3,  // Prime 239
};

#if defined(__GNUC__) || defined(__clang__)
# define MBO_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MBO_FORCE_INLINE __forceinline
#else
# define MBO_FORCE_INLINE inline
#endif

template<uint64_t kNumber, uint64_t kMin, uint64_t kMax>
concept NumberInRangeInclusive = requires { requires(kNumber >= kMin && kMax >= kNumber); };

template<uint64_t kNumLanes, uint64_t kMin, uint64_t kMax>
concept NumLanesInRange = NumberInRangeInclusive<kNumLanes, kMin, kMax>;

template<uint64_t kNumConsts, uint64_t kMin, uint64_t kMax>
concept NumConstsInRange = NumberInRangeInclusive<kNumConsts, kMin, kMax>;

}  // namespace tembo_internal

template<uint64_t kNumLanes = 7, uint64_t kNumConsts = 8, bool kSecretLoopInit = true>
requires(
    tembo_internal::NumLanesInRange<kNumLanes, 2, 8>
    && tembo_internal::NumConstsInRange<kNumConsts, 3, tembo_internal::kSecretCodes.size()>)
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
MBO_FORCE_INLINE constexpr uint64_t GetHash64(std::string_view str, uint64_t seed = kDefaultSeed) noexcept {
  using namespace tembo_internal;
  using namespace hash_internal;

  constexpr uint64_t kLoopBaseConstIndex = 0;
  constexpr uint64_t kIfLadderConstIndex = kLoopBaseConstIndex + kNumLanes;
  constexpr uint64_t kLoopInitConstIndex = kIfLadderConstIndex + kNumLanes;  // Might be unsued (kSecretLoopInit)

  // Bulk window of `kNumLanes` lanes each 16 bytes, so 4 lanes * 16 bytes = 64 bytes.
  // Controls the number of registers we need and unltimately whether we get full ILP or run into register starvation.
  constexpr std::size_t kBulkWindow = kNumLanes * 16;

  size_t len = str.size();

  const char* ptr = str.data();

  // Seed mixing is completely independent of length for short keys.
  // Every bit of the seed reliably permutes the state without length-based cancellation.
  seed ^= Mul128Fold64(seed ^ kSecretCodes[0], kSecretCodes[1]);

  if (len <= 16) {
    const SmallInput input = LoadSmall(ptr, len);
    const Hash128 product = Mult128(input.a ^ kSecretCodes[2], input.b ^ seed);
    // Length is injected exactly once here at the very end to protect
    // against trailing zero-byte variations.
    return Mul128Fold64(product.h1 ^ kSecretCodes[3 % kNumConsts], product.h2 ^ len);
  }

  // Bulk tier entry
  if (len >= kBulkWindow) {
    [[maybe_unused]] uint64_t lane0 = seed ^ len;
    [[maybe_unused]] uint64_t lane1 = seed;
    [[maybe_unused]] uint64_t lane2 = seed;
    [[maybe_unused]] uint64_t lane3 = seed;
    [[maybe_unused]] uint64_t lane4 = seed;
    [[maybe_unused]] uint64_t lane5 = seed;
    [[maybe_unused]] uint64_t lane6 = seed;
    [[maybe_unused]] uint64_t lane7 = seed;
    // Each lane is independent, so the compiler can schedule the four Mul128Fold64 operations concurrently, allowing
    // for better instruction-level parallelism and throughput. This works due to the temp vars.
    // Must use length at least once.
    if constexpr (kSecretLoopInit) {
      lane0 ^= kSecretCodes[(kLoopInitConstIndex + 0) % kNumConsts];
      lane1 ^= kSecretCodes[(kLoopInitConstIndex + 1) % kNumConsts];
      lane2 ^= kSecretCodes[(kLoopInitConstIndex + 2) % kNumConsts];
      lane3 ^= kSecretCodes[(kLoopInitConstIndex + 3) % kNumConsts];
      lane4 ^= kSecretCodes[(kLoopInitConstIndex + 4) % kNumConsts];
      lane5 ^= kSecretCodes[(kLoopInitConstIndex + 5) % kNumConsts];
      lane6 ^= kSecretCodes[(kLoopInitConstIndex + 6) % kNumConsts];
      lane7 ^= kSecretCodes[(kLoopInitConstIndex + 7) % kNumConsts];
    }
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    do {
      constexpr uint64_t kSecret0 = kSecretCodes[(kLoopBaseConstIndex + 0) % kNumConsts];
      constexpr uint64_t kSecret1 = kSecretCodes[(kLoopBaseConstIndex + 1) % kNumConsts];
      constexpr uint64_t kSecret2 = kSecretCodes[(kLoopBaseConstIndex + 2) % kNumConsts];
      constexpr uint64_t kSecret3 = kSecretCodes[(kLoopBaseConstIndex + 3) % kNumConsts];
      constexpr uint64_t kSecret4 = kSecretCodes[(kLoopBaseConstIndex + 4) % kNumConsts];
      constexpr uint64_t kSecret5 = kSecretCodes[(kLoopBaseConstIndex + 5) % kNumConsts];
      constexpr uint64_t kSecret6 = kSecretCodes[(kLoopBaseConstIndex + 6) % kNumConsts];
      constexpr uint64_t kSecret7 = kSecretCodes[(kLoopBaseConstIndex + 7) % kNumConsts];
      lane0 = Mul128Fold64(Load64(ptr + 0) ^ kSecret0, Load64(ptr + 8) ^ lane0);
      if constexpr (kNumLanes > 1) {
        lane1 = Mul128Fold64(Load64(ptr + 16) ^ kSecret1, Load64(ptr + 24) ^ lane1);
      }
      if constexpr (kNumLanes > 2) {
        lane2 = Mul128Fold64(Load64(ptr + 32) ^ kSecret2, Load64(ptr + 40) ^ lane2);
      }
      if constexpr (kNumLanes > 3) {
        lane3 = Mul128Fold64(Load64(ptr + 48) ^ kSecret3, Load64(ptr + 56) ^ lane3);
      }
      if constexpr (kNumLanes > 4) {
        lane4 = Mul128Fold64(Load64(ptr + 64) ^ kSecret4, Load64(ptr + 72) ^ lane3);
      }
      if constexpr (kNumLanes > 5) {
        lane5 = Mul128Fold64(Load64(ptr + 80) ^ kSecret5, Load64(ptr + 88) ^ lane3);
      }
      if constexpr (kNumLanes > 6) {
        lane6 = Mul128Fold64(Load64(ptr + 96) ^ kSecret6, Load64(ptr + 104) ^ lane3);
      }
      if constexpr (kNumLanes > 7) {
        lane7 = Mul128Fold64(Load64(ptr + 112) ^ kSecret7, Load64(ptr + 120) ^ lane3);
      }
      ptr += kBulkWindow;
      len -= kBulkWindow;
    } while (len >= kBulkWindow);
    if constexpr (kNumLanes == 1) {
      seed = lane0;
    } else if constexpr (kNumLanes == 2) {
      seed = Mul128Fold64(lane0, lane1);
    } else if constexpr (kNumLanes == 3) {
      seed = Mul128Fold64(lane0 ^ lane2, lane1);
    } else if constexpr (kNumLanes == 4) {
      seed = Mul128Fold64(lane0 ^ lane2, lane1 ^ lane3);
    } else if constexpr (kNumLanes == 5) {
      seed = Mul128Fold64(lane0 ^ lane2 ^ lane4, lane1 ^ lane3);
    } else if constexpr (kNumLanes == 6) {
      seed = Mul128Fold64(lane0 ^ lane2 ^ lane4, lane1 ^ lane3 ^ lane5);
    } else if constexpr (kNumLanes == 7) {
      seed = Mul128Fold64(lane0 ^ lane2 ^ lane4 ^ lane6, lane1 ^ lane3 ^ lane5);
    } else if constexpr (kNumLanes == 8) {
      seed = Mul128Fold64(lane0 ^ lane2 ^ lane4 ^ lane6, lane1 ^ lane3 ^ lane5 ^ lane7);
    }
  }

  // No longer update `ptr` or `len`.
  // If we we were at actual bulk size, then we would use another bulk process step.
  // We support kBulkWindow size minus the final backwards read size.
  // The first here is `+2` and not `+0`. Otherwise `fambo` fails SMHasher.
  constexpr uint64_t kSecret0 = kSecretCodes[(kIfLadderConstIndex + 2) % kNumConsts];
  constexpr uint64_t kSecret1 = kSecretCodes[(kIfLadderConstIndex + 1) % kNumConsts];
  constexpr uint64_t kSecret2 = kSecretCodes[(kIfLadderConstIndex + 2) % kNumConsts];
  constexpr uint64_t kSecret3 = kSecretCodes[(kIfLadderConstIndex + 3) % kNumConsts];
  constexpr uint64_t kSecret4 = kSecretCodes[(kIfLadderConstIndex + 4) % kNumConsts];
  constexpr uint64_t kSecret5 = kSecretCodes[(kIfLadderConstIndex + 5) % kNumConsts];
  constexpr uint64_t kSecret6 = kSecretCodes[(kIfLadderConstIndex + 6) % kNumConsts];
  if (len > 16) {
    seed = Mul128Fold64(Load64(ptr) ^ kSecret0, Load64(ptr + 8) ^ seed);
    if (len > 32) {
      seed = Mul128Fold64(Load64(ptr + 16) ^ kSecret1, Load64(ptr + 24) ^ seed);
      if (len > 48) {
        seed = Mul128Fold64(Load64(ptr + 32) ^ kSecret2, Load64(ptr + 40) ^ seed);
        if (len > 64) {
          seed = Mul128Fold64(Load64(ptr + 48) ^ kSecret3, Load64(ptr + 56) ^ seed);
          if (len > 80) {
            seed = Mul128Fold64(Load64(ptr + 64) ^ kSecret4, Load64(ptr + 72) ^ seed);
            if (len > 96) {
              seed = Mul128Fold64(Load64(ptr + 80) ^ kSecret5, Load64(ptr + 88) ^ seed);
              if (len > 112) {
                seed = Mul128Fold64(Load64(ptr + 96) ^ kSecret6, Load64(ptr + 104) ^ seed);
              }
            }
          }
        }
      }
    }
  }

  // The remaining offset (len) from before the if-ladder is needed for the trailing block computation.
  const uint64_t val_a = Load64(ptr + len - 16) ^ kSecretCodes[0];
  const uint64_t val_b = Load64(ptr + len - 8) ^ seed;

  const Hash128 product = Mult128(val_a, val_b);
  return Mul128Fold64(product.h1 ^ kSecretCodes[1], product.h2 ^ kSecretCodes[2] ^ len);
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). Weakly
// seeded: the seed folds into the initial state and rides the MUM chain.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return mbo::hash::tembo::GetHash64<7>(data, seed);
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-easily-swappable-parameters,readability-identifier-length)

}  // namespace mbo::hash::tembo

#undef MBO_FORCE_INLINE

#endif  // MBO_HASH_HASH_TEMBO_H_
