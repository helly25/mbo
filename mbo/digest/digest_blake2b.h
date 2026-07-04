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

#ifndef MBO_DIGEST_DIGEST_BLAKE2B_H_
#define MBO_DIGEST_DIGEST_BLAKE2B_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// BLAKE2b (RFC 7693), transcribed from the specification; constexpr-safe with
// one code path for compile time and run time. Unkeyed sequential mode with a
// selectable digest size (the RFC's `digest_length` parameter; `blake2b` is
// the full 64-byte form, `blake2b_256` the common 32-byte form). Little-endian
// throughout. Native keying (the RFC's key block) is a possible future
// addition; keyed digesting is otherwise HMAC's job (digest_hmac.h).
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index,*-easily-swappable-parameters)
// NOLINTBEGIN(readability-identifier-length): MixG's parameters mirror the
// spec's a/b/c/d state indices and x/y message words (RFC 7693, section 3.1).

namespace blake2b_internal {

inline constexpr std::size_t kBlockSize = 128;

// The BLAKE2b IV (RFC 7693, section 2.6) - the SHA-512 initial hash values.
inline constexpr std::array<uint64_t, 8> kInit = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179,
};

// Message word schedule (RFC 7693, section 2.7); rounds 10 and 11 repeat rows
// 0 and 1.
inline constexpr std::array<std::array<uint8_t, 16>, 10> kSigma = {{
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
}};

// The G mixing function (RFC 7693, section 3.1).
constexpr void MixG(
    std::array<uint64_t, 16>& vec,
    std::size_t ia,
    std::size_t ib,
    std::size_t ic,
    std::size_t id,
    uint64_t mx,
    uint64_t my) noexcept {
  vec[ia] = vec[ia] + vec[ib] + mx;
  vec[id] = std::rotr(vec[id] ^ vec[ia], 32);
  vec[ic] = vec[ic] + vec[id];
  vec[ib] = std::rotr(vec[ib] ^ vec[ic], 24);
  vec[ia] = vec[ia] + vec[ib] + my;
  vec[id] = std::rotr(vec[id] ^ vec[ia], 16);
  vec[ic] = vec[ic] + vec[id];
  vec[ib] = std::rotr(vec[ib] ^ vec[ic], 63);
}

// Compression function F (RFC 7693, section 3.2). `total` is the byte counter
// t (bytes fed through F including this block; a 64-bit counter suffices for
// any input this library can address). `last` marks the final block.
constexpr void Compress(std::array<uint64_t, 8>& hash, const char* block, uint64_t total, bool last) noexcept {
  std::array<uint64_t, 16> words = {};
  for (std::size_t i = 0; i < 16; ++i) {
    words[i] = ::mbo::hash::hash_internal::Load64(block + (8 * i));  // little-endian
  }
  std::array<uint64_t, 16> vec = {};
  for (std::size_t i = 0; i < 8; ++i) {
    vec[i] = hash[i];
    vec[i + 8] = kInit[i];
  }
  vec[12] ^= total;  // Low word of t; the high word stays 0 (see above).
  if (last) {
    vec[14] = ~vec[14];
  }
  for (std::size_t round = 0; round < 12; ++round) {
    const std::array<uint8_t, 16>& sigma = kSigma[round % 10];
    MixG(vec, 0, 4, 8, 12, words[sigma[0]], words[sigma[1]]);
    MixG(vec, 1, 5, 9, 13, words[sigma[2]], words[sigma[3]]);
    MixG(vec, 2, 6, 10, 14, words[sigma[4]], words[sigma[5]]);
    MixG(vec, 3, 7, 11, 15, words[sigma[6]], words[sigma[7]]);
    MixG(vec, 0, 5, 10, 15, words[sigma[8]], words[sigma[9]]);
    MixG(vec, 1, 6, 11, 12, words[sigma[10]], words[sigma[11]]);
    MixG(vec, 2, 7, 8, 13, words[sigma[12]], words[sigma[13]]);
    MixG(vec, 3, 4, 9, 14, words[sigma[14]], words[sigma[15]]);
  }
  for (std::size_t i = 0; i < 8; ++i) {
    hash[i] ^= vec[i] ^ vec[i + 8];
  }
}

