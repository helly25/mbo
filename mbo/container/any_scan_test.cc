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

#include "mbo/container/any_scan.h"

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

static_assert(std::input_iterator<AnyScan<int, std::ptrdiff_t>::const_iterator>);
static_assert(!std::forward_iterator<AnyScan<int, std::ptrdiff_t>::const_iterator>);
static_assert(!mbo::types::ContainerIsForwardIteratable<AnyScan<int, std::ptrdiff_t>>);

struct AnyScanTest : public ::testing::Test {};

TEST_F(AnyScanTest, TestArray) {
  std::array<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestInitializerList) {
  std::initializer_list<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestLimitedSet) {
  LimitedSet<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestLimitedVector) {
  LimitedVector<int, 3> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestList) {
  std::list<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestSet) {
  std::set<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, TestVector) {
  std::vector<int> data{1, 2, 3};
  EXPECT_THAT(MakeAnyScan(data), ElementsAre(1, 2, 3));
}

template<typename T>
std::vector<T> Tester(const AnyScan<T>& span) {
  return {span.begin(), span.end()};
}

TEST_F(AnyScanTest, CallFunction) {
  EXPECT_THAT(Tester(MakeAnyScan(std::array<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnyScan(LimitedSet<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnyScan(LimitedVector<int, 3>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnyScan(std::list<int>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnyScan(std::set<int>{1, 2, 3})), ElementsAre(1, 2, 3));
  EXPECT_THAT(Tester(MakeAnyScan(std::vector<int>{1, 2, 3})), ElementsAre(1, 2, 3));
}

TEST_F(AnyScanTest, CallFunctionString) {
  EXPECT_THAT(Tester(MakeAnyScan(std::array<std::string, 2>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnyScan(LimitedSet<std::string, 3>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnyScan(LimitedVector<std::string, 3>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnyScan(std::list<std::string>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnyScan(std::set<std::string>{"a", "b"})), ElementsAre("a", "b"));
  EXPECT_THAT(Tester(MakeAnyScan(std::vector<std::string>{"a", "b"})), ElementsAre("a", "b"));
}

}  // namespace
}  // namespace mbo::container
