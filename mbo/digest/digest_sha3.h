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

#ifndef MBO_DIGEST_DIGEST_SHA3_H_
#define MBO_DIGEST_DIGEST_SHA3_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// SHA3-224/256/384/512 (FIPS 202, the Keccak sponge), transcribed from the
// specification; constexpr-safe with one code path for compile time and run
// time. The four variants share the Keccak-f[1600] permutation and differ
// only in the rate (200 - 2 * digest size) and output length. Lanes are
// little-endian. The round constants were generated with the FIPS 202 LFSR;
// all values are pinned in digest_test.cc.
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)
// NOLINTBEGIN(readability-identifier-length): x/y are the spec's lane coordinates.

namespace sha3_internal {

// Keccak round constants (FIPS 202, section 3.2.5; generated via the rc(t)
// LFSR).
inline constexpr std::array<uint64_t, 24> kRoundConstants = {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808A, 0x8000000080008000, 0x000000000000808B,
    0x0000000080000001, 0x8000000080008081, 0x8000000000008009, 0x000000000000008A, 0x0000000000000088,
    0x0000000080008009, 0x000000008000000A, 0x000000008000808B, 0x800000000000008B, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 0x000000000000800A, 0x800000008000000A,
    0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
};

// Rho rotation offsets, indexed [x + 5 * y] (FIPS 202, section 3.2.2;
// generated via the (x, y) -> (y, 2x + 3y) walk).
inline constexpr std::array<int, 25> kRhoOffsets = {
    0,  1,  62, 28, 27,  //
    36, 44, 6,  55, 20,  //
    3,  10, 43, 25, 39,  //
    41, 45, 15, 21, 8,   //
    18, 2,  61, 56, 14,
};

// The Keccak-f[1600] permutation: 24 rounds of theta, rho, pi, chi, iota over
// the 5x5 lane state (indexed [x + 5 * y]).
constexpr void KeccakF(std::array<uint64_t, 25>& state) noexcept {
  for (const uint64_t round_constant : kRoundConstants) {
    // Theta.
    std::array<uint64_t, 5> parity = {};
    for (std::size_t x = 0; x < 5; ++x) {
      parity[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
    }
    for (std::size_t x = 0; x < 5; ++x) {
      const uint64_t theta = parity[(x + 4) % 5] ^ std::rotl(parity[(x + 1) % 5], 1);
      for (std::size_t y = 0; y < 5; ++y) {
        state[x + (5 * y)] ^= theta;
      }
    }
    // Rho + pi.
    std::array<uint64_t, 25> moved = {};
    for (std::size_t x = 0; x < 5; ++x) {
      for (std::size_t y = 0; y < 5; ++y) {
        moved[y + (5 * (((2 * x) + (3 * y)) % 5))] = std::rotl(state[x + (5 * y)], kRhoOffsets[x + (5 * y)]);
      }
    }
    // Chi.
    for (std::size_t y = 0; y < 5; ++y) {
      for (std::size_t x = 0; x < 5; ++x) {
        state[x + (5 * y)] = moved[x + (5 * y)] ^ (~moved[((x + 1) % 5) + (5 * y)] & moved[((x + 2) % 5) + (5 * y)]);
      }
    }
    // Iota.
    state[0] ^= round_constant;
  }
}

// Sponge state: absorbing position `pos` walks 0..rate-1; the lanes hold the
// full 200-byte state.
struct State {
  std::array<uint64_t, 25> lanes = {};
  std::size_t pos = 0;
};

constexpr void XorByte(State& state, std::size_t pos, char chr) noexcept {
  state.lanes[pos / 8] ^= static_cast<uint64_t>(static_cast<uint8_t>(chr)) << (8 * (pos % 8));
}

template<std::size_t Rate>
constexpr void Update(State& state, std::string_view data) noexcept {
  static_assert(Rate % 8 == 0);
  const char* ptr = data.data();
  std::size_t remaining = data.size();
  // Byte-wise absorb until lane-aligned within the rate.
  while (remaining > 0 && (state.pos % 8 != 0 || remaining < 8)) {
    XorByte(state, state.pos++, *ptr++);
    --remaining;
    if (state.pos == Rate) {
      KeccakF(state.lanes);
      state.pos = 0;
    }
  }
  // Lane-wise absorb (the fast path; full-rate blocks amortize the permute).
  while (remaining >= 8) {
    state.lanes[state.pos / 8] ^= ::mbo::hash::hash_internal::Load64(ptr);  // little-endian lanes
    state.pos += 8;
    ptr += 8;
    remaining -= 8;
    if (state.pos == Rate) {
      KeccakF(state.lanes);
      state.pos = 0;
    }
  }
  while (remaining > 0) {
    XorByte(state, state.pos++, *ptr++);
    --remaining;
  }
}

// Domain separation + padding (FIPS 202, section 6.1: append the domain bits
// then pad10*1, i.e. XOR the domain byte at the current position and 0x80
// into the last rate byte), then squeeze `OutputSize` bytes (little-endian
// lanes), permuting between rate-sized output blocks. The domain byte is 0x06
// for the SHA-3 digests and 0x1F for the SHAKE XOFs (digest_shake.h). Takes
// the state by value (peekable finalize).
template<std::size_t OutputSize, std::size_t Rate>
constexpr std::array<uint8_t, OutputSize> FinalizeXof(State state, uint8_t domain) noexcept {
  static_assert(OutputSize > 0 && Rate <= 200);
  XorByte(state, state.pos, static_cast<char>(domain));
  XorByte(state, Rate - 1, static_cast<char>(0x80));
  KeccakF(state.lanes);
  std::array<uint8_t, OutputSize> digest = {};
  std::size_t written = 0;
  while (true) {
    const std::size_t take = OutputSize - written < Rate ? OutputSize - written : Rate;
    for (std::size_t i = 0; i < take; ++i) {
      digest[written + i] = static_cast<uint8_t>(state.lanes[i / 8] >> (8 * (i % 8)));
    }
    written += take;
    if (written == OutputSize) {
      return digest;
    }
    KeccakF(state.lanes);
  }
}

// The fixed-size SHA-3 finalize: domain byte 0x06, single squeeze block.
template<std::size_t DigestSize, std::size_t Rate>
constexpr std::array<uint8_t, DigestSize> Finalize(State state) noexcept {
  static_assert(DigestSize < Rate);
  return FinalizeXof<DigestSize, Rate>(state, 0x06);
}

// The four variants differ only in rate and output length; this template
// provides the per-algorithm surface (see the aliases below). `kBlockSize` is
// the sponge rate - the natural block for HMAC-SHA3 (per NIST).
template<std::size_t DigestSizeParam>
struct Sha3Algorithm {
  static constexpr std::size_t kDigestSize = DigestSizeParam;
  static constexpr std::size_t kBlockSize = 200 - (2 * DigestSizeParam);  // The sponge rate.
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = State;

  static constexpr DigestType Digest(std::string_view data) noexcept {
    State state = {};
    Update<kBlockSize>(state, data);
    return Finalize<kDigestSize, kBlockSize>(state);
  }

  static constexpr StreamState StreamInit() noexcept { return {}; }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    Update<kBlockSize>(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept {
    return Finalize<kDigestSize, kBlockSize>(state);
  }
};

}  // namespace sha3_internal

namespace sha3_224 {
inline constexpr std::size_t kDigestSize = 28;
using Algorithm = sha3_internal::Sha3Algorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha3_224

namespace sha3_256 {
inline constexpr std::size_t kDigestSize = 32;
using Algorithm = sha3_internal::Sha3Algorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha3_256

namespace sha3_384 {
inline constexpr std::size_t kDigestSize = 48;
using Algorithm = sha3_internal::Sha3Algorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha3_384

namespace sha3_512 {
inline constexpr std::size_t kDigestSize = 64;
using Algorithm = sha3_internal::Sha3Algorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha3_512

// NOLINTEND(readability-identifier-length)
// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_SHA3_H_
