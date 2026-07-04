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

#ifndef MBO_DIGEST_DIGEST_SHAKE_H_
#define MBO_DIGEST_DIGEST_SHAKE_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/digest/digest_sha3.h"

// SHAKE128 and SHAKE256 (FIPS 202, the Keccak extendable-output functions),
// built on the sponge in digest_sha3.h: same absorb, domain byte 0x1F instead
// of 0x06, and an output of any requested length. The suffix number is the
// security strength in bits, not an output size.
//
// The output length is a compile-time parameter: `shake256::Digest<64>(data)`
// returns 64 bytes. Requesting N bytes yields exactly the first N bytes of
// the (conceptually infinite) output stream, so digests of different lengths
// share their common prefix - two lengths are NOT domain-separated. The
// fixed-size `Algorithm<N>` satisfies the digest concepts and works with
// `Streamer` and `Hmac`.
namespace mbo::digest {

namespace shake_internal {

// Both XOFs differ only in rate; this template provides the per-algorithm
// surface (see the aliases in the shake128/shake256 namespaces).
template<std::size_t Rate, std::size_t OutputSize>
struct ShakeAlgorithm {
  static constexpr std::size_t kDigestSize = OutputSize;
  static constexpr std::size_t kBlockSize = Rate;  // The sponge rate.
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = sha3_internal::State;

  static constexpr uint8_t kDomain = 0x1F;  // NOLINT(*-magic-numbers)

  static constexpr DigestType Digest(std::string_view data) noexcept {
    StreamState state = {};
    sha3_internal::Update<Rate>(state, data);
    return sha3_internal::FinalizeXof<OutputSize, Rate>(state, kDomain);
  }

  static constexpr StreamState StreamInit() noexcept { return {}; }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    sha3_internal::Update<Rate>(state, data);
  }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept {
    return sha3_internal::FinalizeXof<OutputSize, Rate>(state, kDomain);
  }
};

}  // namespace shake_internal

namespace shake128 {
inline constexpr std::size_t kRate = 168;  // 200 - 2 * (128 / 8)

template<std::size_t OutputSize>
using Algorithm = shake_internal::ShakeAlgorithm<kRate, OutputSize>;

template<std::size_t OutputSize>
constexpr std::array<uint8_t, OutputSize> Digest(std::string_view data) noexcept {
  return Algorithm<OutputSize>::Digest(data);
}
}  // namespace shake128

namespace shake256 {
inline constexpr std::size_t kRate = 136;  // 200 - 2 * (256 / 8)

template<std::size_t OutputSize>
using Algorithm = shake_internal::ShakeAlgorithm<kRate, OutputSize>;

template<std::size_t OutputSize>
constexpr std::array<uint8_t, OutputSize> Digest(std::string_view data) noexcept {
  return Algorithm<OutputSize>::Digest(data);
}
}  // namespace shake256

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_SHAKE_H_
