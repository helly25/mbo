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

#ifndef MBO_HASH_HASH_TEST_UTIL_H_
#define MBO_HASH_HASH_TEST_UTIL_H_

// Test/benchmark support -- NOT part of the public API.
//
// Each descriptor extends a public algorithm struct (see `IsHashAlgorithm` in
// hash.h) with test metadata:
//   static constexpr std::string_view Name();
//   static constexpr bool kStrongAvalanche;  // whether it targets ~50% avalanche
//   static constexpr bool kSeeded;           // whether the seed parameter is used
// The public concepts (`HasGetHash64` / `HasGetHash128`) detect whether an
// algorithm is 64- or 128-bit based. Shared by hash_test.cc and hash_benchmark.cc.

#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <tuple>

#include "mbo/hash/hash.h"

namespace mbo::hash::algo {

// The current default hash (`mbo::hash::mh`, exposed via `mbo::hash::GetHash*`).
struct DefaultHash : ::mbo::hash::mh::Algorithm {
  static constexpr bool kStrongAvalanche = true;
  static constexpr bool kSeeded = true;

  static constexpr std::string_view Name() { return "mh"; }
};

// The previous "simple" implementation (64-bit only; ignores the seed).
struct SimpleHash : ::mbo::hash::simple::Algorithm {
  static constexpr bool kStrongAvalanche = false;
  static constexpr bool kSeeded = false;

  static constexpr std::string_view Name() { return "simple"; }
};

// FNV-1a 64 (canonical values; byte-at-a-time, weak final diffusion).
struct Fnv1aHash : ::mbo::hash::fnv1a::Algorithm {
  static constexpr bool kStrongAvalanche = false;
  static constexpr bool kSeeded = true;

  static constexpr std::string_view Name() { return "fnv1a"; }
};

// XXH64 (canonical xxHash 64-bit values).
struct Xxh64Hash : ::mbo::hash::xxh64::Algorithm {
  static constexpr bool kStrongAvalanche = true;
  static constexpr bool kSeeded = true;

  static constexpr std::string_view Name() { return "xxh64"; }
};

// XXH3 64-bit (canonical values; modern xxHash generation, scalar).
struct Xxh3Hash : ::mbo::hash::xxh3::Algorithm {
  static constexpr bool kStrongAvalanche = true;
  static constexpr bool kSeeded = true;

  static constexpr std::string_view Name() { return "xxh3"; }
};

// MurmurHash3 x64 128 (canonical values; 128-bit based, GetHash64 == h1).
struct Murmur3Hash : ::mbo::hash::murmur3::Algorithm {
  static constexpr bool kStrongAvalanche = true;
  static constexpr bool kSeeded = true;

  static constexpr std::string_view Name() { return "murmur3"; }
};

// All registered algorithm descriptors. The typed tests and the benchmark both
// derive their coverage from this single list, so adding a descriptor here is
// sufficient to test AND benchmark a new algorithm.
using AllAlgorithms = std::tuple<SimpleHash, DefaultHash, Fnv1aHash, Xxh64Hash, Xxh3Hash, Murmur3Hash>;

// The bit width the algorithm is based on: 128 if it exposes a 128-bit variant.
template<typename Algo>
inline constexpr int kHashBits = HasGetHash128<Algo> ? 128 : 64;

// Pseudo-random byte string of `length` bytes (shared by tests and benchmarks).
inline std::string RandomString(std::mt19937_64& rng, std::size_t length) {
  static constexpr int kMaxByte = 255;
  std::string result(length, '\0');
  std::uniform_int_distribution<int> dist(0, kMaxByte);
  for (std::size_t i = 0; i < length; ++i) {
    result[i] = static_cast<char>(dist(rng));
  }
  return result;
}

}  // namespace mbo::hash::algo

#endif  // MBO_HASH_HASH_TEST_UTIL_H_
