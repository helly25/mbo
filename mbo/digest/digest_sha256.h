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

#ifndef MBO_DIGEST_DIGEST_SHA256_H_
#define MBO_DIGEST_DIGEST_SHA256_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// SHA-224 and SHA-256 (FIPS 180-4, the 32-bit-word SHA-2 family), transcribed
// from the specification; constexpr-safe with one code path for compile time
// and run time. The two algorithms share the compression function and differ
// only in the initial hash value and the output truncation. All constants and
// test vectors are pinned against independent references (see
// digest_test.cc).
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)
// NOLINTBEGIN(readability-identifier-length): the two-letter working
// variables in Compress mirror the spec's a..h (FIPS 180-4, section 6.2.2).

namespace sha2_internal {

// Round constants: fractional parts of the cube roots of the first 64 primes
// (FIPS 180-4, section 4.2.2).
inline constexpr std::array<uint32_t, 64> kRoundK = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

// Initial hash values: fractional parts of the square roots of the first 8
// primes (SHA-256), and the low 32 bits of the 64-bit fractional parts of the
// square roots of the 9th through 16th primes (SHA-224).
inline constexpr std::array<uint32_t, 8> kInit256 = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
};
inline constexpr std::array<uint32_t, 8> kInit224 = {
    0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939, 0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4,
};

inline constexpr std::size_t kBlockSize = 64;

// Compression function: absorbs one 64-byte block (FIPS 180-4, section 6.2.2).
constexpr void Compress(std::array<uint32_t, 8>& hash, const char* block) noexcept {
  std::array<uint32_t, 64> schedule = {};
  for (std::size_t i = 0; i < 16; ++i) {
    schedule[i] = ::mbo::hash::hash_internal::Load32BE(block + (4 * i));
  }
  for (std::size_t i = 16; i < 64; ++i) {
    const uint32_t sig0 = std::rotr(schedule[i - 15], 7) ^ std::rotr(schedule[i - 15], 18) ^ (schedule[i - 15] >> 3U);
    const uint32_t sig1 = std::rotr(schedule[i - 2], 17) ^ std::rotr(schedule[i - 2], 19) ^ (schedule[i - 2] >> 10U);
    schedule[i] = schedule[i - 16] + sig0 + schedule[i - 7] + sig1;
  }

  uint32_t sa = hash[0];
  uint32_t sb = hash[1];
  uint32_t sc = hash[2];
  uint32_t sd = hash[3];
  uint32_t se = hash[4];
  uint32_t sf = hash[5];
  uint32_t sg = hash[6];
  uint32_t sh = hash[7];
  for (std::size_t i = 0; i < 64; ++i) {
    const uint32_t sum1 = std::rotr(se, 6) ^ std::rotr(se, 11) ^ std::rotr(se, 25);
    const uint32_t choose = (se & sf) ^ (~se & sg);
    const uint32_t temp1 = sh + sum1 + choose + kRoundK[i] + schedule[i];
    const uint32_t sum0 = std::rotr(sa, 2) ^ std::rotr(sa, 13) ^ std::rotr(sa, 22);
    const uint32_t majority = (sa & sb) ^ (sa & sc) ^ (sb & sc);
    const uint32_t temp2 = sum0 + majority;
    sh = sg;
    sg = sf;
    sf = se;
    se = sd + temp1;
    sd = sc;
    sc = sb;
    sb = sa;
    sa = temp1 + temp2;
  }
  hash[0] += sa;
  hash[1] += sb;
  hash[2] += sc;
  hash[3] += sd;
  hash[4] += se;
  hash[5] += sf;
  hash[6] += sg;
  hash[7] += sh;
}

// Streaming state shared by SHA-224 and SHA-256. The buffer's fill level is
// `total_len % kBlockSize` (no separate counter needed).
struct State {
  std::array<uint32_t, 8> hash = {};
  std::array<char, kBlockSize> buffer = {};
  uint64_t total_len = 0;  // Bytes absorbed, including the buffered tail.
};

constexpr State Init(const std::array<uint32_t, 8>& init) noexcept {
  return {.hash = init, .buffer = {}, .total_len = 0};
}

