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
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_fnv1a.h"          // IWYU pragma: export
#include "mbo/hash/hash_internal_util.h"  // IWYU pragma: export
#include "mbo/hash/hash_mangle.h"         // IWYU pragma: export
#include "mbo/hash/hash_mh.h"             // IWYU pragma: export
#include "mbo/hash/hash_murmur3.h"        // IWYU pragma: export
#include "mbo/hash/hash_rapidhash.h"      // IWYU pragma: export
#include "mbo/hash/hash_simple.h"         // IWYU pragma: export
#include "mbo/hash/hash_siphash.h"        // IWYU pragma: export
#include "mbo/hash/hash_types.h"          // IWYU pragma: export
#include "mbo/hash/hash_xxh3.h"           // IWYU pragma: export
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
concept HasGetHash32 = requires(std::string_view data, uint64_t seed) {
  { Algo::GetHash32(data, seed) } noexcept -> std::same_as<uint32_t>;
};

template<typename Algo>
concept IsHashAlgorithm = HasGetHash64<Algo> || HasGetHash128<Algo>;

// Shrinks a 64-bit hash to 32 bits by XOR-folding the halves, so all 64 input
// bits contribute. For the strong algorithms plain truncation would also be
// sound (their finalizers give uniform low bits), but the fold is the correct
// default for every algorithm -- e.g. FNV-1a's low bits are biased, and
// XOR-folding is that algorithm's official shrinking recommendation.
constexpr uint32_t Hash64To32(uint64_t hash) noexcept {
  return static_cast<uint32_t>(hash ^ (hash >> 32U));
}

// Seed perturbation for the synthesized `GetHash128` fallback's second lane.
inline constexpr uint64_t kSeedFlip = 0x9E3779B97F4A7C15ULL;

// Completes any `IsHashAlgorithm` struct into the full interface, synthesizing
// whatever the algorithm does not provide natively:
//
// - `GetHash64` falls back to folding the 128-bit state (`Hash128To64`).
// - `GetHash128` falls back to two decorrelated 64-bit passes: the second pass
//   skips the first up-to-8 bytes but injects them (and `kSeedFlip`) into its
//   seed. Both lanes thus cover every input byte, yet hash different data, so
//   the lanes decorrelate even for algorithms with weak or ignored seed
//   handling. Note: a synthesized 128-bit value is still built from a 64-bit
//   hash and does not reach true 128-bit collision resistance.
// - `GetHash32` falls back to XOR-folding the 64-bit hash (`Hash64To32`);
//   algorithms with a native 32-bit variant provide `GetHash32` themselves.
// - `GetHash` is always derived: the 64-bit hash folded through `HashMangle`.
//
// `Hasher` is also a transparent functor over `GetHash64`, so it drops straight
// into hash containers with heterogeneous string lookup, e.g.:
//
//   absl::flat_hash_map<std::string, V, mbo::hash::DefaultHasher> map;
//   std::unordered_map<std::string, V, mbo::hash::DefaultHasher, std::equal_to<>> map2;
//   map.find(std::string_view(...));  // no temporary std::string
template<IsHashAlgorithm Algo>
struct Hasher {
  using Algorithm = Algo;
  using is_transparent = void;

  constexpr uint64_t operator()(std::string_view data) const noexcept { return GetHash64(data); }

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
      const std::size_t skip = data.size() < 8 ? data.size() : 8;        // NOLINT(*-magic-numbers)
      const uint64_t head = hash_internal::LoadTail(data.data(), skip);  // NOLINT(*-stringview-data-usage)
      return {
          .h1 = Algo::GetHash64(data, seed),
          .h2 = Algo::GetHash64(data.substr(skip), seed ^ kSeedFlip ^ head),
      };
    }
  }

  static constexpr uint32_t GetHash32(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    if constexpr (HasGetHash32<Algo>) {
      return Algo::GetHash32(data, seed);
    } else {
      return Hash64To32(GetHash64(data, seed));
    }
  }

  static constexpr uint64_t GetHash(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
    return HashMangle(GetHash64(data, seed));
  }
};

// Streaming (incremental) hashing: an algorithm MAY provide
//
//   struct StreamState;                                  // opaque state
//   static StreamState StreamInit(uint64_t seed);
//   static void StreamUpdate(StreamState&, std::string_view chunk);
//   static uint64_t StreamFinalize(StreamState);         // by value: peekable
//
// with the contract that any chunking of the input produces exactly the
// one-shot `GetHash64` value. Not every algorithm can stream faithfully
// (e.g. rapidhash has no canonical streaming form) -- absence is honest.
template<typename Algo>
concept HasStreaming = requires(typename Algo::StreamState state, std::string_view data, uint64_t seed) {
  { Algo::StreamInit(seed) } noexcept -> std::same_as<typename Algo::StreamState>;
  { Algo::StreamUpdate(state, data) } noexcept;
  { Algo::StreamFinalize(state) } noexcept -> std::same_as<uint64_t>;
};

// Object-style convenience wrapper over an algorithm's streaming interface:
//
//   mbo::hash::Streamer<mh::Algorithm> stream;   // or Streamer(seed)
//   stream.Update(part1).Update(part2);
//   uint64_t hash = stream.Finalize();           // == GetHash64(part1 + part2)
template<HasStreaming Algo>
class Streamer {
 public:
  using Algorithm = Algo;

  constexpr Streamer() noexcept : state_(Algo::StreamInit(kDefaultSeed)) {}

  constexpr explicit Streamer(uint64_t seed) noexcept : state_(Algo::StreamInit(seed)) {}

  constexpr Streamer& Update(std::string_view data) noexcept {
    Algo::StreamUpdate(state_, data);
    return *this;
  }

  // Non-destructive: the streamer can continue to be updated afterwards.
  [[nodiscard]] constexpr uint64_t Finalize() const noexcept { return Algo::StreamFinalize(state_); }

 private:
  typename Algo::StreamState state_;
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

// The 32-bit companion (same algorithm selection and stability caveats). Uses
// the algorithm's native 32-bit variant where provided; otherwise the XOR-fold
// of the 64-bit hash (see `Hash64To32`).
template<IsHashAlgorithm Algo = DefaultHashAlgorithm>
constexpr uint32_t GetHash32(std::string_view data, uint64_t seed = kDefaultSeed) noexcept {
  return Hasher<Algo>::GetHash32(data, seed);
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

// Combines two 64-bit hashes into one (order-dependent, well mixed). Chain for
// more values: `CombineHashes(CombineHashes(h1, h2), h3)`.
constexpr uint64_t CombineHashes(uint64_t first, uint64_t second) noexcept {
  // NOLINTNEXTLINE(*-magic-numbers): boost-style golden-ratio combine, Fmix64-finalized.
  return hash_internal::Fmix64(first ^ (second + 0x9E3779B97F4A7C15ULL + (first << 6U) + (first >> 2U)));
}

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_H_
