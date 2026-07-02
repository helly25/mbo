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

#ifndef MBO_HASH_HASH_FNV1A_H_
#define MBO_HASH_HASH_FNV1A_H_

// IWYU pragma: private, include "mbo/hash/hash.h"

#include <cstdint>
#include <string_view>

// FNV-1a, 64-bit (Fowler-Noll-Vo; public domain). Byte-at-a-time, tiny and
// portable, matching the canonical reference values. Weak final diffusion --
// prefer `mbo::hash::mh` unless FNV-1a compatibility is required.
namespace mbo::hash::fnv1a {

// The standard 64-bit FNV offset basis, used as the default seed.
inline constexpr uint64_t kOffsetBasis = 0xCBF29CE484222325ULL;

// The standard 64-bit FNV prime.
inline constexpr uint64_t kPrime = 0x100000001B3ULL;

// Canonical FNV-1a: for each byte `hash = (hash ^ byte) * kPrime`. With the
// default seed this matches published FNV-1a 64 reference values.
constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kOffsetBasis) noexcept {
  uint64_t hash = seed;
  for (const char chr : data) {
    hash ^= static_cast<uint8_t>(chr);
    hash *= kPrime;
  }
  return hash;
}

}  // namespace mbo::hash::fnv1a

#endif  // MBO_HASH_HASH_FNV1A_H_
