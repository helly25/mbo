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

#ifndef MBO_DIGEST_DIGEST_H_
#define MBO_DIGEST_DIGEST_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "mbo/digest/digest_blake2b.h"   // IWYU pragma: export
#include "mbo/digest/digest_concepts.h"  // IWYU pragma: export
#include "mbo/digest/digest_hmac.h"      // IWYU pragma: export
#include "mbo/digest/digest_md5.h"       // IWYU pragma: export
#include "mbo/digest/digest_sha1.h"      // IWYU pragma: export
#include "mbo/digest/digest_sha256.h"    // IWYU pragma: export
#include "mbo/digest/digest_sha3.h"      // IWYU pragma: export
#include "mbo/digest/digest_sha512.h"    // IWYU pragma: export

// Message digests: spec-based, constexpr-safe, vector-pinned transcriptions
// (see README.md for the charter, including what this library will never be).
//
// Each algorithm lives in its own namespace and provides a one-shot
// `Digest(std::string_view) -> std::array<uint8_t, kDigestSize>` plus an
// `Algorithm` struct satisfying `IsDigestAlgorithm` and `HasStreaming`
// (digest_concepts.h), usable with the `Streamer` wrapper below:
//
// - `sha256`, `sha224` - SHA-2, 32-bit words (FIPS 180-4)
// - `sha512`, `sha384`, `sha512_224`, `sha512_256` - SHA-2, 64-bit words
// - `sha3_224`, `sha3_256`, `sha3_384`, `sha3_512` - SHA-3 (FIPS 202)
// - `blake2b`, `blake2b_256` - BLAKE2b (RFC 7693)
// - `sha1`, `md5` - legacy interop only; COLLISION-BROKEN (see their headers)
//
// `Hmac<Algo>` / `HmacStreamer<Algo>` (digest_hmac.h) provide the keyed MAC
// over any of them. Digest algorithms themselves take no seed.
namespace mbo::digest {

// Streaming wrapper:
//
//   mbo::digest::Streamer<mbo::digest::sha256::Algorithm> stream;
//   stream.Update(part1).Update(part2);
//   const auto digest = stream.Finalize();  // Peekable: Update may continue.
template<typename Algo>
requires(IsDigestAlgorithm<Algo> && HasStreaming<Algo>)
class Streamer {
 public:
  using DigestType = typename Algo::DigestType;

  constexpr Streamer() noexcept : state_(Algo::StreamInit()) {}

  constexpr Streamer& Update(std::string_view data) noexcept {
    Algo::StreamUpdate(state_, data);
    return *this;
  }

  [[nodiscard]] constexpr DigestType Finalize() const noexcept { return Algo::StreamFinalize(state_); }

 private:
  typename Algo::StreamState state_;
};

// Lowercase hex rendering, the conventional presentation of a digest (matches
// e.g. `sha256sum` and python's `hexdigest()`).
template<std::size_t N>
std::string ToHexString(const std::array<uint8_t, N>& digest) {
  constexpr std::string_view kHexChars = "0123456789abcdef";
  std::string result;
  result.reserve(2 * N);
  for (const uint8_t byte : digest) {
    result += kHexChars[byte >> 4U];
    result += kHexChars[byte & 0x0FU];  // NOLINT(*-magic-numbers)
  }
  return result;
}

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_H_
