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

#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::testing::Ge;
using ::testing::IsEmpty;
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
  const Required<int> cmp(val);
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

TEST_F(RequiredTest, PairByArgs) {
  Required<std::pair<int, int>> req(std::in_place, 25, 33);
  EXPECT_THAT(*req, Pair(25, 33));
  EXPECT_THAT(req->first, 25);
  EXPECT_THAT(req->second, 33);
  req.emplace(42, 99);
  EXPECT_THAT(*req, Pair(42, 99));
  EXPECT_THAT(req->first, 42);
  EXPECT_THAT(req->second, 99);
}

TEST_F(RequiredTest, DefCtor) {
  Required<std::string> req;
  EXPECT_THAT(*req, IsEmpty());
}

TEST_F(RequiredTest, NoDefCtor) {
  struct NoDefCtor {
    NoDefCtor() = delete;

    explicit NoDefCtor(int v) : value(v) {}

    bool operator==(int v) const noexcept { return v == value; }

    int value;
  };

  Required<NoDefCtor> req(25);
  EXPECT_THAT(*req, 25);
}

template<typename T>
struct MoveOnly {
  ~MoveOnly() = default;
  MoveOnly() = delete;

  explicit MoveOnly(T v) : value(std::move(v)) {}

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) = delete;

  bool operator==(const T& v) const noexcept { return v == value; }

  const T value;
};

TEST_F(RequiredTest, MoveOnly) {
  {
    Required<MoveOnly<int>> req(25);
    EXPECT_THAT(*req, 25);
    req.emplace(42);
    EXPECT_THAT(*req, 42);
  }
  {
    Required<MoveOnly<std::string>> req("Good Morning America!");
    EXPECT_THAT(*req, "Good Morning America!");
    req.emplace("Good Evening Germany!");
    EXPECT_THAT(*req, "Good Evening Germany!");
  }
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
