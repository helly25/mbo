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

using HashAlgorithms =
    ::testing::Types<algo::SimpleHash, algo::DefaultHash, algo::Fnv1aHash, algo::Xxh64Hash, algo::Murmur3Hash>;
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
// How Get64 relates to Get128 is algorithm-specific (mh folds, murmur3 truncates)
// and covered by per-algorithm tests below.
TYPED_TEST(HashTest, Hash128) {
  if constexpr (algo::HasHash128<TypeParam>) {
    EXPECT_EQ(TypeParam::Get128("abc", kSeed), TypeParam::Get128("abc", kSeed));  // deterministic
    EXPECT_NE(TypeParam::Get128("abc", kSeed), TypeParam::Get128("abd", kSeed));  // distinct
    constexpr std::string_view kLong = "this input is longer than the small-input cutoff.";
    const Hash128 hash = TypeParam::Get128(kLong, kSeed);
    EXPECT_NE(hash.h1, hash.h2);
  } else {
    GTEST_SKIP() << TypeParam::Name() << " is 64-bit only";
  }
}

// mh-specific: above the small-input cutoff the 64-bit variant folds the 128-bit state.
TEST(MhHashTest, Hash64FoldsHash128AboveCutoff) {
  constexpr std::string_view kLong = "this input is longer than the small-input cutoff.";
  static_assert(kLong.size() > mh::kSmallInputCutoff);
  EXPECT_EQ(mh::GetHash64(kLong, kSeed), hash_internal::Hash128To64(mh::GetHash128(kLong, kSeed)));
}

// murmur3-specific: the 64-bit variant is the canonical `h1` truncation.
TEST(Murmur3HashTest, Hash64IsH1) {
  EXPECT_EQ(murmur3::GetHash64("truncation-check"), murmur3::GetHash128("truncation-check").h1);
}

// ---------------------------------------------------------------------------
// Known-answer tests: fnv1a / xxh64 / murmur3 must produce the canonical
// reference values (vectors generated with the reference implementations:
// python `xxhash` and `mmh3` packages, FNV-1a per its published spec).
// The inputs cover the block/tail boundaries of each algorithm.
// ---------------------------------------------------------------------------

// Positional rows read better than repeating field names in a homogeneous table.
// NOLINTBEGIN(modernize-use-designated-initializers)

struct KnownAnswer64 {
  uint64_t seed;
  std::string_view data;
  uint64_t expected;
};

TEST(KnownAnswerTest, Fnv1a) {
  // Canonical FNV-1a 64 with the standard offset basis.
  static constexpr std::array<KnownAnswer64, 6> kVectors{{
      {fnv1a::kOffsetBasis, "", 0xcbf29ce484222325ULL},
      {fnv1a::kOffsetBasis, "a", 0xaf63dc4c8601ec8cULL},
      {fnv1a::kOffsetBasis, "abc", 0xe71fa2190541574bULL},
      {fnv1a::kOffsetBasis, "12345678", 0x173932c41a90a42dULL},
      {fnv1a::kOffsetBasis, "1234567890123456", 0x3d30e6aa7c1d4bd3ULL},
      {fnv1a::kOffsetBasis, "The quick brown fox jumps over the lazy dog", 0xf3f9b7f5e7e47110ULL},
  }};
  static_assert(fnv1a::GetHash64("") == 0xcbf29ce484222325ULL);  // constexpr-safe & canonical
  for (const auto& [seed, data, expected] : kVectors) {
    EXPECT_EQ(fnv1a::GetHash64(data, seed), expected) << "input: \"" << data << "\"";
  }
}

