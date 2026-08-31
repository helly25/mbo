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

#include "mbo/types/optional_ref.h"

#include <compare>
#include <optional>
#include <set>
#include <string>

#include "absl/hash/hash.h"
#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

// NOLINTBEGIN(*-magic-numbers)

namespace mbo::types {
namespace {

using ::mbo::testing::IsNullopt;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Not;

struct OptionalRefTest : ::testing::Test {};

TEST_F(OptionalRefTest, Null) {
  // NOLINTNEXTLINE(misc-const-correctness): const would change decltype() in the static_assert below.
  OptionalRef<int> ref;
  static_assert(IsOptionalRef<decltype(ref)>);
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

TEST_F(OptionalRefTest, ComparisonDistinguishesEmptyAndReferencedValues) {
  int one = 1;
  int two = 2;
  const OptionalRef<int> empty;
  const OptionalRef<int> also_empty;
  const OptionalRef<int> first(one);
  const OptionalRef<int> same(one);
  const OptionalRef<int> second(two);

  EXPECT_THAT(empty == also_empty, IsTrue());
  EXPECT_THAT(empty == first, IsFalse());
  EXPECT_THAT(first == same, IsTrue());
  EXPECT_THAT(first == second, IsFalse());
  EXPECT_THAT(empty < first, IsTrue());
  EXPECT_THAT(first < empty, IsFalse());
  EXPECT_THAT(first < second, IsTrue());
  EXPECT_THAT(empty <=> also_empty, std::strong_ordering::equal);
  EXPECT_THAT(empty <=> first, std::strong_ordering::less);
  EXPECT_THAT(first <=> second, std::strong_ordering::less);
}

TEST_F(OptionalRefTest, ComparisonWithValuesAndNulloptPreservesOptionalOrdering) {
  int value = 7;
  const OptionalRef<int> empty;
  const OptionalRef<int> ref(value);

  EXPECT_THAT(empty == std::nullopt, IsTrue());
  EXPECT_THAT(ref == std::nullopt, IsFalse());
  EXPECT_THAT(empty <=> std::nullopt, std::strong_ordering::equal);
  EXPECT_THAT(ref <=> std::nullopt, std::strong_ordering::greater);
  EXPECT_THAT(empty == value, IsFalse());
  EXPECT_THAT(ref == value, IsTrue());
  EXPECT_THAT(empty < value, IsTrue());
  EXPECT_THAT(ref < value, IsFalse());
  EXPECT_THAT(empty <=> value, std::strong_ordering::less);
  EXPECT_THAT(ref <=> value, std::strong_ordering::equal);
}

TEST_F(OptionalRefTest, StringificationAndHashingReflectPresenceAndValue) {
  int value = 11;
  const OptionalRef<int> empty;
  const OptionalRef<int> ref(value);
  const OptionalRef<int> same(value);

  EXPECT_THAT(absl::StrCat(empty), "std::nullopt");
  EXPECT_THAT(absl::StrCat(ref), "11");
  EXPECT_THAT(absl::HashOf(empty) == absl::HashOf(ref), IsFalse());
  EXPECT_THAT(absl::HashOf(ref), absl::HashOf(same));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
