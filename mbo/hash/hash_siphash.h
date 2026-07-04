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

#ifndef MBO_HASH_HASH_SIPHASH_H_
#define MBO_HASH_HASH_SIPHASH_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// Upstream attribution: SipHash by Jean-Philippe Aumasson and Daniel J.
// Bernstein; reference implementation released under CC0
// (https://github.com/veorq/SipHash). This file is a constexpr transcription.

// SipHash (Aumasson & Bernstein; reference implementation CC0/public domain):
// a keyed PRF designed to resist hash-flooding DoS attacks -- THE algorithm for
// hashing untrusted keys (used by the hash tables of Python, Ruby, Rust, ...).
// This is a constexpr-safe implementation producing the canonical values
// (little-endian defined), verified against the reference test vectors.
//
// Unlike the other algorithms in this library, its collision resistance against
// an adversary stems from the 128-bit key staying SECRET; with a public or
// low-entropy key it degrades to an ordinary (slower) non-cryptographic hash.
// SipHash-2-4 is the standard parameterization; SipHash-1-3 is the faster
// variant many hash tables use.
namespace mbo::hash::siphash {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic)

namespace siphash_internal {

struct State {
  uint64_t v0;
  uint64_t v1;
  uint64_t v2;
  uint64_t v3;
};

constexpr void SipRound(State& state) noexcept {
  state.v0 += state.v1;
  state.v1 = std::rotl(state.v1, 13);
  state.v1 ^= state.v0;
  state.v0 = std::rotl(state.v0, 32);
  state.v2 += state.v3;
  state.v3 = std::rotl(state.v3, 16);
  state.v3 ^= state.v2;
  state.v0 += state.v3;
  state.v3 = std::rotl(state.v3, 21);
  state.v3 ^= state.v0;
  state.v2 += state.v1;
  state.v1 = std::rotl(state.v1, 17);
  state.v1 ^= state.v2;
  state.v2 = std::rotl(state.v2, 32);
}

template<int CompressionRounds>
constexpr void Absorb(State& state, uint64_t word) noexcept {
  state.v3 ^= word;
  for (int round = 0; round < CompressionRounds; ++round) {
    SipRound(state);
  }
  state.v0 ^= word;
}

}  // namespace siphash_internal

// Canonical SipHash-c-d with an explicit 128-bit key (`key0`, `key1` are the
// little-endian halves of the reference implementation's 16-byte key).
template<int CompressionRounds, int FinalizationRounds>
constexpr uint64_t SipHash(std::string_view data, uint64_t key0, uint64_t key1) noexcept {
  siphash_internal::State state = {
      .v0 = 0x736f6d6570736575ULL ^ key0,
      .v1 = 0x646f72616e646f6dULL ^ key1,
      .v2 = 0x6c7967656e657261ULL ^ key0,
      .v3 = 0x7465646279746573ULL ^ key1,
  };

  const char* ptr = data.data();
  const std::size_t len = data.size();
  std::size_t remaining = len;

  while (remaining >= 8) {
    const uint64_t word = hash_internal::Load64(ptr);
    state.v3 ^= word;
    for (int round = 0; round < CompressionRounds; ++round) {
      siphash_internal::SipRound(state);
    }
    state.v0 ^= word;
    ptr += 8;
    remaining -= 8;
  }

  const uint64_t last =
      (remaining > 0 ? hash_internal::LoadTail(ptr, remaining) : 0) | (static_cast<uint64_t>(len & 0xFFU) << 56U);
  state.v3 ^= last;
  for (int round = 0; round < CompressionRounds; ++round) {
    siphash_internal::SipRound(state);
  }
  state.v0 ^= last;

  state.v2 ^= 0xFFU;
  for (int round = 0; round < FinalizationRounds; ++round) {
    siphash_internal::SipRound(state);
  }
  return state.v0 ^ state.v1 ^ state.v2 ^ state.v3;
}

// Canonical SipHash-2-4 (the standard parameterization).
constexpr uint64_t GetHash64(std::string_view data, uint64_t key0, uint64_t key1) noexcept {
  return SipHash<2, 4>(data, key0, key1);
}

// SipHash-1-3 (the faster variant used by many hash-table implementations).
constexpr uint64_t GetHash64Sip13(std::string_view data, uint64_t key0, uint64_t key1) noexcept {
  return SipHash<1, 3>(data, key0, key1);
}

// The algorithm struct (see `mbo::hash::IsHashAlgorithm` in hash.h). The 64-bit
// seed is expanded into the 128-bit key as `(seed, Fmix64(seed))`. For actual
// DoS resistance derive the seed from a secret entropy source; a public or
// compile-time seed provides no adversarial protection.
struct Algorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = 0) noexcept {
    return SipHash<2, 4>(data, seed, hash_internal::Fmix64(seed));
  }

  // Streaming interface (see `mbo::hash::HasStreaming`); SipHash is a block
  // ARX construction, so streaming is its native mode. Chunked updates produce
  // exactly the one-shot GetHash64 value.
  struct StreamState {
    siphash_internal::State state = {};
    std::array<char, 8> buffer = {};
    std::size_t buffered = 0;
    std::size_t total = 0;
  };

  static constexpr StreamState StreamInit(uint64_t seed) noexcept {
    const uint64_t key0 = seed;
    const uint64_t key1 = hash_internal::Fmix64(seed);
    return {
        .state =
            {
                .v0 = 0x736f6d6570736575ULL ^ key0,
                .v1 = 0x646f72616e646f6dULL ^ key1,
                .v2 = 0x6c7967656e657261ULL ^ key0,
                .v3 = 0x7465646279746573ULL ^ key1,
            },
        .buffer = {},
        .buffered = 0,
        .total = 0,
    };
  }

  static constexpr void StreamUpdate(StreamState& stream, std::string_view data) noexcept {
    stream.total += data.size();
    const char* ptr = data.data();
    std::size_t remaining = data.size();
    if (stream.buffered > 0) {
      while (stream.buffered < 8 && remaining > 0) {
        stream.buffer[stream.buffered++] = *ptr++;  // NOLINT(*-constant-array-index)
        --remaining;
      }
      if (stream.buffered < 8) {
        return;
      }
      siphash_internal::Absorb<2>(stream.state, hash_internal::Load64(stream.buffer.data()));
      stream.buffered = 0;
    }
    while (remaining >= 8) {
      siphash_internal::Absorb<2>(stream.state, hash_internal::Load64(ptr));
      ptr += 8;  // NOLINT(*-pointer-arithmetic)
      remaining -= 8;
    }
    for (std::size_t i = 0; i < remaining; ++i) {
      stream.buffer[stream.buffered++] = ptr[i];  // NOLINT(*-constant-array-index,*-pointer-arithmetic)
    }
  }

  static constexpr uint64_t StreamFinalize(StreamState stream) noexcept {
    const uint64_t last = (stream.buffered > 0 ? hash_internal::LoadTail(stream.buffer.data(), stream.buffered) : 0)
                          | (static_cast<uint64_t>(stream.total & 0xFFU) << 56U);
    siphash_internal::Absorb<2>(stream.state, last);
    stream.state.v2 ^= 0xFFU;
    for (int round = 0; round < 4; ++round) {
      siphash_internal::SipRound(stream.state);
    }
    return stream.state.v0 ^ stream.state.v1 ^ stream.state.v2 ^ stream.state.v3;
  }
};

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic)

}  // namespace mbo::hash::siphash

#endif  // MBO_HASH_HASH_SIPHASH_H_