// Streaming state. Unlike the SHA family, BLAKE2 compresses a full buffer
// only once MORE input arrives - the final block (even a full one) is
// compressed at finalize time with the last-block flag.
struct State {
  std::array<uint64_t, 8> hash = {};
  std::array<char, kBlockSize> buffer = {};
  std::size_t buffered = 0;  // Bytes in `buffer` (0..kBlockSize).
  uint64_t compressed = 0;   // Bytes already fed through Compress.
};

template<std::size_t DigestSize>
constexpr State Init() noexcept {
  static_assert(1 <= DigestSize && DigestSize <= 64);
  State state = {.hash = kInit, .buffer = {}, .buffered = 0, .compressed = 0};
  // Parameter block p[0]: digest_length | (key_length << 8) | (fanout << 16)
  // | (depth << 24); sequential mode uses fanout = depth = 1, no key.
  state.hash[0] ^= 0x01010000ULL | static_cast<uint64_t>(DigestSize);
  return state;
}

constexpr void Update(State& state, std::string_view data) noexcept {
  const char* ptr = data.data();
  std::size_t remaining = data.size();
  while (remaining > 0) {
    if (state.buffered == kBlockSize) {
      state.compressed += kBlockSize;
      Compress(state.hash, state.buffer.data(), state.compressed, /*last=*/false);
      state.buffered = 0;
    }
    const std::size_t take = remaining < kBlockSize - state.buffered ? remaining : kBlockSize - state.buffered;
    for (std::size_t i = 0; i < take; ++i) {
      state.buffer[state.buffered + i] = ptr[i];
    }
    state.buffered += take;
    ptr += take;
    remaining -= take;
  }
}

// Zero-pads the final block and compresses it with the last-block flag
// (RFC 7693, section 3.3); output is the little-endian bytes of the state.
// Takes the state by value so the caller's state stays valid (peekable).
template<std::size_t DigestSize>
constexpr std::array<uint8_t, DigestSize> Finalize(State state) noexcept {
  for (std::size_t i = state.buffered; i < kBlockSize; ++i) {
    state.buffer[i] = 0;
  }
  Compress(state.hash, state.buffer.data(), state.compressed + state.buffered, /*last=*/true);
  std::array<uint8_t, DigestSize> digest = {};
  for (std::size_t i = 0; i < DigestSize; ++i) {
    digest[i] = static_cast<uint8_t>(state.hash[i / 8] >> (8 * (i % 8)));
  }
  return digest;
}

// The variants differ only in the digest_length parameter; this template
// provides the per-algorithm surface (see the aliases below).
template<std::size_t DigestSizeParam>
struct Blake2bAlgorithm {
  static constexpr std::size_t kDigestSize = DigestSizeParam;
  static constexpr std::size_t kBlockSize = blake2b_internal::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = State;

  static constexpr DigestType Digest(std::string_view data) noexcept {
    State state = Init<kDigestSize>();
    Update(state, data);
    return Finalize<kDigestSize>(state);
  }

  static constexpr StreamState StreamInit() noexcept { return Init<kDigestSize>(); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept { Update(state, data); }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept { return Finalize<kDigestSize>(state); }
};

}  // namespace blake2b_internal

namespace blake2b {
inline constexpr std::size_t kDigestSize = 64;
using Algorithm = blake2b_internal::Blake2bAlgorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace blake2b

namespace blake2b_256 {
inline constexpr std::size_t kDigestSize = 32;
using Algorithm = blake2b_internal::Blake2bAlgorithm<kDigestSize>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace blake2b_256

// NOLINTEND(readability-identifier-length)
// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index,*-easily-swappable-parameters)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_BLAKE2B_H_
