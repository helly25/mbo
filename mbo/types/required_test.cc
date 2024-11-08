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

#include "mbo/types/required.h"

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

struct RequiredTest : ::testing::Test {};

TEST_F(RequiredTest, Basics) {
  Required<int> req(25);
  ASSERT_THAT(req, 25);
  req.emplace(33);
  EXPECT_THAT(req, 33);
}

TEST_F(RequiredTest, Compare) {
  Required<const int> req(25);
  EXPECT_THAT(req, 25);
  EXPECT_THAT(req <= 55, true);
  EXPECT_THAT(req < 55, true);
  EXPECT_THAT(req <= 11, false);
  EXPECT_THAT(req < 11, false);
  EXPECT_THAT(req == 55, false);
  EXPECT_THAT(req != 55, true);
  EXPECT_THAT(req == 25, true);
  EXPECT_THAT(req != 25, false);
  EXPECT_THAT(55 >= req, true);
  EXPECT_THAT(55 > req, true);
  EXPECT_THAT(11 >= req, false);
  EXPECT_THAT(11 > req, false);
  EXPECT_THAT(req, Le(55));
  EXPECT_THAT(req, Le(25));
  EXPECT_THAT(req, Not(Le(11)));
  EXPECT_THAT(req, Ge(11));
  EXPECT_THAT(req, Ge(25));
  EXPECT_THAT(req, Not(Ge(33)));
  int val = 25;
  Required<int> cmp(val);
  EXPECT_THAT(req == val, true);
  EXPECT_THAT(req != val, false);
  EXPECT_THAT(req <= val, true);
  EXPECT_THAT(req >= val, true);
  EXPECT_THAT(req < val, false);
  EXPECT_THAT(req > val, false);
  val = 11;
  EXPECT_THAT(req == val, false);
  EXPECT_THAT(req != val, true);
  EXPECT_THAT(req <= val, false);
  EXPECT_THAT(req >= val, true);
  EXPECT_THAT(req < val, false);
  EXPECT_THAT(req > val, true);
  val = 33;
  EXPECT_THAT(req == val, false);
  EXPECT_THAT(req != val, true);
  EXPECT_THAT(req <= val, true);
  EXPECT_THAT(req >= val, false);
  EXPECT_THAT(req < val, true);
  EXPECT_THAT(req > val, false);
}

TEST_F(RequiredTest, Pair) {
  Required<std::pair<int, int>> req({25, 33});
  EXPECT_THAT(*req, Pair(25, 33));
  EXPECT_THAT(req->first, 25);
  EXPECT_THAT(req->second, 33);
  req.emplace({42, 99});
  EXPECT_THAT(*req, Pair(42, 99));
  EXPECT_THAT(req->first, 42);
  EXPECT_THAT(req->second, 99);
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
