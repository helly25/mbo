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

#ifndef MBO_DIGEST_DIGEST_CONCEPTS_H_
#define MBO_DIGEST_DIGEST_CONCEPTS_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

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

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_CONCEPTS_H_
