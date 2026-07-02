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

#ifndef MBO_HASH_HASH_TYPES_H_
#define MBO_HASH_HASH_TYPES_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <array>
#include <compare>  // IWYU pragma: keep  // std::strong_ordering for the defaulted operator<=>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace mbo::hash {

// The structured 128-bit hash state produced by `mbo::hash::GetHash128`.
struct Hash128 {
  uint64_t h1;
  uint64_t h2;

  constexpr auto operator<=>(const Hash128& other) const noexcept = default;

  // Abseil hashing support. Dependency-free: the caller-supplied `H` provides
  // `combine`, so this needs no abseil header and keeps `hash_cc` free of that
  // dependency while still letting `Hash128` be used as a key in Abseil maps.
  template<typename H>
  friend H AbslHashValue(H state, const Hash128& value) {
    return H::combine(std::move(state), value.h1, value.h2);
  }

  // Abseil stringification: renders as `0x` followed by 32 lowercase hex digits
  // (`h1` high half, `h2` low half). Dependency-free -- writes to the sink
  // directly instead of via `absl::Format`.
  template<typename Sink>
  friend void AbslStringify(Sink& sink, const Hash128& value) {
    // NOLINTBEGIN(*-magic-numbers,*-constant-array-index)
    static constexpr std::string_view kHex = "0123456789abcdef";
    std::array<char, 34> buffer = {'0', 'x'};
    const std::array<uint64_t, 2> lanes = {value.h1, value.h2};
    std::size_t pos = 2;
    for (const uint64_t lane : lanes) {
      for (int shift = 60; shift >= 0; shift -= 4) {
        buffer[pos++] = kHex[(lane >> static_cast<unsigned>(shift)) & 0xFU];
      }
    }
    sink.Append(std::string_view(buffer.data(), pos));
    // NOLINTEND(*-magic-numbers,*-constant-array-index)
  }
};

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_TYPES_H_
