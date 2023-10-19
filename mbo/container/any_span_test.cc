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

#include "mbo/container/any_span.h"

#include <array>
#include <initializer_list>
#include <iterator>
#include <list>
#include <set>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/container/limited_set.h"
#include "mbo/container/limited_vector.h"
#include "mbo/types/traits.h"

namespace mbo::container {
namespace {

using ::testing::ElementsAre;

static_assert(std::input_iterator<AnySpan<int, std::ptrdiff_t>::const_iterator>);
static_assert(!std::forward_iterator<AnySpan<int, std::ptrdiff_t>::const_iterator>);
static_assert(!mbo::types::ContainerIsForwardIteratable<AnySpan<int, std::ptrdiff_t>>);

struct AnySpanTest : public ::testing::Test {};

TEST_F(AnySpanTest, TestArray) {
  std::array<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestInitializerList) {
  std::initializer_list<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestLimitedSet) {
  LimitedSet<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestLimitedVector) {
  LimitedVector<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestList) {
  std::list<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestSet) {
  std::set<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, TestVector) {
  std::vector<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnySpan(data), ElementsAre(1, 2, 3));
}

template<typename T>
std::vector<T> Tester(const AnySpan<T>& span) {
  return {span.begin(), span.end()};
}

TEST_F(AnySpanTest, CallFunction) {
  EXPECT_THAT(Tester(MakeAnySpan(std::array<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnySpan(LimitedSet<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnySpan(LimitedVector<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnySpan(std::list<int>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnySpan(std::set<int>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnySpan(std::vector<int>{1, 2, 3})), ElementsAre(1, 2, 3));
}

TEST_F(AnySpanTest, CallFunctionString) {
  EXPECT_THAT(Tester(MakeAnySpan(std::array<std::string, 2>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnySpan(LimitedSet<std::string, 3>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnySpan(LimitedVector<std::string, 3>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnySpan(std::list<std::string>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnySpan(std::set<std::string>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnySpan(std::vector<std::string>{"a", "b"})), ElementsAre("a", "b"));
}

}  // namespace
}  // namespace mbo::container
