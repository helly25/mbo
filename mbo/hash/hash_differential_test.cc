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

// Differential test ("fuzz-lite"): our constexpr-safe XXH64 / XXH3-64
// implementations must agree with the actual reference library (BCR `xxhash`)
// for randomized inputs, seeds, and lengths across all dispatch classes --
// far beyond what fixed known-answer vectors can cover.

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

#include "gtest/gtest.h"
#include "mbo/hash/hash.h"
#include "mbo/hash/hash_test_util.h"
#include "xxhash.h"

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-magic-numbers)

TEST(DifferentialTest, Xxh64MatchesReference) {
  std::mt19937_64 rng(0xD1FF64U);  // NOLINT(cert-msc51-cpp,cert-msc32-c): reproducible
  for (int trial = 0; trial < 20'000; ++trial) {
    const std::size_t len = rng() % 300;
    const std::string data = algo::RandomString(rng, len);
    const uint64_t seed = rng();
    ASSERT_EQ(xxh64::GetHash64(data, seed), XXH64(data.data(), data.size(), seed))
        << "len: " << len << ", seed: " << seed << ", trial: " << trial;
  }
  // Long buffers crossing many stripe blocks.
  for (const std::size_t len : {1'000UL, 4'096UL, 100'000UL}) {
    const std::string data = algo::RandomString(rng, len);
    ASSERT_EQ(xxh64::GetHash64(data, 77), XXH64(data.data(), data.size(), 77)) << "len: " << len;
  }
}

TEST(DifferentialTest, Xxh3MatchesReference) {
  std::mt19937_64 rng(0xD1FF33U);  // NOLINT(cert-msc51-cpp,cert-msc32-c): reproducible
  for (int trial = 0; trial < 20'000; ++trial) {
    const std::size_t len = rng() % 300;
    const std::string data = algo::RandomString(rng, len);
    const uint64_t seed = rng();
    ASSERT_EQ(xxh3::GetHash64(data, seed), XXH3_64bits_withSeed(data.data(), data.size(), seed))
        << "len: " << len << ", seed: " << seed << ", trial: " << trial;
    ASSERT_EQ(xxh3::GetHash64(data), XXH3_64bits(data.data(), data.size())) << "len: " << len << " (unseeded)";
  }
  // Long buffers: block boundaries (1024) and beyond, seeded (custom secret)
  // and unseeded, including the reference's SIMD-accelerated path -- proving
  // our scalar transcription matches the vectorized reference bit-for-bit.
  for (const std::size_t len : {241UL, 1'023UL, 1'024UL, 1'025UL, 2'048UL, 100'000UL}) {
    const std::string data = algo::RandomString(rng, len);
    ASSERT_EQ(xxh3::GetHash64(data, 77), XXH3_64bits_withSeed(data.data(), data.size(), 77)) << "len: " << len;
    ASSERT_EQ(xxh3::GetHash64(data), XXH3_64bits(data.data(), data.size())) << "len: " << len;
  }
}

TEST(DifferentialTest, Xxh3_128MatchesReference) {
  std::mt19937_64 rng(0xD1FF128U);  // NOLINT(cert-msc51-cpp,cert-msc32-c): reproducible
  const auto check = [](std::string_view data, uint64_t seed) {
    const XXH128_hash_t ref = XXH3_128bits_withSeed(data.data(), data.size(), seed);
    const Hash128 ours = xxh3::GetHash128(data, seed);
    ASSERT_EQ(ours, (Hash128{.h1 = ref.low64, .h2 = ref.high64})) << "len: " << data.size() << ", seed: " << seed;
  };
  for (int trial = 0; trial < 20'000; ++trial) {
    const std::string data = algo::RandomString(rng, rng() % 300);
    check(data, rng());
    check(data, 0);
  }
  for (const std::size_t len : {241UL, 1'023UL, 1'024UL, 1'025UL, 2'048UL, 100'000UL}) {
    const std::string data = algo::RandomString(rng, len);
    check(data, 77);
    check(data, 0);
  }
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
