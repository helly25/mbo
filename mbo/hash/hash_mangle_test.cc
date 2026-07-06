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

#include "mbo/hash/hash_mangle.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/hash/hash.h"
#include "mbo/hash/hash_test_util.h"

namespace mbo::hash {
namespace {

using ::testing::Conditional;
using ::testing::Eq;
using ::testing::Ne;

// NOLINTBEGIN(*-magic-numbers)

constexpr uint64_t kSeed = 0x0EAF00D0BADC0DE5ULL;

// A fully transparent algorithm: its output is trivially predictable, so the
// mangle pipeline (Hasher plumbing, seed defaulting, the XOR itself) can be
// pinned value-for-value in every flag configuration.
struct DummyAlgorithm {
  static constexpr uint64_t GetHash64(std::string_view data, uint64_t seed) noexcept { return seed + data.size(); }
};

static_assert(IsHashAlgorithm<DummyAlgorithm>);
static_assert((GetHash<DummyAlgorithm, 5>("ab")) == ((5ULL + 2ULL) ^ kMangleConstant));

// The mangled entry points stay constexpr.
static_assert(GetHash("constexpr") == HashMangle(GetHash64("constexpr")));

struct HashMangleTest : ::testing::Test {};

// Typed sweep over ALL registered algorithms (core and hash_extra alike -
// tests do not ship, so the NOTICE-driven target split does not apply here):
// the mangle must compose with every `IsHashAlgorithm`. Same tuple-to-Types
// derivation as hash_test.cc, from the central `algo::AllAlgorithms` registry.
template<typename Algo>
struct MangleAlgoTest : ::testing::Test {};

template<typename Tuple>
struct TupleToGtestTypes;

template<typename... Ts>
struct TupleToGtestTypes<std::tuple<Ts...>> {
  using type = ::testing::Types<Ts...>;
};

using MangleAlgorithms = TupleToGtestTypes<algo::AllAlgorithms>::type;
TYPED_TEST_SUITE(MangleAlgoTest, MangleAlgorithms);

TYPED_TEST(MangleAlgoTest, MangleComposesWithEveryAlgorithm) {
  const uint64_t plain = Hasher<TypeParam>::GetHash64("mangle-sweep", kSeed);
  EXPECT_THAT(MangledHasher<TypeParam>::GetHash("mangle-sweep", kSeed), Eq(HashMangle(plain)));
  EXPECT_THAT(MangledHasher<TypeParam>::GetHash("mangle-sweep", kSeed), Eq(plain ^ kMangleConstant));
  EXPECT_THAT(MangledHasher<TypeParam>{}("mangle-sweep"), Eq(GetHash<TypeParam>("mangle-sweep")));
  // Enabled <=> the mangled value differs from the plain one (see
  // MangleEngagesExactlyWhenEnabled).
  constexpr bool kEnabled = kMangleConstant != 0;
  EXPECT_THAT(MangledHasher<TypeParam>::GetHash("mangle-sweep", kSeed), Conditional(kEnabled, Ne(plain), Eq(plain)));
}

TEST_F(HashMangleTest, IsConstantXorMask) {
  // HashMangle(x) == x ^ constant, so XORing two mangled values cancels it.
  EXPECT_THAT(HashMangle(0) ^ HashMangle(1), Eq(0ULL ^ 1ULL));
  EXPECT_THAT(HashMangle(0x1111) ^ HashMangle(0x2222), Eq(0x1111ULL ^ 0x2222ULL));
  // The constant is the flag-selected one from `hash_mangle_seed_gen.h`.
  EXPECT_THAT(HashMangle(0), Eq(kMangleConstant));
  EXPECT_THAT(HashMangle(0x0123456789ABCDEFULL), Eq(0x0123456789ABCDEFULL ^ kMangleConstant));
}

TEST_F(HashMangleTest, GetHashAppliesMangle) {
  EXPECT_THAT(GetHash("hash-mangle-check"), Eq(HashMangle(GetHash64("hash-mangle-check"))));
  EXPECT_THAT(GetHash("hash-mangle-check"), Eq(GetHash64("hash-mangle-check") ^ kMangleConstant));
}

TEST_F(HashMangleTest, MangleEngagesExactlyWhenEnabled) {
  // `mangle_seed_gen` guarantees `--//mbo/hash:mangle_seed_buckets=0` <=>
  // `kMangleConstant == 0` (any enabled bucket count yields a nonzero
  // constant), so each configuration verifies its own direction end to end.
  // CI runs this test with buckets=0 in addition to the default configuration
  // (see .github/workflows/test.yml).
  constexpr bool kEnabled = kMangleConstant != 0;
  EXPECT_THAT(
      GetHash("reproducible"), Conditional(kEnabled, Ne(GetHash64("reproducible")), Eq(GetHash64("reproducible"))));
  EXPECT_THAT(GetHash(""), Conditional(kEnabled, Ne(GetHash64("")), Eq(GetHash64(""))));
  EXPECT_THAT(
      DefaultMangledHasher::GetHash("reproducible"),
      Conditional(kEnabled, Ne(GetHash64("reproducible")), Eq(GetHash64("reproducible"))));
}

TEST_F(HashMangleTest, DummyAlgorithmManglesExactly) {
  // With a transparent algorithm the mangled result is exactly the predicted
  // value: nothing but the configured XOR happens on top of the raw hash.
  EXPECT_THAT((GetHash<DummyAlgorithm, 100>("abc")), Eq((100ULL + 3ULL) ^ kMangleConstant));
  EXPECT_THAT((GetHash<DummyAlgorithm, 100>("")), Eq(100ULL ^ kMangleConstant));
  EXPECT_THAT(GetHash<DummyAlgorithm>("abcd"), Eq((kDefaultSeed + 4ULL) ^ kMangleConstant));
  EXPECT_THAT(MangledHasher<DummyAlgorithm>::GetHash("abcde", 7), Eq((7ULL + 5ULL) ^ kMangleConstant));
  EXPECT_THAT(MangledHasher<DummyAlgorithm>{}("abc"), Eq((kDefaultSeed + 3ULL) ^ kMangleConstant));
}

TEST_F(HashMangleTest, MangledNeverEqualsUnmangledWhileEnabled) {
  // `mangle_seed_gen` never yields constant 0 for an enabled bucket count, and
  // a nonzero XOR mask changes EVERY value - so with mangling enabled,
  // `GetHash` can never equal `GetHash64`, for any input or seed. Disabled,
  // they are identical. Swept over lengths/seeds via the default algorithm
  // and the transparent dummy.
  constexpr bool kEnabled = kMangleConstant != 0;
  for (std::size_t len = 0; len <= 64; ++len) {
    SCOPED_TRACE(len);
    const std::string data(len, static_cast<char>('a' + (len % 26)));
    EXPECT_THAT(GetHash(data), Conditional(kEnabled, Ne(GetHash64(data)), Eq(GetHash64(data))));
    EXPECT_THAT(
        MangledHasher<DummyAlgorithm>::GetHash(data, len),
        Conditional(
            kEnabled, Ne(Hasher<DummyAlgorithm>::GetHash64(data, len)),
            Eq(Hasher<DummyAlgorithm>::GetHash64(data, len))));
  }
}

TEST_F(HashMangleTest, TopLevelGetHashIsPluggable) {
  // Any algorithm struct can replace the default, with the seed as a template
  // constant (parens guard the macro comma).
  EXPECT_THAT(GetHash("top"), Eq(HashMangle(mumbo::GetHash64("top", kDefaultSeed))));
  EXPECT_THAT((GetHash<fnv1a::Algorithm, 999>("plug")), Eq(HashMangle(fnv1a::GetHash64("plug", 999))));
  EXPECT_THAT(GetHash("plug"), Ne(GetHash<fnv1a::Algorithm, 999>("plug")));
  // A different algorithm yields a different mangled value.
  EXPECT_THAT(GetHash("plug"), Ne(GetHash<murmur3::Algorithm>("plug")));
}

TEST_F(HashMangleTest, MangledHasherStaticGetHashIsMangled64) {
  EXPECT_THAT(
      MangledHasher<fnv1a::Algorithm>::GetHash("mangle", kSeed), Eq(HashMangle(fnv1a::GetHash64("mangle", kSeed))));
  EXPECT_THAT(DefaultMangledHasher::GetHash("mangle"), Eq(GetHash("mangle")));
}

TEST_F(HashMangleTest, MangledHasherFunctorManglesAndInheritsDeterministicInterface) {
  constexpr DefaultMangledHasher kHasher;
  EXPECT_THAT(kHasher("value"), Eq(GetHash("value")));
  // The deterministic `Hasher` interface stays available unmangled.
  EXPECT_THAT(DefaultMangledHasher::GetHash64("value"), Eq(GetHash64("value")));
  EXPECT_THAT(DefaultMangledHasher::GetHash128("value"), Eq(GetHash128<DefaultHashAlgorithm>("value")));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
