// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "gmock/gmock.h"
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

  EXPECT_THAT(hash_internal::Load32(input.data()), 0x44434241U);
  EXPECT_THAT(hash_internal::Load64(input.data()), 0x4847464544434241ULL);
  EXPECT_THAT(hash_internal::Load32BE(input.data()), 0x41424344U);
  EXPECT_THAT(hash_internal::Load64BE(input.data()), 0x4142434445464748ULL);
}

TEST_F(HashExtraTest, LoadTailCoversEveryLength) {
  constexpr std::array<uint64_t, 9> kExpected = {
      0, 0x41, 0x4241, 0x434241, 0x44434241, 0x4544434241, 0x464544434241, 0x47464544434241, 0x4847464544434241,
  };
  const std::string input = "ABCDEFGH";
  volatile std::size_t runtime_length = 0;
  for (std::size_t length = 0; length < kExpected.size(); ++length) {
    runtime_length = length;
    EXPECT_THAT(hash_internal::LoadTail(input.data(), runtime_length), kExpected.at(length));
  }
}

TEST_F(HashExtraTest, LoadSmallCoversEveryLength) {
  struct Expected {
    uint64_t a;
    uint64_t b;
  };

  constexpr std::array<Expected, 17> kExpected = {{
      {.a = 0, .b = 0},
      {.a = 0x8200000000041, .b = 0x8200000000041},
      {.a = 0x8200000004241, .b = 0x8200000004241},
      {.a = 0x8200000004243, .b = 0x8200000004243},
      {.a = 0x44434241, .b = 0x44434241},
      {.a = 0x44434241, .b = 0x45444342},
      {.a = 0x44434241, .b = 0x46454443},
      {.a = 0x44434241, .b = 0x47464544},
      {.a = 0x4847464544434241, .b = 0x4847464544434241},
      {.a = 0x4847464544434241, .b = 0x4948474645444342},
      {.a = 0x4847464544434241, .b = 0x4a49484746454443},
      {.a = 0x4847464544434241, .b = 0x4b4a494847464544},
      {.a = 0x4847464544434241, .b = 0x4c4b4a4948474645},
      {.a = 0x4847464544434241, .b = 0x4d4c4b4a49484746},
      {.a = 0x4847464544434241, .b = 0x4e4d4c4b4a494847},
      {.a = 0x4847464544434241, .b = 0x4f4e4d4c4b4a4948},
      {.a = 0x4847464544434241, .b = 0x504f4e4d4c4b4a49},
  }};
  const std::string input = "ABCDEFGHIJKLMNOP";
  volatile std::size_t runtime_length = 0;
  for (std::size_t length = 0; length < kExpected.size(); ++length) {
    runtime_length = length;
    const auto actual = hash_internal::LoadSmall(input.data(), runtime_length);
    EXPECT_THAT(actual.a, kExpected.at(length).a);
    EXPECT_THAT(actual.b, kExpected.at(length).b);
  }
}

TEST_F(HashExtraTest, MultiplyHelpersExposeBothResultLanes) {
  const volatile uint64_t lhs = uint64_t{1} << 63U;
  const volatile uint64_t rhs = 3;
  const Hash128 product = hash_internal::Mult128(lhs, rhs);
  EXPECT_THAT(product.h1, uint64_t{1} << 63U);
  EXPECT_THAT(product.h2, 1);
  EXPECT_THAT(hash_internal::Mul128Fold64(lhs, rhs), (uint64_t{1} << 63U) | 1);
}

}  // namespace
}  // namespace mbo::hash
