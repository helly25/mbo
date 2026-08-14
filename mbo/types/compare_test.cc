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

#include "mbo/types/compare.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers)

struct CompareTest : ::testing::Test {};

// As a named concept the substitution failure stays in the immediate context and
// yields false; the same requires-expression written inline in a static_assert is a
// hard error for a type that lacks the member.
template<typename T>
concept HasIsTransparent = requires { typename T::is_transparent; };

// CompareLess -----------------------------------------------------------------

TEST_F(CompareTest, CompareLessOrdersLikeStdLess) {
  constexpr CompareLess<int> kLess;
  EXPECT_THAT(kLess(1, 2), true);
  EXPECT_THAT(kLess(2, 1), false);
  EXPECT_THAT(kLess(2, 2), false) << "equal is not less";
}

TEST_F(CompareTest, CompareLessOffersThreeWayCompare) {
  constexpr CompareLess<int> kLess;
  EXPECT_THAT(kLess.Compare(1, 2), std::strong_ordering::less);
  EXPECT_THAT(kLess.Compare(2, 1), std::strong_ordering::greater);
  EXPECT_THAT(kLess.Compare(2, 2), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareLessComparesHeterogeneously) {
  // The `other_type` overloads: a `long` set compared against `int` and back.
  constexpr CompareLess<long> kLess;  // NOLINT(google-runtime-int)
  EXPECT_THAT(kLess(1L, 2), true) << "value_type on the left, other on the right";
  EXPECT_THAT(kLess(2, 1L), false) << "other on the left, value_type on the right";
  EXPECT_THAT(kLess.Compare(1L, 2), std::strong_ordering::less);
  EXPECT_THAT(kLess.Compare(2, 1L), std::strong_ordering::greater);
}

TEST_F(CompareTest, CompareLessIsTransparent) {
  // Declaring `is_transparent` is what lets a container REACH the heterogeneous
  // overloads above; without it every lookup has to build a `value_type` first.
  static_assert(HasIsTransparent<CompareLess<int>>);
  // std::less<T> is not transparent; only std::less<void> is.
  static_assert(!HasIsTransparent<std::less<int>>);
  static_assert(HasIsTransparent<std::less<>>);
}

TEST_F(CompareTest, IsCompareLessAndIsStdLessIdentifyTheirComparators) {
  static_assert(IsCompareLess<CompareLess<int>>);
  static_assert(!IsCompareLess<std::less<int>>);
  static_assert(!IsCompareLess<int>);
  static_assert(IsStdLess<std::less<int>>);
  static_assert(!IsStdLess<CompareLess<int>>);
  static_assert(!IsStdLess<int>);
}

// CompareFloat ----------------------------------------------------------------

TEST_F(CompareTest, CompareFloatOrdersOrdinaryValues) {
  EXPECT_THAT(CompareFloat(1.0, 2.0), std::strong_ordering::less);
  EXPECT_THAT(CompareFloat(2.0, 1.0), std::strong_ordering::greater);
  EXPECT_THAT(CompareFloat(2.0, 2.0), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareFloatTreatsZeroSignsAsEquivalent) {
  // `-0.0 <=> 0.0` is `equivalent`, not `unordered`, so no NaN branch is taken.
  EXPECT_THAT(CompareFloat(-0.0, 0.0), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareFloatGivesNanATotalOrder) {
  // The point of returning `strong_ordering` for floats: `<=>` yields `unordered`
  // for NaN, which cannot be used to sort. Here NaN sorts after every number, and
  // two NaNs are equivalent - so a container ordered by this cannot be corrupted by
  // a NaN the way a raw `<` would corrupt it.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THAT(CompareFloat(nan, 1.0), std::strong_ordering::greater);
  EXPECT_THAT(CompareFloat(1.0, nan), std::strong_ordering::less);
  EXPECT_THAT(CompareFloat(nan, nan), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareFloatOrdersInfinities) {
  const double inf = std::numeric_limits<double>::infinity();
  EXPECT_THAT(CompareFloat(-inf, inf), std::strong_ordering::less);
  EXPECT_THAT(CompareFloat(inf, 1.0), std::strong_ordering::greater);
  EXPECT_THAT(CompareFloat(inf, inf), std::strong_ordering::equal);
}

// CompareScalar: mixed signedness ---------------------------------------------
//
// These are the branches worth testing. A plain `lhs <=> rhs` between a signed and
// an unsigned type converts the signed one, so `-1 < 1u` comes out FALSE. The
// helper exists to give the mathematically correct answer instead.

TEST_F(CompareTest, CompareScalarNegativeIsLessThanAnyUnsigned) {
  EXPECT_THAT(CompareScalar(-1, 1U), std::strong_ordering::less) << "the case a raw <=> gets wrong";
  EXPECT_THAT(CompareScalar(1U, -1), std::strong_ordering::greater) << "and its mirror";
  EXPECT_THAT(CompareScalar(std::numeric_limits<int>::min(), 0U), std::strong_ordering::less);
}

TEST_F(CompareTest, CompareScalarUnsignedAboveSignedMaxIsGreater) {
  constexpr auto kIntMax = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
  EXPECT_THAT(CompareScalar(std::numeric_limits<std::int32_t>::max(), kIntMax + 1), std::strong_ordering::less);
  EXPECT_THAT(CompareScalar(kIntMax + 1, std::numeric_limits<std::int32_t>::max()), std::strong_ordering::greater);
}

TEST_F(CompareTest, CompareScalarSameSignednessCompesNormally) {
  EXPECT_THAT(CompareScalar(1, 2), std::strong_ordering::less);
  EXPECT_THAT(CompareScalar(2U, 1U), std::strong_ordering::greater);
  EXPECT_THAT(CompareScalar(2, 2), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareScalarHandlesBool) {
  EXPECT_THAT(CompareScalar(false, true), std::strong_ordering::less);
  EXPECT_THAT(CompareScalar(true, false), std::strong_ordering::greater);
  // A bool against a number compares as bools: any non-zero is `true`.
  EXPECT_THAT(CompareScalar(true, 2), std::strong_ordering::equal);
  EXPECT_THAT(CompareScalar(false, 0), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareScalarMixesFloatAndIntegral) {
  EXPECT_THAT(CompareScalar(1, 2.5), std::strong_ordering::less);
  EXPECT_THAT(CompareScalar(2.5, 1), std::strong_ordering::greater);
  EXPECT_THAT(CompareScalar(2.0, 2), std::strong_ordering::equal);
}

TEST_F(CompareTest, CompareArithmeticAndIntegralAgreeWithCompareScalar) {
  EXPECT_THAT(CompareArithmetic(-1, 1U), std::strong_ordering::less);
  EXPECT_THAT(CompareArithmetic(1.5, 2), std::strong_ordering::less);
  EXPECT_THAT(CompareIntegral(-1, 1U), std::strong_ordering::less);
  EXPECT_THAT(CompareIntegral(2, 2), std::strong_ordering::equal);
}

// WeakToStrong ----------------------------------------------------------------

TEST_F(CompareTest, WeakToStrongMapsEveryWeakOrdering) {
  EXPECT_THAT(WeakToStrong(std::weak_ordering::less), std::strong_ordering::less);
  EXPECT_THAT(WeakToStrong(std::weak_ordering::greater), std::strong_ordering::greater);
  EXPECT_THAT(WeakToStrong(std::weak_ordering::equivalent), std::strong_ordering::equal);
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
