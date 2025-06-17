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

#include "mbo/types/optional_data_or_ref.h"

#include <concepts>
#include <optional>
#include <set>
#include <string>

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

struct OptionalDataOrRefTest : ::testing::Test {};

TEST_F(OptionalDataOrRefTest, Null) {
  OptionalDataOrRef<int> ref;
  EXPECT_THAT(ref, IsNullopt());
  EXPECT_THAT(ref.has_value(), false);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), true);
  EXPECT_THAT(ref.HoldsReference(), false);
}

TEST_F(OptionalDataOrRefTest, Value) {
  int val = 10;
  static_assert(std::same_as<int, typename OptionalDataOrRef<int>::value_type>);
  OptionalDataOrRef<int> ref(val);
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), true);
  EXPECT_THAT(ref, Not(IsNullopt()));
  EXPECT_THAT(*ref, 10);
  val = 11;
  EXPECT_THAT(ref, 11);
  EXPECT_THAT(ref, 11);
  ref = 12;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, 12);
  EXPECT_THAT(val, 11) << "We set an actual value, so val should nto change.";
  ref.set_ref(val);
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), true);
  EXPECT_THAT(ref, 11);
  EXPECT_THAT(val, 11);
  val = 13;
  EXPECT_THAT(ref, 13);
  EXPECT_THAT(val, 13);
  ref = val;
  val = 14;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), true);
  EXPECT_THAT(ref, 14) << "We were binding to a reference, so setting val updates it.";
  EXPECT_THAT(val, 14);
  int other = 15;
  ref.set_ref(other);
  EXPECT_THAT(ref, 15);
  EXPECT_THAT(val, 14) << "We did not change this.";
  ref.set_ref(val);
  EXPECT_THAT(ref, 14);
  EXPECT_THAT(val, 14);
  ref.reset();
  EXPECT_THAT(ref.has_value(), false);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), true);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, IsNullopt());
  ref = 15;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, Not(IsNullopt()));
  EXPECT_THAT(ref, Not(std::nullopt));
  EXPECT_THAT(ref, 15);
  EXPECT_THAT(val, 14);
  ref.reset();
  EXPECT_THAT(ref.has_value(), false);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), true);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, IsNullopt());
  EXPECT_THAT(ref, std::nullopt);
  val = 16;
  ref.set_ref(val);
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), true);
  EXPECT_THAT(ref, Not(IsNullopt()));
  EXPECT_THAT(ref, Not(std::nullopt));
  EXPECT_THAT(ref, 16);
  EXPECT_THAT(val, 16);
  val = 17;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), true);
  EXPECT_THAT(ref, 17);
  EXPECT_THAT(val, 17);
  EXPECT_THAT(ref.as_data(), 17);
  ref.as_data() = 18;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, 18);
  EXPECT_THAT(val, 17);
  ref.emplace(19);
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, 19);
  EXPECT_THAT(val, 17);
}

TEST_F(OptionalDataOrRefTest, DifferentType) {
  OptionalDataOrRef<std::string> ref;
  EXPECT_THAT(ref.has_value(), false);
  EXPECT_THAT(ref.HoldsData(), false);
  EXPECT_THAT(ref.HoldsNullopt(), true);
  EXPECT_THAT(ref.HoldsReference(), false);
  ref = "100";
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, "100");
  ref = std::string("200");
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, "200");
  ref = std::string_view("300");
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, "300");
  const std::string cstr{"400"};
  ref = cstr;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, "400");
  std::string str{"500"};
  ref = str;
  EXPECT_THAT(ref.has_value(), true);
  EXPECT_THAT(ref.HoldsData(), true);
  EXPECT_THAT(ref.HoldsNullopt(), false);
  EXPECT_THAT(ref.HoldsReference(), false);
  EXPECT_THAT(ref, "500");
}

TEST_F(OptionalDataOrRefTest, Compare) {
  std::set<OptionalDataOrRef<int>> refs;
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