constexpr void Update(State& state, std::string_view data) noexcept {
  const std::size_t fill = state.total_len % kBlockSize;
  state.total_len += data.size();
  const char* ptr = data.data();
  std::size_t remaining = data.size();
  if (fill != 0) {
    const std::size_t take = remaining < kBlockSize - fill ? remaining : kBlockSize - fill;
    for (std::size_t i = 0; i < take; ++i) {
      state.buffer[fill + i] = ptr[i];
    }
    ptr += take;
    remaining -= take;
    if (fill + take < kBlockSize) {
      return;
    }
    Compress(state.hash, state.buffer.data());
  }
  while (remaining >= kBlockSize) {
    Compress(state.hash, ptr);
    ptr += kBlockSize;
    remaining -= kBlockSize;
  }
  for (std::size_t i = 0; i < remaining; ++i) {
    state.buffer[i] = ptr[i];
  }
}

// Padding + output (FIPS 180-4, section 5.1.1): append 0x80, zero-fill to 56
// mod 64, append the bit length as a big-endian 64-bit integer. Takes the
// state by value so the caller's state stays valid (peekable finalize).
template<std::size_t DigestSize>
constexpr std::array<uint8_t, DigestSize> Finalize(State state) noexcept {
  static_assert(DigestSize <= 32 && DigestSize % 4 == 0);
  const uint64_t total_bits = state.total_len * 8;
  std::size_t fill = state.total_len % kBlockSize;
  state.buffer[fill++] = static_cast<char>(0x80);
  if (fill > kBlockSize - 8) {
    for (; fill < kBlockSize; ++fill) {
      state.buffer[fill] = 0;
    }
    Compress(state.hash, state.buffer.data());
    fill = 0;
  }
  for (; fill < kBlockSize - 8; ++fill) {
    state.buffer[fill] = 0;
  }
  for (std::size_t i = 0; i < 8; ++i) {
    state.buffer[(kBlockSize - 8) + i] = static_cast<char>(total_bits >> (56 - (8 * i)));
  }
  Compress(state.hash, state.buffer.data());

  std::array<uint8_t, DigestSize> digest = {};
  for (std::size_t i = 0; i < DigestSize; ++i) {
    digest[i] = static_cast<uint8_t>(state.hash[i / 4] >> (24 - (8 * (i % 4))));
  }
  return digest;
}

}  // namespace sha2_internal

namespace sha256 {

inline constexpr std::size_t kDigestSize = 32;
inline constexpr std::size_t kBlockSize = sha2_internal::kBlockSize;

constexpr std::array<uint8_t, kDigestSize> Digest(std::string_view data) noexcept {
  sha2_internal::State state = sha2_internal::Init(sha2_internal::kInit256);
  sha2_internal::Update(state, data);
  return sha2_internal::Finalize<kDigestSize>(state);
}

// The algorithm struct (see `mbo::digest::IsDigestAlgorithm` in digest.h).
struct Algorithm {
  static constexpr std::size_t kDigestSize = ::mbo::digest::sha256::kDigestSize;
  static constexpr std::size_t kBlockSize = ::mbo::digest::sha256::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = sha2_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept { return ::mbo::digest::sha256::Digest(data); }

  static constexpr StreamState StreamInit() noexcept { return sha2_internal::Init(sha2_internal::kInit256); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    sha2_internal::Update(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept {
    return sha2_internal::Finalize<kDigestSize>(state);
  }
};

}  // namespace sha256

namespace sha224 {

inline constexpr std::size_t kDigestSize = 28;
inline constexpr std::size_t kBlockSize = sha2_internal::kBlockSize;

constexpr std::array<uint8_t, kDigestSize> Digest(std::string_view data) noexcept {
  sha2_internal::State state = sha2_internal::Init(sha2_internal::kInit224);
  sha2_internal::Update(state, data);
  return sha2_internal::Finalize<kDigestSize>(state);
}

// The algorithm struct (see `mbo::digest::IsDigestAlgorithm` in digest.h).
struct Algorithm {
  static constexpr std::size_t kDigestSize = ::mbo::digest::sha224::kDigestSize;
  static constexpr std::size_t kBlockSize = ::mbo::digest::sha224::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = sha2_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept { return ::mbo::digest::sha224::Digest(data); }

  static constexpr StreamState StreamInit() noexcept { return sha2_internal::Init(sha2_internal::kInit224); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    sha2_internal::Update(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept {
    return sha2_internal::Finalize<kDigestSize>(state);
  }
};

}  // namespace sha224

// NOLINTEND(readability-identifier-length)
// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_SHA256_H_
