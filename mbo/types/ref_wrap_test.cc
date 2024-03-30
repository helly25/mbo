// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#include "mbo/types/ref_wrap.h"

#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::testing::Ge;
using ::testing::Le;
using ::testing::Not;
using ::testing::Pair;

struct RefWrapTest : ::testing::Test {};

TEST_F(RefWrapTest, Basics) {
  int num = 25;
  {
    RefWrap<int> ref(num);
    ASSERT_THAT(&*ref, &num);
    EXPECT_THAT(ref, 25);
    num = 42;
    EXPECT_THAT(ref, 42);
    EXPECT_THAT(num, 42);
  }
  {
    RefWrap<int> ref{num};
    ASSERT_THAT(&*ref, &num);
    EXPECT_THAT(ref, 42);
    num = 25;
    EXPECT_THAT(ref, 25);
    EXPECT_THAT(num, 25);
  }
}

TEST_F(RefWrapTest, BasicConst) {
  const int num = 25;
  RefWrap<const int> ref(num);
  ASSERT_THAT(&*ref, &num);
  EXPECT_THAT(ref, 25);
}

TEST_F(RefWrapTest, BasicConstFromNonConst) {
  int num = 25;
  RefWrap<const int> ref(num);
  ASSERT_THAT(&*ref, &num);
  EXPECT_THAT(ref, 25);
  num = 42;
  EXPECT_THAT(ref, 42);
  EXPECT_THAT(ref, Le(55));
  EXPECT_THAT(ref, Ge(33));
  num = 99;
  EXPECT_THAT(ref, 99);
  EXPECT_THAT(ref, Ge(55));
  EXPECT_THAT(ref, Ge(33));
}

TEST_F(RefWrapTest, Compare) {
  const int num = 25;
  RefWrap<const int> ref(num);
  ASSERT_THAT(&*ref, &num);
  EXPECT_THAT(ref, 25);
  EXPECT_THAT(ref <= 55, true);
  EXPECT_THAT(ref < 55, true);
  EXPECT_THAT(ref <= 11, false);
  EXPECT_THAT(ref < 11, false);
  EXPECT_THAT(ref == 55, false);
  EXPECT_THAT(ref != 55, true);
  EXPECT_THAT(ref == 25, true);
  EXPECT_THAT(ref != 25, false);
  EXPECT_THAT(55 >= ref, true);
  EXPECT_THAT(55 > ref, true);
  EXPECT_THAT(11 >= ref, false);
  EXPECT_THAT(11 > ref, false);
  EXPECT_THAT(ref, Le(55));
  EXPECT_THAT(ref, Le(25));
  EXPECT_THAT(ref, Not(Le(11)));
  EXPECT_THAT(ref, Ge(11));
  EXPECT_THAT(ref, Ge(25));
  EXPECT_THAT(ref, Not(Ge(33)));
  int val = 25;
  RefWrap<int> cmp(val);
  EXPECT_THAT(ref == val, true);
  EXPECT_THAT(ref != val, false);
  EXPECT_THAT(ref <= val, true);
  EXPECT_THAT(ref >= val, true);
  EXPECT_THAT(ref < val, false);
  EXPECT_THAT(ref > val, false);
  val = 11;
  EXPECT_THAT(ref == val, false);
  EXPECT_THAT(ref != val, true);
  EXPECT_THAT(ref <= val, false);
  EXPECT_THAT(ref >= val, true);
  EXPECT_THAT(ref < val, false);
  EXPECT_THAT(ref > val, true);
  val = 33;
  EXPECT_THAT(ref == val, false);
  EXPECT_THAT(ref != val, true);
  EXPECT_THAT(ref <= val, true);
  EXPECT_THAT(ref >= val, false);
  EXPECT_THAT(ref < val, true);
  EXPECT_THAT(ref > val, false);
}

TEST_F(RefWrapTest, Pair) {
  std::pair<int, int> data{25, 33};
  RefWrap<std::pair<int, int>> ref(data);
  ASSERT_THAT(&*ref, &data);
  ASSERT_THAT(&ref->first, &data.first);
  ASSERT_THAT(&ref->second, &data.second);
  EXPECT_THAT(*ref, Pair(data.first, data.second));
  EXPECT_THAT(ref->first, data.first);
  EXPECT_THAT(ref->second, data.second);
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
