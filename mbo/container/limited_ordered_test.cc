// Copyright 2023 M. Boerger (helly25.com)
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

#include "mbo/container/internal/limited_ordered.h"

#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

namespace mbo::container {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::mbo::container::internal::LimitedOrdered;
using ::mbo::testing::CapacityIs;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Ne;
using ::testing::Not;
using ::testing::Pair;
using ::testing::SizeIs;

static_assert(std::ranges::range<LimitedOrdered<int, int, int, 1>>);
static_assert(std::contiguous_iterator<LimitedOrdered<int, int, int, 2>::iterator>);
static_assert(std::contiguous_iterator<LimitedOrdered<int, int, int, 3>::const_iterator>);
static_assert(std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<4, true>{}>>);
static_assert(std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<5, false>{}>>);

template <typename K = int, typename M = int, typename V = int, auto Capacity = 1>
struct LimitedOrderedTester : public LimitedOrdered<K, M, V, Capacity> {
  public:
   using typename LimitedOrdered<K, M, V, Capacity>::Data;
};

struct LimitedOrderedTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedOrderedTest, ConstexprData) {
  constexpr auto kTest = LimitedOrderedTester<>::Data{};
  EXPECT_THAT(kTest.data, 0);  // NOLINT(cppcoreguidelines-pro-type-union-access)
}

TEST_F(LimitedOrderedTest, ConstexprNoDtor) {
  constexpr auto kTest = internal::LimitedOrdered<int, int, int, LimitedOptions<3, true>{}>{};
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedOrderedTest, Constexpr) {
  constexpr auto kTest = internal::LimitedOrdered<int, int, int, LimitedOptions<3, false>{}>{};
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre());
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
