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

#include "mbo/hash/hash.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "mbo/hash/hash_test_util.h"

// Shrink the heavy collision test under sanitizers (kept from the original test).
#undef MBO_HASH_TEST_SANITIZER
#if defined(__has_feature)
# if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer)
#  define MBO_HASH_TEST_SANITIZER
# endif
#elif defined(__SANITIZE_ADDRESS__)
# define MBO_HASH_TEST_SANITIZER
#endif

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-magic-numbers)

constexpr uint64_t kSeed = 5'381;

// ---------------------------------------------------------------------------
// Templated framework: every test below runs once per hash-algorithm descriptor
// (see hash_test_util.h). Add an algorithm to `HashAlgorithms` to test it.
// ---------------------------------------------------------------------------
template<typename Algo>
class HashTest : public ::testing::Test {};

using HashAlgorithms = ::testing::Types<algo::SimpleHash, algo::DefaultHash>;
TYPED_TEST_SUITE(HashTest, HashAlgorithms);

// The framework detects 64- vs 128-bit algorithms from the descriptor.
TYPED_TEST(HashTest, BitWidthDetection) {
  EXPECT_EQ((algo::kHashBits<TypeParam>), algo::HasHash128<TypeParam> ? 128 : 64);
}

// Compile-time and run-time evaluation must agree (single code path).
TYPED_TEST(HashTest, ConstexprMatchesRuntime) {
  constexpr uint64_t kCompile = TypeParam::Get64("constexpr-vs-runtime", kSeed);
  std::string_view runtime = "constexpr-vs-runtime";  // non-const forces the runtime path
  EXPECT_EQ(kCompile, TypeParam::Get64(runtime, kSeed));
  if constexpr (algo::HasHash128<TypeParam>) {
    constexpr Hash128 kCompile128 = TypeParam::Get128("constexpr-vs-runtime", kSeed);
    EXPECT_EQ(kCompile128, TypeParam::Get128(runtime, kSeed));
  }
}

TYPED_TEST(HashTest, EmptyEqualsNullAndIsNontrivial) {
  const uint64_t empty = TypeParam::Get64(std::string_view(""), kSeed);
  const uint64_t null_view = TypeParam::Get64(std::string_view(), kSeed);
  EXPECT_EQ(empty, null_view);
  EXPECT_NE(empty, uint64_t{0});
  EXPECT_NE(empty, ~uint64_t{0});
}

TYPED_TEST(HashTest, DistinctForDistinctShortInputs) {
  constexpr std::array<std::string_view, 10> kData{
      "1", "12", "123", "1234", "12345", "123456", "1234567", "12345678", "123456789", "1234567890",
  };
  std::set<uint64_t> hashes;
  for (const std::string_view data : kData) {
    hashes.insert(TypeParam::Get64(data, kSeed));
  }
  EXPECT_EQ(hashes.size(), kData.size());
}

TYPED_TEST(HashTest, LowCollisionRate) {
  std::mt19937_64 rng(::testing::UnitTest::GetInstance()->random_seed());
#ifdef MBO_HASH_TEST_SANITIZER
  constexpr std::size_t kNumStrings = 200'000;
#else   // MBO_HASH_TEST_SANITIZER
  constexpr std::size_t kNumStrings = 2'000'000;
#endif  // MBO_HASH_TEST_SANITIZER
  std::set<std::string> inputs;
  while (inputs.size() < kNumStrings) {
    inputs.insert(algo::RandomString(rng, 30));
  }
  std::set<uint64_t> hashes;
  for (const std::string& input : inputs) {
    hashes.insert(TypeParam::Get64(input, kSeed));
  }
  EXPECT_LE(inputs.size() - hashes.size(), kNumStrings / 2'000'000);
}