TEST(KnownAnswerTest, Xxh64) {
  static constexpr std::array<KnownAnswer64, 13> kVectors{{
      // seed 0; lengths cover <4, 4..7, 8..31, one 32-byte stripe, and beyond.
      {0, "", 0xef46db3751d8e999ULL},
      {0, "a", 0xd24ec4f1a98c6e5bULL},
      {0, "abc", 0x44bc2cf5ad770999ULL},
      {0, "1234", 0xd8316e61d84f6ba4ULL},
      {0, "12345678", 0xd2d02f08cf7cfd4aULL},
      {0, "123456789012345", 0xc377d78ade001a3cULL},
      {0, "1234567890123456", 0xb61c33dc6c59f270ULL},
      {0, "1234567890123456789012345678901", 0x8f367cb873a5376eULL},
      {0, "12345678901234567890123456789012", 0x40fd1aa52d98274cULL},
      {0, "The quick brown fox jumps over the lazy dog", 0x0b242d361fda71bcULL},
      // Non-zero seed.
      {5'381, "", 0xaf382146d5c3cacfULL},
      {5'381, "abc", 0x19d1c1b5c673451fULL},
      {5'381, "12345678901234567890123456789012", 0x38ea26e7c55f8095ULL},
  }};
  static_assert(xxh64::GetHash64("") == 0xef46db3751d8e999ULL);  // constexpr-safe & canonical
  for (const auto& [seed, data, expected] : kVectors) {
    EXPECT_EQ(xxh64::GetHash64(data, seed), expected) << "input: \"" << data << "\", seed: " << seed;
  }
}

struct KnownAnswer128 {
  uint64_t seed;
  std::string_view data;
  Hash128 expected;
};

TEST(KnownAnswerTest, Murmur3) {
  static constexpr std::array<KnownAnswer128, 13> kVectors{{
      // seed 0; lengths cover empty, tail-only, one 16-byte block, block+tail.
      {0, "", {.h1 = 0x0000000000000000ULL, .h2 = 0x0000000000000000ULL}},
      {0, "a", {.h1 = 0x85555565f6597889ULL, .h2 = 0xe6b53a48510e895aULL}},
      {0, "abc", {.h1 = 0xb4963f3f3fad7867ULL, .h2 = 0x3ba2744126ca2d52ULL}},
      {0, "1234", {.h1 = 0x0897364d218fe7b4ULL, .h2 = 0x341e8bd92437fda5ULL}},
      {0, "12345678", {.h1 = 0x3b4a640638b1419cULL, .h2 = 0x913b0e676bd42557ULL}},
      {0, "123456789012345", {.h1 = 0x887001aea2afcfd6ULL, .h2 = 0x1ec326364f0801b3ULL}},
      {0, "1234567890123456", {.h1 = 0x4fbe5dc5c0e32cf8ULL, .h2 = 0xc0c8e96b60c322c1ULL}},
      {0, "1234567890123456789012345678901", {.h1 = 0x834b6c169e6c76d0ULL, .h2 = 0x9fb1ab235cc38195ULL}},
      {0, "12345678901234567890123456789012", {.h1 = 0xfb942466dfc9d856ULL, .h2 = 0x383859a6263746cfULL}},
      {0, "The quick brown fox jumps over the lazy dog", {.h1 = 0xe34bbc7bbc071b6cULL, .h2 = 0x7a433ca9c49a9347ULL}},
      // Non-zero seed (the reference takes a 32-bit seed; 5381 is in range).
      {5'381, "", {.h1 = 0xef2b598707a4bc70ULL, .h2 = 0x702e1ccf5cc2c6d8ULL}},
      {5'381, "abc", {.h1 = 0x5ff46b62f90afefaULL, .h2 = 0xa5d8a2001713a02eULL}},
      {5'381, "1234567890123456", {.h1 = 0x17e206afe56af6d7ULL, .h2 = 0xe73148f7ad58bae2ULL}},
  }};
  static_assert(murmur3::GetHash128("").h1 == 0);  // constexpr-safe & canonical
  for (const auto& [seed, data, expected] : kVectors) {
    EXPECT_EQ(murmur3::GetHash128(data, seed), expected) << "input: \"" << data << "\", seed: " << seed;
  }
}

// NOLINTEND(modernize-use-designated-initializers)

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

// A stand-in alternative implementation matching `Hash64Fn`, used to exercise
// GetHash's pluggability.
constexpr uint64_t AltHash64(std::string_view data, uint64_t seed) noexcept {
  return mh::GetHash64(data, seed) ^ 0xABCDEFULL;
}

TEST(HashMangleTest, GetHashIsPluggable) {
  // The default template arguments match the bare default call.
  EXPECT_EQ(GetHash("plug"), GetHash<&mh::GetHash64>("plug"));
  // Any Hash64Fn flows through the same mangle...
  EXPECT_EQ(GetHash<&AltHash64>("plug"), HashMangle(AltHash64("plug", mh::kDefaultSeed)));
  // ...and a different implementation yields a different mangled value.
  EXPECT_NE(GetHash("plug"), GetHash<&AltHash64>("plug"));
  // The seed is a template constant too (parens guard the macro comma).
  EXPECT_EQ((GetHash<&mh::GetHash64, 999>("plug")), HashMangle(mh::GetHash64("plug", 999)));
  EXPECT_NE(GetHash("plug"), (GetHash<&mh::GetHash64, 999>("plug")));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
