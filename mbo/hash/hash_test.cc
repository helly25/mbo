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
#include <functional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include "absl/container/flat_hash_map.h"
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

// Derive the gtest type list from the central algo::AllAlgorithms tuple, so a
// descriptor added there is automatically tested here.
template<typename Tuple>
struct TupleToGtestTypes;

template<typename... Ts>
struct TupleToGtestTypes<std::tuple<Ts...>> {
  using type = ::testing::Types<Ts...>;
};

using HashAlgorithms = TupleToGtestTypes<algo::AllAlgorithms>::type;
TYPED_TEST_SUITE(HashTest, HashAlgorithms);

// The framework detects 64- vs 128-bit algorithms from the descriptor.
TYPED_TEST(HashTest, BitWidthDetection) {
  EXPECT_EQ((algo::kHashBits<TypeParam>), HasGetHash128<TypeParam> ? 128 : 64);
}

// Compile-time and run-time evaluation must agree. This also guards the loads'
// dual implementation (constexpr byte assembly vs runtime memcpy): the probes
// cover the tiny, single-lane, and multi-stripe input tiers.
TYPED_TEST(HashTest, ConstexprMatchesRuntime) {
  static constexpr std::array<std::string_view, 3> kProbes{
      "abc",
      "constexpr-vs-runtime",
      "a long probe that spans multiple 32-byte stripes of the large-input hashing loop",
  };
  constexpr std::array<uint64_t, 3> kCompile{
      TypeParam::GetHash64(kProbes[0], kSeed),
      TypeParam::GetHash64(kProbes[1], kSeed),
      TypeParam::GetHash64(kProbes[2], kSeed),
  };
  for (std::size_t i = 0; i < kProbes.size(); ++i) {
    std::string_view runtime = kProbes.at(i);  // non-const forces the runtime path
    EXPECT_EQ(kCompile.at(i), TypeParam::GetHash64(runtime, kSeed)) << "probe: \"" << runtime << "\"";
  }
  if constexpr (HasGetHash128<TypeParam>) {
    constexpr Hash128 kCompile128 = TypeParam::GetHash128(kProbes[2], kSeed);
    std::string_view runtime = kProbes[2];
    EXPECT_EQ(kCompile128, TypeParam::GetHash128(runtime, kSeed));
  }
}

TYPED_TEST(HashTest, EmptyEqualsNullAndIsNontrivial) {
  const uint64_t empty = TypeParam::GetHash64(std::string_view(""), kSeed);
  const uint64_t null_view = TypeParam::GetHash64(std::string_view(), kSeed);
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
    hashes.insert(TypeParam::GetHash64(data, kSeed));
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
    hashes.insert(TypeParam::GetHash64(input, kSeed));
  }
  EXPECT_LE(inputs.size() - hashes.size(), kNumStrings / 2'000'000);
}

// Avalanche: flipping a single input bit should flip ~half the output bits.
// Lengths chosen to exercise every input tier: tail-only (4), single-lane loop
// (12), stripes + remainder (100), and multiple stripes (300).
TYPED_TEST(HashTest, Avalanche) {
  std::mt19937_64 rng(0xA5A5A5A5U);  // NOLINT(cert-msc51-cpp,cert-msc32-c): fixed seed keeps this reproducible
  constexpr int kTrials = 8'000;
  for (const std::size_t length : std::array<std::size_t, 4>{4, 12, 100, 300}) {
    std::size_t flipped = 0;
    for (int trial = 0; trial < kTrials; ++trial) {
      const std::string base = algo::RandomString(rng, length);
      const uint64_t hash0 = TypeParam::GetHash64(base, kSeed);
      const std::size_t bit = rng() % (base.size() * 8);
      std::string mutated = base;
      const unsigned mask = 1U << (bit % 8U);
      mutated[bit / 8] = static_cast<char>(static_cast<unsigned>(static_cast<unsigned char>(mutated[bit / 8])) ^ mask);
      const uint64_t hash1 = TypeParam::GetHash64(mutated, kSeed);
      flipped += static_cast<std::size_t>(std::popcount(hash0 ^ hash1));
    }
    const double ratio = static_cast<double>(flipped) / (static_cast<double>(kTrials) * 64.0);
    if constexpr (TypeParam::kStrongAvalanche) {
      EXPECT_GT(ratio, 0.45) << "len=" << length << " weak avalanche: " << ratio;
      EXPECT_LT(ratio, 0.55) << "len=" << length << " biased avalanche: " << ratio;
    } else {
      // Weak algorithms only need to react at all; e.g. `simple` sits at ~0.07
      // for 4-byte inputs (a documented weakness of its short-input handling).
      EXPECT_GT(ratio, 0.05) << "len=" << length << " barely reacts: " << ratio;
    }
  }
}

