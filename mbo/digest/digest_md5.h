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

#ifndef MBO_DIGEST_DIGEST_MD5_H_
#define MBO_DIGEST_DIGEST_MD5_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// MD5 (RFC 1321), transcribed from the specification; constexpr-safe with one
// code path for compile time and run time. Little-endian throughout (unlike
// the SHA family).
//
// SECURITY WARNING: MD5 is COLLISION-BROKEN against adversaries - two
// colliding files cost seconds on a laptop (chosen-prefix collisions are
// practical). Provided for legacy interoperability (it remains the most
// common file checksum in the wild) and for detecting *accidental*
// corruption only - never use it where untrusted parties influence content.
// Use SHA-256 for adversarial integrity.
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

namespace md5_internal {

inline constexpr std::size_t kBlockSize = 64;

// Per-round addends: floor(2^32 * abs(sin(i + 1))) (RFC 1321, section 3.4);
// generated programmatically.
inline constexpr std::array<uint32_t, 64> kRoundK = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE, 0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE, 0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1, 0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
};

// Per-round left-rotation amounts (RFC 1321, section 3.4).
inline constexpr std::array<uint32_t, 64> kShifts = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,  //
    5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,  //
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,  //
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

// Initial state (RFC 1321, section 3.3).
inline constexpr std::array<uint32_t, 4> kInit = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

// Compression function: absorbs one 64-byte block (RFC 1321, section 3.4).
constexpr void Compress(std::array<uint32_t, 4>& hash, const char* block) noexcept {
  std::array<uint32_t, 16> words = {};
  for (std::size_t i = 0; i < 16; ++i) {
    words[i] = ::mbo::hash::hash_internal::Load32(block + (4 * i));  // little-endian
  }

  uint32_t va = hash[0];
  uint32_t vb = hash[1];
  uint32_t vc = hash[2];
  uint32_t vd = hash[3];
  for (std::size_t i = 0; i < 64; ++i) {
    uint32_t func = 0;
    std::size_t word_index = 0;
    if (i < 16) {
      func = (vb & vc) | (~vb & vd);
      word_index = i;
    } else if (i < 32) {
      func = (vd & vb) | (~vd & vc);
      word_index = ((5 * i) + 1) % 16;
    } else if (i < 48) {
      func = vb ^ vc ^ vd;
      word_index = ((3 * i) + 5) % 16;
    } else {
      func = vc ^ (vb | ~vd);
      word_index = (7 * i) % 16;
    }
    const uint32_t temp = vd;
    vd = vc;
    vc = vb;
    vb += std::rotl(va + func + kRoundK[i] + words[word_index], static_cast<int>(kShifts[i]));
    va = temp;
  }
  hash[0] += va;
  hash[1] += vb;
  hash[2] += vc;
  hash[3] += vd;
}

// Streaming state; the buffer's fill level is `total_len % kBlockSize`.
struct State {
  std::array<uint32_t, 4> hash = {};
  std::array<char, kBlockSize> buffer = {};
  uint64_t total_len = 0;  // Bytes absorbed, including the buffered tail.
};

constexpr State Init() noexcept {
  return {.hash = kInit, .buffer = {}, .total_len = 0};
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

// Padding + output (RFC 1321, section 3.1/3.5): append 0x80, zero-fill to 56
// mod 64, append the bit length as a **little-endian** 64-bit integer; output
// is the little-endian bytes of the state. Takes the state by value so the
// caller's state stays valid (peekable finalize).
constexpr std::array<uint8_t, 16> Finalize(State state) noexcept {
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
    state.buffer[(kBlockSize - 8) + i] = static_cast<char>(total_bits >> (8 * i));
  }
  Compress(state.hash, state.buffer.data());

  std::array<uint8_t, 16> digest = {};
  for (std::size_t i = 0; i < 16; ++i) {
    digest[i] = static_cast<uint8_t>(state.hash[i / 4] >> (8 * (i % 4)));
  }
  return digest;
}

}  // namespace md5_internal

namespace md5 {

inline constexpr std::size_t kDigestSize = 16;
inline constexpr std::size_t kBlockSize = md5_internal::kBlockSize;

constexpr std::array<uint8_t, kDigestSize> Digest(std::string_view data) noexcept {
  md5_internal::State state = md5_internal::Init();
  md5_internal::Update(state, data);
  return md5_internal::Finalize(state);
}

// The algorithm struct (see `mbo::digest::IsDigestAlgorithm` in digest.h).
// See the collision warning at the top of this file.
struct Algorithm {
  static constexpr std::size_t kDigestSize = ::mbo::digest::md5::kDigestSize;
  static constexpr std::size_t kBlockSize = ::mbo::digest::md5::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = md5_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept { return ::mbo::digest::md5::Digest(data); }

  static constexpr StreamState StreamInit() noexcept { return md5_internal::Init(); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    md5_internal::Update(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept { return md5_internal::Finalize(state); }
};

}  // namespace md5

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_MD5_H_