// Avalanche: flipping a single input bit should flip ~half the output bits.
// Two lengths so both the small-input path and the block path get exercised.
TYPED_TEST(HashTest, Avalanche) {
  std::mt19937_64 rng(0xA5A5A5A5U);  // NOLINT(cert-msc51-cpp,cert-msc32-c): fixed seed keeps this reproducible
  constexpr int kTrials = 8'000;
  for (const std::size_t length : std::array<std::size_t, 2>{12, 100}) {
    std::size_t flipped = 0;
    for (int trial = 0; trial < kTrials; ++trial) {
      const std::string base = algo::RandomString(rng, length);
      const uint64_t hash0 = TypeParam::Get64(base, kSeed);
      const std::size_t bit = rng() % (base.size() * 8);
      std::string mutated = base;
      const unsigned mask = 1U << (bit % 8U);
      mutated[bit / 8] = static_cast<char>(static_cast<unsigned>(static_cast<unsigned char>(mutated[bit / 8])) ^ mask);
      const uint64_t hash1 = TypeParam::Get64(mutated, kSeed);
      flipped += static_cast<std::size_t>(std::popcount(hash0 ^ hash1));
    }
    const double ratio = static_cast<double>(flipped) / (static_cast<double>(kTrials) * 64.0);
    if constexpr (TypeParam::kStrongAvalanche) {
      EXPECT_GT(ratio, 0.45) << "len=" << length << " weak avalanche: " << ratio;
      EXPECT_LT(ratio, 0.55) << "len=" << length << " biased avalanche: " << ratio;
    } else {
      EXPECT_GT(ratio, 0.10) << "len=" << length << " barely reacts: " << ratio;
    }
  }
}

// 128-bit-only checks; skipped for 64-bit-based algorithms via the detection trait.
TYPED_TEST(HashTest, Hash128) {
  if constexpr (algo::HasHash128<TypeParam>) {
    EXPECT_EQ(TypeParam::Get128("abc", kSeed), TypeParam::Get128("abc", kSeed));  // deterministic
    EXPECT_NE(TypeParam::Get128("abc", kSeed), TypeParam::Get128("abd", kSeed));  // distinct
    constexpr std::string_view kLong = "this input is longer than the small-input cutoff.";
    const Hash128 hash = TypeParam::Get128(kLong, kSeed);
    EXPECT_NE(hash.h1, hash.h2);
    // Above the small-input cutoff, the 64-bit variant folds the 128-bit state.
    EXPECT_EQ(TypeParam::Get64(kLong, kSeed), hash_internal::Hash128To64(TypeParam::Get128(kLong, kSeed)));
  } else {
    GTEST_SKIP() << TypeParam::Name() << " is 64-bit only";
  }
}

// ---------------------------------------------------------------------------
// Hash128 value-type behaviour (not per-algorithm).
// ---------------------------------------------------------------------------
TEST(Hash128Test, AbslStringifyRendersHex) {
  const Hash128 hash{.h1 = 0x0123456789abcdefULL, .h2 = 0xfedcba9876543210ULL};
  EXPECT_EQ(absl::StrCat(hash), "0x0123456789abcdeffedcba9876543210");
}

TEST(Hash128Test, AbslHashValueUsableAsKey) {
  absl::flat_hash_set<Hash128> seen;
  seen.insert(Hash128{.h1 = 1, .h2 = 2});
  seen.insert(Hash128{.h1 = 1, .h2 = 2});
  seen.insert(Hash128{.h1 = 3, .h2 = 4});
  EXPECT_EQ(seen.size(), 2U);
}

TEST(Hash128Test, Ordering) {
  EXPECT_EQ((Hash128{.h1 = 1, .h2 = 2}), (Hash128{.h1 = 1, .h2 = 2}));
  EXPECT_LT((Hash128{.h1 = 1, .h2 = 2}), (Hash128{.h1 = 1, .h2 = 3}));
  EXPECT_LT((Hash128{.h1 = 1, .h2 = 9}), (Hash128{.h1 = 2, .h2 = 0}));
}

// ---------------------------------------------------------------------------
// HashMangle: a single per-build seed XORed in (see hash_mangle.h). The seed
// value is build-dependent, so tests only assert seed-independent properties.
// ---------------------------------------------------------------------------
TEST(HashMangleTest, IsConstantXorMask) {
  // HashMangle(x) == x ^ seed, so XORing two mangled values cancels the seed.
  EXPECT_EQ(HashMangle(0) ^ HashMangle(1), 0ULL ^ 1ULL);
  EXPECT_EQ(HashMangle(0x1111) ^ HashMangle(0x2222), 0x1111ULL ^ 0x2222ULL);
}

TEST(HashMangleTest, GetHashAppliesMangle) {
  EXPECT_EQ(GetHash("hash-mangle-check"), HashMangle(GetHash64("hash-mangle-check")));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
