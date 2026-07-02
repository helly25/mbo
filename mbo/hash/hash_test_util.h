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
// Provides a set of hash-algorithm *descriptors* and the traits that let a
// single templated test/benchmark drive many algorithms, and detect whether an
// algorithm is 64- or 128-bit based. Shared by hash_test.cc and hash_benchmark.cc.
//
// A descriptor exposes:
//   static constexpr std::string_view Name();
//   static constexpr uint64_t Get64(std::string_view data, uint64_t seed);   // required
//   static constexpr Hash128  Get128(std::string_view data, uint64_t seed);  // optional
//   static constexpr bool     kStrongAvalanche;   // whether it targets ~50% avalanche
// Exposing Get128 makes the algorithm "128-bit based" (see HasHash128 / kHashBits).

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "mbo/hash/hash.h"

namespace mbo::hash::algo {

// The current default hash (namespace `mbo::hash::mh`, exposed via `GetHash*`).
struct DefaultHash {
  static constexpr bool kStrongAvalanche = true;

  static constexpr std::string_view Name() { return "mh"; }

  static constexpr uint64_t Get64(std::string_view data, uint64_t seed) {
    return ::mbo::hash::mh::GetHash64(data, seed);
  }

  static constexpr ::mbo::hash::Hash128 Get128(std::string_view data, uint64_t seed) {
    return ::mbo::hash::mh::GetHash128(data, seed);
  }
};

// The previous "simple" implementation (64-bit only; ignores the seed).
struct SimpleHash {
  static constexpr bool kStrongAvalanche = false;

  static constexpr std::string_view Name() { return "simple"; }

  static constexpr uint64_t Get64(std::string_view data, uint64_t /*seed*/) {
    return ::mbo::hash::simple::GetHash64(data);
  }
};

// FNV-1a 64 (canonical values; byte-at-a-time, weak final diffusion).
struct Fnv1aHash {
  static constexpr bool kStrongAvalanche = false;

  static constexpr std::string_view Name() { return "fnv1a"; }

  static constexpr uint64_t Get64(std::string_view data, uint64_t seed) {
    return ::mbo::hash::fnv1a::GetHash64(data, seed);
  }
};

// XXH64 (canonical xxHash 64-bit values).
struct Xxh64Hash {
  static constexpr bool kStrongAvalanche = true;

  static constexpr std::string_view Name() { return "xxh64"; }

  static constexpr uint64_t Get64(std::string_view data, uint64_t seed) {
    return ::mbo::hash::xxh64::GetHash64(data, seed);
  }
};

// MurmurHash3 x64 128 (canonical values; 128-bit based, Get64 == h1).
struct Murmur3Hash {
  static constexpr bool kStrongAvalanche = true;

  static constexpr std::string_view Name() { return "murmur3"; }

  static constexpr uint64_t Get64(std::string_view data, uint64_t seed) {
    return ::mbo::hash::murmur3::GetHash64(data, seed);
  }

  static constexpr ::mbo::hash::Hash128 Get128(std::string_view data, uint64_t seed) {
    return ::mbo::hash::murmur3::GetHash128(data, seed);
  }
};

// Detects whether an algorithm provides a 128-bit variant (is "128-bit based").
template<typename Algo>
concept HasHash128 = requires(std::string_view data, uint64_t seed) {
  { Algo::Get128(data, seed) } -> std::same_as<::mbo::hash::Hash128>;
};

// The bit width the algorithm is based on: 128 if it exposes a 128-bit variant.
template<typename Algo>
inline constexpr int kHashBits = HasHash128<Algo> ? 128 : 64;

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
