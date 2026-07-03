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

#ifndef MBO_HASH_HASH_H_
#define MBO_HASH_HASH_H_

#include <concepts>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_fnv1a.h"          // IWYU pragma: export
#include "mbo/hash/hash_internal_util.h"  // IWYU pragma: export
#include "mbo/hash/hash_mangle.h"         // IWYU pragma: export
#include "mbo/hash/hash_mh.h"             // IWYU pragma: export
#include "mbo/hash/hash_murmur3.h"        // IWYU pragma: export
#include "mbo/hash/hash_simple.h"         // IWYU pragma: export
#include "mbo/hash/hash_types.h"          // IWYU pragma: export
#include "mbo/hash/hash_xxh64.h"          // IWYU pragma: export

namespace mbo::hash {

// Every algorithm is represented by a struct with static member functions
// `GetHash64` and/or `GetHash128` taking `(std::string_view, uint64_t seed)`
// (e.g. `mh::Algorithm`, `xxh64::Algorithm`, `murmur3::Algorithm`). These concepts detect what
// an algorithm provides; `Hasher` below completes any partial algorithm.

template<typename Algo>
concept HasGetHash64 = requires(std::string_view data, uint64_t seed) {
  { Algo::GetHash64(data, seed) } noexcept -> std::same_as<uint64_t>;
};

template<typename Algo>
concept HasGetHash128 = requires(std::string_view data, uint64_t seed) {
  { Algo::GetHash128(data, seed) } noexcept -> std::same_as<Hash128>;
};

template<typename Algo>
concept IsHashAlgorithm = HasGetHash64<Algo> || HasGetHash128<Algo>;

// Seed perturbation for the synthesized `GetHash128` fallback's second lane.
inline constexpr uint64_t kSeedFlip = 0x9E3779B97F4A7C15ULL;

// Completes any `IsHashAlgorithm` struct into the full interface, synthesizing
// whatever the algorithm does not provide natively:
//
// - `GetHash64` falls back to folding the 128-bit state (`Hash128To64`).
// - `GetHash128` falls back to two independently seeded 64-bit passes (`seed`
//   and `seed ^ kSeedFlip`). Note: such a synthesized 128-bit value retains only
//   64-bit collision strength per lane.
// - `GetHash` is always derived: the 64-bit hash folded through `HashMangle`.
template<IsHashAlgorithm Algo>
struct Hasher {
  using Algorithm = Algo;

  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    if constexpr (HasGetHash64<Algo>) {
      return Algo::GetHash64(data, seed);
    } else {
      return hash_internal::Hash128To64(Algo::GetHash128(data, seed));
    }
  }

  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    if constexpr (HasGetHash128<Algo>) {
      return Algo::GetHash128(data, seed);
    } else {
      return {.h1 = Algo::GetHash64(data, seed), .h2 = Algo::GetHash64(data, seed ^ kSeedFlip)};
    }
  }

  static constexpr uint64_t GetHash(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return HashMangle(GetHash64(data, seed));
  }
};

// The selected default algorithm behind the `mbo::hash` entry points.
using DefaultHashAlgorithm = mh::Algorithm;
using DefaultHasher = Hasher<DefaultHashAlgorithm>;

// A fast, constexpr-safe, non-cryptographic 64-bit hash.
//
// The algorithm defaults to `DefaultHashAlgorithm` and can be replaced with any
// `IsHashAlgorithm` struct, e.g. `GetHash64<xxh64::Algorithm>(data)` -- including
// 128-bit-only algorithms, for which the fold fallback applies (see `Hasher`).
//
// Stability: values are not guaranteed stable across library versions, are not
// portable as a persisted/on-the-wire format, and are unsuitable for
// cryptographic use.
template<IsHashAlgorithm Algo = DefaultHashAlgorithm>
constexpr uint64_t GetHash64(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
  return Hasher<Algo>::GetHash64(data, seed);
}

// The 128-bit companion of `GetHash64` (same algorithm selection and stability
// caveats). For 64-bit-only algorithms the synthesized fallback applies.
template<IsHashAlgorithm Algo = DefaultHashAlgorithm>
constexpr Hash128 GetHash128(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
  return Hasher<Algo>::GetHash128(data, seed);
}

// As `GetHash64` but folded through `HashMangle`, which mixes in one of a small,
// fixed set of build-derived seeds (bucketed from `__DATE__`/`__TIME__`, see
// `kMangleSeedCount`; identity with `MBO_HASH_MANGLE=0`). Its value therefore
// **may** change from build to build -- bounded to that seed set so it stays
// cache-friendly, and pinned to a single seed in a hermetic build (e.g. Bazel).
// Use `GetHash` when values must deliberately not be comparable across
// independent builds; use `GetHash64` / `GetHash128` for fully deterministic,
// reproducible values.
template<IsHashAlgorithm Algo = DefaultHashAlgorithm, uint64_t Seed = kDefaultSeed>
constexpr uint64_t GetHash(std::string_view data) noexcept {
  return HashMangle(Hasher<Algo>::GetHash64(data, Seed));
}

// Folds a 128-bit hash into a well-mixed 64-bit one (not a plain truncation).
//
// Useful to derive a 64-bit value from any 128-bit based algorithm, e.g. a
// fold-mixed (non-canonical) 64-bit murmur3:
//
//   uint64_t hash = mbo::hash::Hash128To64(mbo::hash::murmur3::GetHash128(data));
//
// Note that `murmur3::GetHash64` deliberately returns `h1` instead -- the
// customary truncation that matches other MurmurHash3 implementations.
using ::mbo::hash::hash_internal::Hash128To64;

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_H_