// Seed avalanche (SMHasher-style): flipping a single *seed* bit should flip
// ~half the output bits. Skipped for seedless algorithms; weak-avalanche
// algorithms only need to react (fnv1a's multiplies diffuse upward only, so
// high seed bits move few output bits).
TYPED_TEST(HashTest, SeedAvalanche) {
  if constexpr (!TypeParam::kSeeded) {
    GTEST_SKIP() << TypeParam::Name() << " ignores the seed";
  } else {
    std::mt19937_64 rng(0x5EED5EEDU);  // NOLINT(cert-msc51-cpp,cert-msc32-c): fixed seed keeps this reproducible
    constexpr int kTrials = 8'000;
    for (const std::size_t length : std::array<std::size_t, 4>{4, 12, 100, 300}) {
      std::size_t flipped = 0;
      for (int trial = 0; trial < kTrials; ++trial) {
        const std::string data = algo::RandomString(rng, length);
        const uint64_t seed = rng();
        const uint64_t seed_bit = uint64_t{1} << (rng() % 64U);
        flipped += static_cast<std::size_t>(
            std::popcount(TypeParam::GetHash64(data, seed) ^ TypeParam::GetHash64(data, seed ^ seed_bit)));
      }
      const double ratio = static_cast<double>(flipped) / (static_cast<double>(kTrials) * 64.0);
      if constexpr (TypeParam::kStrongAvalanche) {
        EXPECT_GT(ratio, 0.45) << "len=" << length << " weak seed avalanche: " << ratio;
        EXPECT_LT(ratio, 0.55) << "len=" << length << " biased seed avalanche: " << ratio;
      } else {
        EXPECT_GT(ratio, 0.05) << "len=" << length << " seed barely reacts: " << ratio;
      }
    }
  }
}

// Structured/sparse keys (SMHasher-inspired): degenerate input families where
// weak mixing hides from random-string tests. All inputs are pairwise distinct,
// so a 64-bit hash is expected to produce zero collisions across the ~1200 keys
// (birthday bound ~2^-44).
TYPED_TEST(HashTest, StructuredKeysAreDistinct) {
  std::set<std::string> inputs;
  // All-zero inputs of every length 0..256: only the length distinguishes them.
  for (std::size_t len = 0; len <= 256; ++len) {
    inputs.insert(std::string(len, '\0'));
  }
  // Single-bit keys: exactly one bit set, at every position.
  for (const std::size_t len : std::array<std::size_t, 2>{16, 64}) {
    for (std::size_t bit = 0; bit < len * 8; ++bit) {
      std::string key(len, '\0');
      key[bit / 8] = static_cast<char>(1U << (bit % 8U));
      inputs.insert(std::move(key));
    }
  }
  // Cyclic patterns: short periods repeated to a fixed length.
  for (std::size_t period = 1; period <= 16; ++period) {
    std::string pattern;
    for (std::size_t i = 0; i < 48; ++i) {
      pattern.push_back(static_cast<char>('a' + (i % period)));
    }
    inputs.insert(std::move(pattern));
  }
  std::set<uint64_t> hashes;
  for (const std::string& input : inputs) {
    hashes.insert(TypeParam::GetHash64(input, kSeed));
  }
  EXPECT_EQ(hashes.size(), inputs.size()) << "collisions among structured keys";
}

