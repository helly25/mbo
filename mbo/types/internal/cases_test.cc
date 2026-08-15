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

#include "mbo/types/internal/cases.h"

#include <concepts>
#include <cstddef>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types::types_internal {
namespace {

struct CasesInternalTest : ::testing::Test {};

// `CasesImpl` is a compile-time switch: the FIRST case whose `value` is true
// supplies the resulting `type`. Everything here is a static_assert; the TEST_F
// bodies exist only to group them.

TEST_F(CasesInternalTest, PicksTheOnlyTrueCase) {
  static_assert(std::same_as<CasesImpl<IfThen<true, int>>::type, int>);
  static_assert(std::same_as<CasesImpl<IfThen<false, int>>::type, void>);
}

TEST_F(CasesInternalTest, PicksTheFirstTrueCaseNotTheLast) {
  // Order is the whole semantic: a later true case must not win.
  static_assert(std::same_as<CasesImpl<IfThen<true, int>, IfThen<true, std::string>>::type, int>);
  static_assert(std::same_as<CasesImpl<IfThen<false, int>, IfThen<true, std::string>>::type, std::string>);
  static_assert(
      std::same_as<CasesImpl<IfThen<false, int>, IfThen<false, std::string>, IfThen<true, double>>::type, double>);
}

TEST_F(CasesInternalTest, YieldsVoidWhenNoCaseMatches) {
  // The two trailing IfTrueThenVoid entries make the expansion always possible, so
  // an all-false list resolves to void rather than failing to compile.
  static_assert(std::same_as<CasesImpl<IfThen<false, int>>::type, void>);
  static_assert(std::same_as<CasesImpl<IfThen<false, int>, IfThen<false, std::string>>::type, void>);
}

TEST_F(CasesInternalTest, IfElseAlwaysMatches) {
  static_assert(IfElse<int>::value);
  static_assert(std::same_as<CasesImpl<IfThen<false, double>, IfElse<int>>::type, int>);
  // Placed first it shadows everything after it, which is why it "must go last".
  static_assert(std::same_as<CasesImpl<IfElse<int>, IfThen<true, double>>::type, int>);
}

TEST_F(CasesInternalTest, VoidHelpersCarryTheirDocumentedTruth) {
  static_assert(IfTrueThenVoid::value);
  static_assert(std::same_as<IfTrueThenVoid::type, void>);
  static_assert(!IfFalseThenVoid::value);
  static_assert(std::same_as<IfFalseThenVoid::type, void>);
  // IfFalseThenVoid skips a slot: the next true case decides.
  static_assert(std::same_as<CasesImpl<IfFalseThenVoid, IfThen<true, int>>::type, int>);
}

TEST_F(CasesInternalTest, IsIfThenAcceptsOnlyCaseLikeTypes) {
  static_assert(IsIfThen<IfThen<true, int>>);
  static_assert(IsIfThen<IfElse<int>>);
  static_assert(IsIfThen<IfTrueThenVoid>);
  static_assert(IsIfThen<IfFalseThenVoid>);
  static_assert(!IsIfThen<int>);
}

TEST_F(CasesInternalTest, CaseIndexIsOneBasedAndZeroForNoMatch) {
  // `CaseIndexImpl` reports 1-based position of the winning case, and 0 when none
  // of the caller's own cases matched - so 0 is distinguishable from "case 1".
  EXPECT_THAT((CaseIndexImpl<IfThen<true, int>>::index), 1U);
  EXPECT_THAT((CaseIndexImpl<IfThen<false, int>, IfThen<true, std::string>>::index), 2U);
  EXPECT_THAT((CaseIndexImpl<IfThen<false, int>, IfThen<false, std::string>, IfThen<true, double>>::index), 3U);
  EXPECT_THAT((CaseIndexImpl<IfThen<false, int>>::index), 0U) << "no case matched";
  EXPECT_THAT((CaseIndexImpl<IfThen<false, int>, IfThen<false, std::string>>::index), 0U);
}

}  // namespace
}  // namespace mbo::types::types_internal
