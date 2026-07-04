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

#ifndef MBO_DIGEST_DIGEST_SHA1_H_
#define MBO_DIGEST_DIGEST_SHA1_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// SHA-1 (FIPS 180-4), transcribed from the specification; constexpr-safe with
// one code path for compile time and run time.
//
// SECURITY WARNING: SHA-1 is COLLISION-BROKEN against adversaries (SHAttered,
// 2017; chosen-prefix collisions since 2020). Provided for legacy
// interoperability only - never use it where untrusted parties influence
// content. Use SHA-256 (or wider) for adversarial integrity.
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

namespace sha1_internal {

inline constexpr std::size_t kBlockSize = 64;

// Initial hash value (FIPS 180-4, section 5.3.1).
inline constexpr std::array<uint32_t, 5> kInit = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0,
};

// Compression function: absorbs one 64-byte block (FIPS 180-4, section 6.1.2).
constexpr void Compress(std::array<uint32_t, 5>& hash, const char* block) noexcept {
  std::array<uint32_t, 80> schedule = {};
  for (std::size_t i = 0; i < 16; ++i) {
    schedule[i] = ::mbo::hash::hash_internal::Load32BE(block + (4 * i));
  }
  for (std::size_t i = 16; i < 80; ++i) {
    schedule[i] = std::rotl(schedule[i - 3] ^ schedule[i - 8] ^ schedule[i - 14] ^ schedule[i - 16], 1);
  }

  uint32_t va = hash[0];
  uint32_t vb = hash[1];
  uint32_t vc = hash[2];
  uint32_t vd = hash[3];
  uint32_t ve = hash[4];
  for (std::size_t i = 0; i < 80; ++i) {
    uint32_t func = 0;
    uint32_t round_k = 0;
    if (i < 20) {
      func = (vb & vc) | (~vb & vd);
      round_k = 0x5A827999;
    } else if (i < 40) {
      func = vb ^ vc ^ vd;
      round_k = 0x6ED9EBA1;
    } else if (i < 60) {
      func = (vb & vc) | (vb & vd) | (vc & vd);
      round_k = 0x8F1BBCDC;
    } else {
      func = vb ^ vc ^ vd;
      round_k = 0xCA62C1D6;
    }
    const uint32_t temp = std::rotl(va, 5) + func + ve + round_k + schedule[i];
    ve = vd;
    vd = vc;
    vc = std::rotl(vb, 30);
    vb = va;
    va = temp;
  }
  hash[0] += va;
  hash[1] += vb;
  hash[2] += vc;
  hash[3] += vd;
  hash[4] += ve;
}

// Streaming state; the buffer's fill level is `total_len % kBlockSize`.
struct State {
  std::array<uint32_t, 5> hash = {};
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

// Padding + output (FIPS 180-4, section 5.1.1; identical scheme to SHA-256).
// Takes the state by value so the caller's state stays valid (peekable).
constexpr std::array<uint8_t, 20> Finalize(State state) noexcept {
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

  std::array<uint8_t, 20> digest = {};
  for (std::size_t i = 0; i < 20; ++i) {
    digest[i] = static_cast<uint8_t>(state.hash[i / 4] >> (24 - (8 * (i % 4))));
  }
  return digest;
}

}  // namespace sha1_internal

namespace sha1 {

inline constexpr std::size_t kDigestSize = 20;
inline constexpr std::size_t kBlockSize = sha1_internal::kBlockSize;

constexpr std::array<uint8_t, kDigestSize> Digest(std::string_view data) noexcept {
  sha1_internal::State state = sha1_internal::Init();
  sha1_internal::Update(state, data);
  return sha1_internal::Finalize(state);
}

// The algorithm struct (see `mbo::digest::IsDigestAlgorithm` in digest.h).
// See the collision warning at the top of this file.
struct Algorithm {
  static constexpr std::size_t kDigestSize = ::mbo::digest::sha1::kDigestSize;
  static constexpr std::size_t kBlockSize = ::mbo::digest::sha1::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = sha1_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept { return ::mbo::digest::sha1::Digest(data); }

  static constexpr StreamState StreamInit() noexcept { return sha1_internal::Init(); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    sha1_internal::Update(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept { return sha1_internal::Finalize(state); }
};

}  // namespace sha1

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_SHA1_H_
