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

#include "mbo/types/cases.h"

#include <cstddef>
#include <type_traits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types {

using std::size_t;

namespace {

class CasesTest : public ::testing::Test {};

TEST_F(CasesTest, Cases) {
  EXPECT_TRUE((std::is_same_v<Cases<>, void>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<true, std::true_type>, IfThen<false, std::false_type>>, std::true_type>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<false, std::true_type>, IfThen<true, std::false_type>>, std::false_type>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<false, std::true_type>, IfThen<false, std::false_type>>, void>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<0, int>>, void>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<1, int>>, int>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<0, int>, IfThen<0, char>>, void>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<0, int>, IfThen<1, char>>, char>));
  EXPECT_TRUE((std::is_same_v<Cases<IfThen<1, int>, IfThen<0, char>>, int>));
}

TEST_F(CasesTest, CasesIndex) {
  EXPECT_THAT((CaseIndex<>), 0);
  EXPECT_THAT((CaseIndex<false>), 0);
  EXPECT_THAT((CaseIndex<true>), 1);
  EXPECT_THAT((CaseIndex<false, false>), 0);
  EXPECT_THAT((CaseIndex<false, true>), 2);
  EXPECT_THAT((CaseIndex<true, false>), 1);
  EXPECT_THAT((CaseIndex<true, true>), 1);
  EXPECT_THAT((CaseIndex<0, 0, 0>), 0);
  EXPECT_THAT((CaseIndex<0, 0, 1>), 3);
  EXPECT_THAT((CaseIndex<0, 1, 0>), 2);
  EXPECT_THAT((CaseIndex<0, 1, 1>), 2);
  EXPECT_THAT((CaseIndex<1, 0, 0>), 1);
  EXPECT_THAT((CaseIndex<1, 0, 1>), 1);
  EXPECT_THAT((CaseIndex<1, 1, 0>), 1);
  EXPECT_THAT((CaseIndex<1, 1, 1>), 1);
}

TEST_F(CasesTest, Else) {
  using Case0 = std::integral_constant<size_t, 0>;
  using Case1 = std::integral_constant<size_t, 1>;
  using Case2 = std::integral_constant<size_t, 2>;
  EXPECT_THAT((Cases<IfThen<false, Case0>, IfThen<false, Case1>, IfElse<Case2>>::value), 2);
  EXPECT_THAT((Cases<IfThen<false, Case0>, IfThen<true, Case1>, IfElse<Case2>>::value), 1);
  EXPECT_THAT((Cases<IfThen<true, Case0>, IfThen<false, Case1>, IfElse<Case2>>::value), 0);
}

}  // namespace
}  // namespace mbo::types
