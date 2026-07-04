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

#ifndef MBO_DIGEST_DIGEST_HMAC_H_
#define MBO_DIGEST_DIGEST_HMAC_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/digest/digest_concepts.h"

// HMAC (RFC 2104 / FIPS 198-1), generic over any streaming digest algorithm
// of this library: H((K' ^ opad) || H((K' ^ ipad) || message)), where K' is
// the key zero-padded to the block size (hashed first if longer). For SHA-3
// the block size is the sponge rate, matching NIST's HMAC-SHA3 vectors.
//
// The MAC's strength follows the underlying digest: HMAC-MD5/HMAC-SHA1 remain
// unbroken as MACs but are legacy interop only - pick HMAC-SHA256 or wider
// for new designs. All values are pinned in digest_test.cc against an
// independent reference.
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-easily-swappable-parameters,*-constant-array-index)

// HMAC over `Algo`, e.g. `Hmac<sha256::Algorithm>`. One-shot:
//
//   const auto mac = Hmac<sha256::Algorithm>::Digest(key, message);
//
// Streaming (the key is part of stream initialization):
//
//   auto state = Hmac<sha256::Algorithm>::StreamInit(key);
//   Hmac<sha256::Algorithm>::StreamUpdate(state, part);
//   const auto mac = Hmac<sha256::Algorithm>::StreamFinalize(state);  // Peekable.
template<typename Algo>
requires(IsDigestAlgorithm<Algo> && HasStreaming<Algo>)
struct Hmac {
  static constexpr std::size_t kDigestSize = Algo::kDigestSize;
  static constexpr std::size_t kBlockSize = Algo::kBlockSize;
  using DigestType = typename Algo::DigestType;

  static_assert(kDigestSize <= kBlockSize, "HMAC requires the digest to fit one block.");

  struct StreamState {
    typename Algo::StreamState inner;
    std::array<char, kBlockSize> opad = {};
  };

  static constexpr StreamState StreamInit(std::string_view key) noexcept {
    std::array<char, kBlockSize> key_block = {};
    if (key.size() > kBlockSize) {
      const DigestType key_digest = Algo::Digest(key);
      for (std::size_t i = 0; i < kDigestSize; ++i) {
        key_block[i] = static_cast<char>(key_digest[i]);
      }
    } else {
      for (std::size_t i = 0; i < key.size(); ++i) {
        key_block[i] = key[i];
      }
    }
    StreamState state = {.inner = Algo::StreamInit(), .opad = {}};
    std::array<char, kBlockSize> ipad = {};
    for (std::size_t i = 0; i < kBlockSize; ++i) {
      ipad[i] = static_cast<char>(static_cast<uint8_t>(key_block[i]) ^ 0x36U);
      state.opad[i] = static_cast<char>(static_cast<uint8_t>(key_block[i]) ^ 0x5CU);
    }
    Algo::StreamUpdate(state.inner, std::string_view(ipad.data(), kBlockSize));
    return state;
  }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    Algo::StreamUpdate(state.inner, data);
  }

  // Takes the state by value so the caller's state stays valid (peekable).
  static constexpr DigestType StreamFinalize(StreamState state) noexcept {
    const DigestType inner_digest = Algo::StreamFinalize(state.inner);
    std::array<char, kDigestSize> inner_chars = {};
    for (std::size_t i = 0; i < kDigestSize; ++i) {
      inner_chars[i] = static_cast<char>(inner_digest[i]);
    }
    typename Algo::StreamState outer = Algo::StreamInit();
    Algo::StreamUpdate(outer, std::string_view(state.opad.data(), kBlockSize));
    Algo::StreamUpdate(outer, std::string_view(inner_chars.data(), kDigestSize));
    return Algo::StreamFinalize(outer);
  }

  static constexpr DigestType Digest(std::string_view key, std::string_view message) noexcept {
    StreamState state = StreamInit(key);
    StreamUpdate(state, message);
    return StreamFinalize(state);
  }
};

// Streaming wrapper analogous to `Streamer`, keyed at construction:
//
//   mbo::digest::HmacStreamer<sha256::Algorithm> stream(key);
//   stream.Update(part1).Update(part2);
//   const auto mac = stream.Finalize();  // Peekable: Update may continue.
template<typename Algo>
requires(IsDigestAlgorithm<Algo> && HasStreaming<Algo>)
class HmacStreamer {
 public:
  using DigestType = typename Algo::DigestType;

  constexpr explicit HmacStreamer(std::string_view key) noexcept : state_(Hmac<Algo>::StreamInit(key)) {}

  constexpr HmacStreamer& Update(std::string_view data) noexcept {
    Hmac<Algo>::StreamUpdate(state_, data);
    return *this;
  }

  [[nodiscard]] constexpr DigestType Finalize() const noexcept { return Hmac<Algo>::StreamFinalize(state_); }

 private:
  typename Hmac<Algo>::StreamState state_;
};

// NOLINTEND(*-magic-numbers,*-easily-swappable-parameters,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_HMAC_H_
