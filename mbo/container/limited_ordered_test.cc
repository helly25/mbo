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

#include "mbo/container/internal/limited_ordered.h"

#include <ranges>       // IWYU pragma: keep
#include <string>       // IWYU pragma: keep
#include <string_view>  // IWYU pragma: keep
#include <type_traits>  // IWYU pragma: keep

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

namespace mbo::container {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::mbo::container::container_internal::LimitedOrdered;
using ::mbo::testing::CapacityIs;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;

static_assert(std::ranges::range<LimitedOrdered<int, int, int, 1>>);
static_assert(std::contiguous_iterator<LimitedOrdered<int, int, int, 2>::iterator>);
static_assert(std::contiguous_iterator<LimitedOrdered<int, int, int, 3>::const_iterator>);
static_assert(IsLimitedOptions<LimitedOptions<4>>);
static_assert(std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<4>{}>>);
static_assert(IsLimitedOptions<LimitedOptions<5, LimitedOptionsFlag::kDefault>>);
static_assert(std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<5, LimitedOptionsFlag::kDefault>{}>>);
static_assert(IsLimitedOptions<LimitedOptions<6, LimitedOptionsFlag::kEmptyDestructor>>);
static_assert(
    std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<6, LimitedOptionsFlag::kEmptyDestructor>{}>>);
static_assert(IsLimitedOptions<LimitedOptions<7, LimitedOptionsFlag::kRequireSortedInput>>);
static_assert(
    std::ranges::range<LimitedOrdered<int, int, int, LimitedOptions<7, LimitedOptionsFlag::kRequireSortedInput>{}>>);
static_assert(
    IsLimitedOptions<LimitedOptions<8, LimitedOptionsFlag::kEmptyDestructor, LimitedOptionsFlag::kRequireSortedInput>>);
static_assert(std::ranges::range<LimitedOrdered<
                  int,
                  int,
                  int,
                  LimitedOptions<8, LimitedOptionsFlag::kEmptyDestructor, LimitedOptionsFlag::kRequireSortedInput>{}>>);

template<typename K = int, typename M = int, typename V = int, auto Capacity = 1>
struct LimitedOrderedTester : public LimitedOrdered<K, M, V, Capacity> {
 public:
  using typename LimitedOrdered<K, M, V, Capacity>::Data;
};

struct LimitedOrderedTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedOrderedTest, ConstexprData) {
#ifndef NDEBUG
  // In opt mode the compiler won't initialize the int.
  constexpr auto kTest = LimitedOrderedTester<>::Data{};
  EXPECT_THAT(kTest.data, 0);  // NOLINT(cppcoreguidelines-pro-type-union-access)
#endif
}

TEST_F(LimitedOrderedTest, ConstexprNoDtor) {
  static constexpr auto kTest =
      LimitedOrdered<int, int, int, LimitedOptions<3, LimitedOptionsFlag::kEmptyDestructor>{}>{};
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedOrderedTest, ConstexprRequireSortedInput) {
  static constexpr auto kTest =
      LimitedOrdered<int, int, int, LimitedOptions<3, LimitedOptionsFlag::kRequireSortedInput>{}>{};
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre());
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