// 128-bit-only checks; skipped for 64-bit-based algorithms via the detection trait.
// How GetHash64 relates to GetHash128 is algorithm-specific (mh folds, murmur3 truncates)
// and covered by per-algorithm tests below.
TYPED_TEST(HashTest, Hash128) {
  if constexpr (HasGetHash128<TypeParam>) {
    EXPECT_EQ(TypeParam::GetHash128("abc", kSeed), TypeParam::GetHash128("abc", kSeed));  // deterministic
    EXPECT_NE(TypeParam::GetHash128("abc", kSeed), TypeParam::GetHash128("abd", kSeed));  // distinct
    constexpr std::string_view kLong = "this input is longer than the small-input cutoff.";
    const Hash128 hash = TypeParam::GetHash128(kLong, kSeed);
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

// The documented composition: a fold-mixed (non-canonical) 64-bit value can be
// derived from any 128-bit based algorithm via the public Hash128To64.
TEST(Murmur3HashTest, FoldedHash64ByComposition) {
  constexpr uint64_t kFolded = Hash128To64(murmur3::GetHash128("fold-check"));  // constexpr-safe
  EXPECT_EQ(kFolded, hash_internal::Hash128To64(murmur3::GetHash128("fold-check")));
  EXPECT_NE(kFolded, murmur3::GetHash64("fold-check"));  // distinct from the canonical truncation
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

// Deterministic buffer matching the python generator: byte i = (i*131 + 7) % 256.
inline std::string PatternBuffer(std::size_t len) {
  std::string result(len, '\0');
  for (std::size_t i = 0; i < len; ++i) {
    result[i] = static_cast<char>(((i * 131) + 7) % 256);
  }
  return result;
}

TEST(KnownAnswerTest, Xxh3) {
  // Vectors from the reference implementation (python xxhash 3.8.0 / libxxhash
  // 0.8.x, XXH3_64bits[_withSeed]); lengths cover every dispatch class:
  // 0, 1..3, 4..8, 9..16, 17..128, 129..240, and >240 (with block boundaries),
  // plus the seeded custom-secret long path.
  struct Vector {
    uint64_t seed;
    std::size_t len;
    uint64_t expected;
  };

  // NOLINTBEGIN(modernize-use-designated-initializers)
  static constexpr std::array<Vector, 33> kVectors{{
      {0, 0, 0x2d06800538d394c2ULL},       {0, 1, 0x4c5cca45d0f4811fULL},         {0, 3, 0x6e3e2670e61106acULL},
      {0, 4, 0x5c4c63133443d03fULL},       {0, 8, 0xf9fd4dd0b04d78f5ULL},         {0, 9, 0x7c20df9712c26edfULL},
      {0, 16, 0x86abf6baccea0858ULL},      {0, 17, 0xb58bf5dc5022d071ULL},        {0, 32, 0xe3712ed84c04a66eULL},
      {0, 100, 0x5da67eac6d4093d5ULL},     {0, 128, 0x10d17f72c0ccba41ULL},       {0, 129, 0x1648bdc3db49d1a2ULL},
      {0, 240, 0xb6cfaf343fab81e6ULL},     {0, 241, 0x956cae592c67279eULL},       {0, 1'024, 0x70bd377d9574f4bbULL},
      {0, 1'025, 0x66c4487c41e127a7ULL},   {0, 3'000, 0x33a08283bba03e0cULL},     {5'381, 0, 0x0bdb294d2ae928f4ULL},
      {5'381, 1, 0xccbc30ca28c5b7e8ULL},   {5'381, 3, 0xc12a2fc1e85cfda3ULL},     {5'381, 4, 0x2a3e441637b99decULL},
      {5'381, 8, 0x3368bd3455763aa3ULL},   {5'381, 9, 0xbf055cc53ba01ac3ULL},     {5'381, 16, 0x22d53e6beb618654ULL},
      {5'381, 17, 0x6613984cb09ca12fULL},  {5'381, 32, 0xe3183cb5fe6a7771ULL},    {5'381, 100, 0xceb50f7a528f1274ULL},
      {5'381, 128, 0xb540c9613098d20fULL}, {5'381, 129, 0xc058e3a79595a255ULL},   {5'381, 240, 0x3fcdb3a19eba1245ULL},
      {5'381, 241, 0xb2cf4aaf8fc38710ULL}, {5'381, 1'024, 0xfb8787295ec29567ULL}, {5'381, 1'025, 0x70b3e67a34b687d0ULL},
  }};
  // NOLINTEND(modernize-use-designated-initializers)
  static_assert(xxh3::GetHash64("") == 0x2d06800538d394c2ULL);  // constexpr-safe & canonical
  for (const auto& [seed, len, expected] : kVectors) {
    EXPECT_EQ(xxh3::GetHash64(PatternBuffer(len), seed), expected) << "len: " << len << ", seed: " << seed;
  }
  EXPECT_EQ(xxh3::GetHash64(PatternBuffer(3'000), 5'381), 0x9ea762ada6fccc4cULL);
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

// ---------------------------------------------------------------------------
// Algorithm structs, concepts, and the Hasher interface generator (hash.h).
// ---------------------------------------------------------------------------

// Fake algorithms exercising the Hasher fallback synthesis.
struct Fake64Only {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed) noexcept {
    return mh::GetHash64(data, seed) ^ 0xABCDEFULL;
  }
};

struct Fake128Only {
  static constexpr Hash128 GetHash128(std::string_view data, uint64_t seed) noexcept {
    return mh::GetHash128(data, seed);
  }
};

// Concept detection over the public algorithm structs and the fakes.
static_assert(HasGetHash64<mh::Algorithm> && HasGetHash128<mh::Algorithm>);
static_assert(HasGetHash64<simple::Algorithm> && !HasGetHash128<simple::Algorithm>);
static_assert(HasGetHash64<fnv1a::Algorithm> && !HasGetHash128<fnv1a::Algorithm>);
static_assert(HasGetHash64<xxh64::Algorithm> && !HasGetHash128<xxh64::Algorithm>);
static_assert(HasGetHash64<murmur3::Algorithm> && HasGetHash128<murmur3::Algorithm>);
static_assert(!HasGetHash64<Fake128Only> && HasGetHash128<Fake128Only>);
static_assert(IsHashAlgorithm<Fake64Only> && IsHashAlgorithm<Fake128Only>);
static_assert(!IsHashAlgorithm<int> && !IsHashAlgorithm<Hash128>);

TEST(HasherTest, NativeFunctionsPassThrough) {
  EXPECT_EQ(Hasher<mh::Algorithm>::GetHash64("pass", kSeed), mh::GetHash64("pass", kSeed));
  EXPECT_EQ(Hasher<mh::Algorithm>::GetHash128("pass", kSeed), mh::GetHash128("pass", kSeed));
  EXPECT_EQ(Hasher<murmur3::Algorithm>::GetHash64("pass", kSeed), murmur3::GetHash64("pass", kSeed));
}

TEST(HasherTest, GetHash64FallsBackToFold) {
  EXPECT_EQ(Hasher<Fake128Only>::GetHash64("fold", kSeed), Hash128To64(Fake128Only::GetHash128("fold", kSeed)));
}

TEST(HasherTest, GetHash128FallsBackToTwoDecorrelatedPasses) {
  // The second lane skips the first up-to-8 bytes and injects them via the seed.
  constexpr std::string_view kData = "twopass-fallback";
  const Hash128 hash = Hasher<Fake64Only>::GetHash128(kData, kSeed);
  EXPECT_EQ(hash.h1, Fake64Only::GetHash64(kData, kSeed));
  const uint64_t head = hash_internal::LoadTail(kData.data(), 8);
  EXPECT_EQ(hash.h2, Fake64Only::GetHash64(kData.substr(8), kSeed ^ kSeedFlip ^ head));
  EXPECT_NE(hash.h1, hash.h2);
  // Short inputs (< 8 bytes): the second lane hashes the empty remainder with
  // all bytes injected via the seed -- still covering every byte.
  const Hash128 tiny = Hasher<Fake64Only>::GetHash128("abc", kSeed);
  const uint64_t tiny_head = hash_internal::LoadTail("abc", 3);
  EXPECT_EQ(tiny.h2, Fake64Only::GetHash64("", kSeed ^ kSeedFlip ^ tiny_head));
}

TEST(HasherTest, GetHash128FallbackDecorrelatesSeedIgnoringAlgorithms) {
  // `simple` ignores the seed entirely; a naive two-seed fallback would yield
  // h1 == h2. The lanes hash different data, so they still differ.
  const Hash128 hash = Hasher<simple::Algorithm>::GetHash128("longer than eight bytes", kSeed);
  EXPECT_NE(hash.h1, hash.h2);
  // And inputs differing only past the first 8 bytes change both lanes.
  const Hash128 other = Hasher<simple::Algorithm>::GetHash128("longer than eight BYTES", kSeed);
  EXPECT_NE(hash.h1, other.h1);
  EXPECT_NE(hash.h2, other.h2);
}

TEST(HasherTest, GetHashIsMangled64) {
  EXPECT_EQ(Hasher<xxh64::Algorithm>::GetHash("mangle", kSeed), HashMangle(xxh64::GetHash64("mangle", kSeed)));
}

TEST(HasherTest, TopLevelFunctionsUseDefaultAlgorithm) {
  // The bare entry points equal the default algorithm (and remain constexpr).
  constexpr uint64_t kHash64 = GetHash64("top");
  EXPECT_EQ(kHash64, (DefaultHasher::GetHash64("top")));
  constexpr Hash128 kHash128 = GetHash128("top");
  EXPECT_EQ(kHash128, (DefaultHasher::GetHash128("top")));
  EXPECT_EQ(GetHash("top"), HashMangle(kHash64));
}

TEST(HasherTest, TopLevelFunctionsArePluggable) {
  // Any algorithm struct can replace the default...
  EXPECT_EQ(GetHash64<xxh64::Algorithm>("plug", kSeed), xxh64::GetHash64("plug", kSeed));
  EXPECT_EQ(GetHash128<Fake64Only>("plug", kSeed), (Hasher<Fake64Only>::GetHash128("plug", kSeed)));
  // ...including in the mangled form, with the seed as a template constant
  // (parens guard the macro comma).
  EXPECT_EQ((GetHash<xxh64::Algorithm, 999>("plug")), HashMangle(xxh64::GetHash64("plug", 999)));
  EXPECT_NE(GetHash("plug"), (GetHash<xxh64::Algorithm, 999>("plug")));
  // A different algorithm yields a different mangled value.
  EXPECT_NE(GetHash("plug"), GetHash<Fake64Only>("plug"));
}

// ---------------------------------------------------------------------------
// Hasher as a transparent container functor, and CombineHashes.
// ---------------------------------------------------------------------------
TEST(HasherTest, WorksAsTransparentContainerFunctor) {
  absl::flat_hash_map<std::string, int, DefaultHasher, std::equal_to<>> map;
  map["alpha"] = 1;
  map["beta"] = 2;
  const std::string_view lookup = "alpha";  // heterogeneous: no temporary std::string
  const auto it = map.find(lookup);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, 1);
  std::unordered_map<std::string, int, Hasher<xxh3::Algorithm>, std::equal_to<>> std_map;
  std_map["gamma"] = 3;
  EXPECT_EQ(std_map.find(std::string_view("gamma"))->second, 3);
}

TEST(CombineHashesTest, MixesAndIsOrderDependent) {
  constexpr uint64_t kHash1 = GetHash64("first");
  constexpr uint64_t kHash2 = GetHash64("second");
  constexpr uint64_t kCombined = CombineHashes(kHash1, kHash2);  // constexpr-safe
  EXPECT_NE(kCombined, kHash1);
  EXPECT_NE(kCombined, kHash2);
  EXPECT_NE(kCombined, CombineHashes(kHash2, kHash1));  // order matters
  EXPECT_EQ(kCombined, CombineHashes(kHash1, kHash2));  // deterministic
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
