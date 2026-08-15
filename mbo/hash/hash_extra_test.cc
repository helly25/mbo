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

#include "mbo/hash/hash_extra.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "mbo/hash/hash_internal_util.h"

// Direct tests for the NOTICE-bearing extra algorithms and the low-level load
// helpers they are built on. The full known-answer matrix lives in hash_test's
// generated vectors; here each library's own contract is pinned in its package.

namespace mbo::hash {
namespace {

struct HashExtraTest : ::testing::Test {};

TEST_F(HashExtraTest, AllExtraAlgorithmsSatisfyTheConcept) {
  static_assert(IsHashAlgorithm<rapidhash::Algorithm>);
  static_assert(IsHashAlgorithm<xxh64::Algorithm>);
  static_assert(IsHashAlgorithm<xxh3::Algorithm>);
}

TEST_F(HashExtraTest, Xxh64EmptyIsCanonical) {
  // The published XXH64 value for the empty input, seed 0.
  static_assert(xxh64::Algorithm::GetHash64("", 0) == 0xef46db3751d8e999ULL);
}

TEST_F(HashExtraTest, Xxh3EmptyIsCanonical) {
  // The published XXH3_64bits value for the empty input.
  static_assert(xxh3::Algorithm::GetHash64("", 0) == 0x2d06800538d394c2ULL);
}

TEST_F(HashExtraTest, AlgorithmsAreDeterministicAndSeedSensitive) {
  static_assert(rapidhash::Algorithm::GetHash64("hello", 0) == rapidhash::Algorithm::GetHash64("hello", 0));
  static_assert(rapidhash::Algorithm::GetHash64("hello", 0) != rapidhash::Algorithm::GetHash64("hello", 1));
  static_assert(rapidhash::Algorithm::GetHash64("hello", 0) != rapidhash::Algorithm::GetHash64("hellp", 0));
  static_assert(xxh3::Algorithm::GetHash64("hello", 0) != xxh3::Algorithm::GetHash64("hello", 1));
}

TEST_F(HashExtraTest, Xxh3Hash128LowLaneNotDegenerate) {
  constexpr Hash128 kHash = xxh3::Algorithm::GetHash128("hello", 0);
  constexpr Hash128 kOther = xxh3::Algorithm::GetHash128("world", 0);
  static_assert(kHash != kOther);
}

// hash_internal_util: the load helpers must read exactly the documented bytes.

TEST_F(HashExtraTest, LoadHelpersReadLittleEndian) {
  static_assert(hash_internal::Load32("ABCD") == 0x44434241U);
  static_assert(hash_internal::Load64("ABCDEFGH") == 0x4847464544434241ULL);
  static_assert(hash_internal::Load32BE("ABCD") == 0x41424344U);
  static_assert(hash_internal::Load64BE("ABCDEFGH") == 0x4142434445464748ULL);
}

TEST_F(HashExtraTest, LoadSmallCoversItsLengthRanges) {
  // len 8: both lanes are the same full load.
  constexpr auto kLen8 = hash_internal::LoadSmall("ABCDEFGH", 8);
  static_assert(kLen8.a == hash_internal::Load64("ABCDEFGH"));
  static_assert(kLen8.a == kLen8.b);
  // len 9..16: two 64-bit loads overlapping the end.
  constexpr auto kLen10 = hash_internal::LoadSmall("ABCDEFGHIJ", 10);
  static_assert(kLen10.a == hash_internal::Load64("ABCDEFGH"));
  static_assert(kLen10.b == hash_internal::Load64("CDEFGHIJ"));
  // len 4..7: two 32-bit loads overlapping the end.
  constexpr auto kLen5 = hash_internal::LoadSmall("ABCDE", 5);
  static_assert(kLen5.a == hash_internal::Load32("ABCD"));
  static_assert(kLen5.b == hash_internal::Load32("BCDE"));
  // len 0: zero.
  constexpr auto kLen0 = hash_internal::LoadSmall("", 0);
  static_assert(kLen0.a == 0 && kLen0.b == 0);
}

}  // namespace
}  // namespace mbo::hash
