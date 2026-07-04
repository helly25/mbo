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
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "mbo/digest/digest_sha256.h"  // IWYU pragma: export

// Message digests: spec-based, constexpr-safe, vector-pinned transcriptions
// (see README.md for the charter, including what this library will never be).
//
// Each algorithm lives in its own namespace (e.g. `mbo::digest::sha256`) and
// provides:
// - a one-shot `Digest(std::string_view) -> std::array<uint8_t, kDigestSize>`,
// - an `Algorithm` struct satisfying the concepts below, usable with the
//   `Streamer` wrapper for incremental input.
//
// Unlike `mbo::hash`, digests take no seed - keyed digesting is HMAC's job
// (planned, see README.md).
namespace mbo::digest {

// A digest algorithm: fixed-size output, one-shot interface.
template<typename Algo>
concept IsDigestAlgorithm = requires(std::string_view data) {
  requires std::same_as<std::remove_const_t<decltype(Algo::kDigestSize)>, std::size_t>;
  requires std::same_as<typename Algo::DigestType, std::array<uint8_t, Algo::kDigestSize>>;
  { Algo::Digest(data) } noexcept -> std::same_as<typename Algo::DigestType>;
};

// Streaming (incremental) interface. `StreamFinalize` takes the state by
// value, so a stream can be finalized ("peeked") and then continued.
template<typename Algo>
concept HasStreaming = requires(typename Algo::StreamState state, std::string_view data) {
  { Algo::StreamInit() } noexcept -> std::same_as<typename Algo::StreamState>;
  { Algo::StreamUpdate(state, data) } noexcept;
  { Algo::StreamFinalize(state) } noexcept -> std::same_as<typename Algo::DigestType>;
};

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
