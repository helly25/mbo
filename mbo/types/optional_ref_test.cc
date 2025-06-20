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

#include "mbo/types/optional_ref.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

// NOLINTBEGIN(*-magic-numbers)

namespace mbo::types {
namespace {

using ::mbo::testing::IsNullopt;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Not;

struct OptionalRefTest : ::testing::Test {};

TEST_F(OptionalRefTest, Null) {
  OptionalRef<int> ref;
  EXPECT_THAT(ref, IsNullopt());
}

TEST_F(OptionalRefTest, Value) {
  int val = 25;
  OptionalRef<int> ref(val);
  EXPECT_THAT(ref, Not(IsNullopt()));
  EXPECT_THAT(*ref, 25);
  val = 33;
  EXPECT_THAT(ref, 33);
  EXPECT_THAT(ref, 33);
  ref = 42;
  EXPECT_THAT(ref, 42);
  EXPECT_THAT(val, 42);
  int other = 55;
  ref.set_ref(other);
  EXPECT_THAT(ref, 55);
  EXPECT_THAT(val, 42);
  ref.set_ref(val);
  EXPECT_THAT(ref, 42);
  EXPECT_THAT(val, 42);
  ref.reset();
  EXPECT_THAT(ref, IsNullopt());
  EXPECT_DEATH(ref = 99, "No value set for:.*OptionalRef");
}

TEST_F(OptionalRefTest, Compare) {
  std::set<OptionalRef<int>> refs;
  int v25 = 25;
  refs.emplace(v25);
  int v33 = 33;
  refs.emplace(v33);
  refs.emplace(std::nullopt);
  EXPECT_THAT(refs, Contains(IsNullopt()));
  EXPECT_THAT(refs, ElementsAre(IsNullopt(), 25, 33));
  EXPECT_THAT(refs, Contains(std::nullopt));
  EXPECT_THAT(refs, ElementsAre(std::nullopt, 25, 33));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
