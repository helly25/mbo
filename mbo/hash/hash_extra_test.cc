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

#include <cstddef>
#include <cstdint>
#include <string>

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
  const volatile std::size_t input_length = 8;
  const std::string input("ABCDEFGH", input_length);

  EXPECT_EQ(hash_internal::Load32(input.data()), 0x44434241U);
  EXPECT_EQ(hash_internal::Load64(input.data()), 0x4847464544434241ULL);
  EXPECT_EQ(hash_internal::Load32BE(input.data()), 0x41424344U);
  EXPECT_EQ(hash_internal::Load64BE(input.data()), 0x4142434445464748ULL);
}

TEST_F(HashExtraTest, LoadSmallCoversItsLengthRanges) {
  volatile std::size_t input_length = 10;
  const std::string input("ABCDEFGHIJ", input_length);

  // len 8: both lanes are the same full load.
  input_length = 8;
  const auto len8 = hash_internal::LoadSmall(input.data(), input_length);
  EXPECT_EQ(len8.a, hash_internal::Load64(input.data()));
  EXPECT_EQ(len8.a, len8.b);
  // len 9..16: two 64-bit loads overlapping the end.
  input_length = 10;
  const auto len10 = hash_internal::LoadSmall(input.data(), input_length);
  EXPECT_EQ(len10.a, hash_internal::Load64(input.data()));
  EXPECT_EQ(len10.b, hash_internal::Load64(input.data() + 2));
  // len 4..7: two 32-bit loads overlapping the end.
  input_length = 5;
  const auto len5 = hash_internal::LoadSmall(input.data(), input_length);
  EXPECT_EQ(len5.a, hash_internal::Load32(input.data()));
  EXPECT_EQ(len5.b, hash_internal::Load32(input.data() + 1));
  // len 0: zero.
  input_length = 0;
  const auto len0 = hash_internal::LoadSmall(input.data(), input_length);
  EXPECT_EQ(len0.a, 0);
  EXPECT_EQ(len0.b, 0);
}

}  // namespace
}  // namespace mbo::hash
