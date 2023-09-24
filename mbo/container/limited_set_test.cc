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

#include "mbo/container/limited_set.h"

#include <iostream>

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::container {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Not;
using ::testing::Pair;
using ::testing::SizeIs;

MATCHER_P(Capacity, cap, "Capacity is") {
  return arg.capacity() == static_cast<std::size_t>(cap);
}

struct LimitedSetTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedSetTest, MakeNoArg) {
  constexpr auto kTest = MakeLimitedSet<int>();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, Capacity(0));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedSetTest, MakeOneArg) {
  constexpr auto kTest = MakeLimitedSet(42);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(1));
  EXPECT_THAT(kTest, Capacity(1));
  EXPECT_THAT(kTest, ElementsAre(42));
}

TEST_F(LimitedSetTest, MakeInitArgFind) {
  constexpr auto kTest = MakeLimitedSet<5>({1, 3, 5});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, Capacity(5));
  EXPECT_THAT(kTest, ElementsAre(1, 3, 5));
  EXPECT_THAT(kTest.find(1) - kTest.begin(), 0);
  EXPECT_THAT(kTest.find(3) - kTest.begin(), 1);
  EXPECT_THAT(kTest.find(5) - kTest.begin(), 2);
  EXPECT_THAT(kTest.find(0), kTest.end());
  EXPECT_THAT(kTest.find(2), kTest.end());
}

TEST_F(LimitedSetTest, MakeInitArgBasics) {
  auto test = MakeLimitedSet<7>({1, 3, 5});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, Capacity(7));
  EXPECT_THAT(test, ElementsAre(1, 3, 5));
  EXPECT_THAT(test.find(1), test.begin());
  EXPECT_THAT(test.find(1) - test.begin(), 0);
  EXPECT_THAT(test.find(3), test.begin() + 1);
  EXPECT_THAT(test.find(3) - test.begin(), 1);
  EXPECT_THAT(test.find(5), test.begin() + 2);
  EXPECT_THAT(test.find(5), test.end() - 1);
  EXPECT_THAT(test.find(5) - test.begin(), 2);
  EXPECT_THAT(test.find(0), test.end());
  EXPECT_THAT(test.emplace(0), Pair(test.begin(), true));
  EXPECT_THAT(test, ElementsAre(0, 1, 3, 5));
  EXPECT_THAT(test.find(2), test.end());
  EXPECT_THAT(test.emplace(2), Pair(test.begin() + 2, true));
  EXPECT_THAT(test, ElementsAre(0, 1, 2, 3, 5));
  EXPECT_THAT(test.find(6), test.end());
  EXPECT_THAT(test.emplace(6), Pair(test.end(), true));
  EXPECT_THAT(test, ElementsAre(0, 1, 2, 3, 5, 6));
  EXPECT_THAT(test.find(4), test.end());
  EXPECT_THAT(test.emplace(4), Pair(test.begin() + 4, true));
  EXPECT_THAT(test, ElementsAre(0, 1, 2, 3, 4, 5, 6));
  EXPECT_THAT(test.find(0), test.begin());
  EXPECT_THAT(test.find(1), test.begin() + 1);
  EXPECT_THAT(test.find(2), test.begin() + 2);
  EXPECT_THAT(test.find(3), test.begin() + 3);
  EXPECT_THAT(test.find(4), test.begin() + 4);
  EXPECT_THAT(test.find(5), test.begin() + 5);
  EXPECT_THAT(test.find(6), test.begin() + 6);
}

TEST_F(LimitedSetTest, MakeInitWithDuplicates) {
  auto test = MakeLimitedSet<3>({1, 3, 3, 3, 5});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, Capacity(3)) << "There are duplicates, and so the construction with M=3 works.";
  EXPECT_THAT(test, ElementsAre(1, 3, 5));
}

TEST_F(LimitedSetTest, MakeInitArg) {
  constexpr auto kTest = MakeLimitedSet<3>({1, 2, 0});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, Capacity(3));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedSetTest, MakeInitArgLarger) {
  constexpr auto kTest = MakeLimitedSet<5>({1, 0, 2});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, Capacity(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedSetTest, MakeMultiArg) {
  constexpr auto kTest = MakeLimitedSet(0, 3, 2, 1);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, Capacity(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedSetTest, CustomCompare) {
  constexpr auto kTest = MakeLimitedSet<4>({0, 3, 2, 1}, std::greater());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, Capacity(4));
  EXPECT_THAT(kTest, ElementsAre(3, 2, 1, 0));
}

TEST_F(LimitedSetTest, MakeIteratorArg) {
  constexpr std::array<int, 4> kVec{0, 1, 2, 3};
  constexpr auto kTest = MakeLimitedSet<5>(kVec.begin(), kVec.end());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, Capacity(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedSetTest, MakeWithStrings) {
  const std::vector<std::string> data{{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = MakeLimitedSet<4>(data.begin(), data.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, Capacity(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
}

TEST_F(LimitedSetTest, ConstructAssignFromSmaller) {
  {
    constexpr LimitedSet<unsigned, 3> kSource({0U, 1U, 2U});
    LimitedSet<int, 5> target(kSource);
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    constexpr LimitedSet<unsigned, 3> kSource({0U, 1U, 2U});
    LimitedSet<int, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = kSource;
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    LimitedSet<unsigned, 4> source({0U, 1U, 2U});
    LimitedSet<int, 5> target(std::move(source));
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    LimitedSet<unsigned, 3> source({0U, 1U, 2U});
    LimitedSet<int, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = std::move(source);
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
}

TEST_F(LimitedSetTest, ToLimitedSet) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  constexpr int kArray[4] = {0, 1, 2, 3};
  constexpr auto kTest = ToLimitedSet(kArray);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, Capacity(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedSetTest, ToLimitedSetStringCopy) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  const std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedSet(array);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, Capacity(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedSetTest, ToLimitedSetStringMove) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedSet(std::move(array));
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, Capacity(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedSetTest, ConstexprMakeClear) {
  constexpr auto kTest = [] {
    auto test = MakeLimitedSet<5>({0, 1, 2});
    test.clear();
    return test;
  }();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, Capacity(5));
  EXPECT_THAT(kTest, ElementsAre());
}


TEST_F(LimitedSetTest, Erase) {
  auto test = MakeLimitedSet(0, 1, 2, 3, 4);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, Capacity(5));
  ASSERT_THAT(test, ElementsAre(0, 1, 2, 3, 4));
  EXPECT_THAT(test.erase(test.begin() + 2), test.begin() + 2);
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, ElementsAre(0, 1, 3, 4));
  EXPECT_THAT(test.erase(test.end() - 1), test.begin() + 3);
  EXPECT_THAT(test.begin() + 3, test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, ElementsAre(0, 1, 3));
  EXPECT_THAT(test.erase(1), 1);
  EXPECT_THAT(test.erase(1), 0);
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, ElementsAre(0, 3));
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.begin(), test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, IsEmpty());
}

TEST_F(LimitedSetTest, Contains) {
  constexpr auto kTest = MakeLimitedSet<6>({0, 1, 2, 3});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, Capacity(6));
  ASSERT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  EXPECT_THAT(kTest.contains(0), true);
  EXPECT_THAT(kTest.contains(4), false);
}

TEST_F(LimitedSetTest, Insert) {
  auto test = MakeLimitedSet<6>({0, 3});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, Capacity(6));
  ASSERT_THAT(test, ElementsAre(0, 3));
  const std::vector<int> other{1, 2, 4};
  test.insert(other.begin(), other.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, Capacity(6));
  ASSERT_THAT(test, ElementsAre(0, 1, 2, 3, 4));
}

TEST_F(LimitedSetTest, Swap) {
  auto test1 = MakeLimitedSet<int>(0, 1, 2);
  auto test2 = LimitedSet<int, 3>({3});
  ASSERT_THAT(test1, ElementsAre(0, 1, 2));
  ASSERT_THAT(test2, ElementsAre(3));
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre(3));
  EXPECT_THAT(test2, ElementsAre(0, 1, 2));
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre(0, 1, 2));
  EXPECT_THAT(test2, ElementsAre(3));
  test2.clear();
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre());
  EXPECT_THAT(test2, ElementsAre(0, 1, 2));
  test2.clear();
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre());
  EXPECT_THAT(test2, ElementsAre());
}

TEST_F(LimitedSetTest, Iterators) {
  constexpr auto kTest = MakeLimitedSet(0, 1, 2);
  // Restrictions apply: The two following cannot be constexpr.
  EXPECT_THAT((MakeLimitedSet<3>(kTest.begin(), kTest.end())), ElementsAre(0, 1, 2));
  EXPECT_THAT((MakeLimitedSet<3>(kTest.rbegin(), kTest.rend())), ElementsAre(0, 1, 2));
}

TEST_F(LimitedSetTest, Compare) {
  constexpr auto k42v65 = MakeLimitedSet(42, 65);
  constexpr auto k42o65 = MakeLimitedSet(42, 65);
  constexpr auto k42v99 = MakeLimitedSet(42, 99);
  constexpr auto k42 = MakeLimitedSet(42);
  EXPECT_THAT(k42v65 == k42o65, true);
  EXPECT_THAT(k42v65, k42o65);
  EXPECT_THAT(k42v65, Eq(k42o65));

  EXPECT_THAT(k42v65 != k42v99, true);
  EXPECT_THAT(k42v65, Not(k42v99));
  EXPECT_THAT(k42v65, Not(Eq(k42v99)));
  EXPECT_THAT(k42v65 != k42, true);
  EXPECT_THAT(k42v65, Not(k42));
  EXPECT_THAT(k42v65, Not(Eq(k42)));

  EXPECT_THAT(k42v65 < k42v99, true);
  EXPECT_THAT(k42v65, Lt(k42v99));
  EXPECT_THAT(k42 < k42v99, true);
  EXPECT_THAT(k42, Lt(k42v99));
  EXPECT_THAT(k42v99 < k42v65, false);
  EXPECT_THAT(k42v99, Not(Lt(k42v65)));
  EXPECT_THAT(k42v99 < k42, false);
  EXPECT_THAT(k42v99, Not(Lt(k42)));

  EXPECT_THAT(k42v65 <= k42v65, true);
  EXPECT_THAT(k42v65, Le(k42v65));
  EXPECT_THAT(k42v65 <= k42v99, true);
  EXPECT_THAT(k42v65, Le(k42v99));
  EXPECT_THAT(k42 <= k42v99, true);
  EXPECT_THAT(k42, Le(k42v99));

  EXPECT_THAT(k42v99 > k42v65, true);
  EXPECT_THAT(k42v99, Gt(k42v65));

  EXPECT_THAT(k42v65 >= k42, true);
  EXPECT_THAT(k42v65, Ge(k42));
}

TEST_F(LimitedSetTest, CompareDifferentType) {
  const auto k42v65 = MakeLimitedSet<std::string>("42", "65");
  constexpr auto k42o65 = MakeLimitedSet<std::string_view>("42", "65");
  constexpr auto k42v99 = MakeLimitedSet("42", "99");
  constexpr auto k42 = MakeLimitedSet("42");
  EXPECT_THAT(k42v65 == k42o65, true);
  EXPECT_THAT(k42v65, k42o65);
  EXPECT_THAT(k42v65, Eq(k42o65));

  EXPECT_THAT(k42v65 != k42v99, true);
  EXPECT_THAT(k42v65, Not(k42v99));
  EXPECT_THAT(k42v65, Not(Eq(k42v99)));
  EXPECT_THAT(k42v65 != k42, true);
  EXPECT_THAT(k42v65, Not(k42));
  EXPECT_THAT(k42v65, Not(Eq(k42)));

  EXPECT_THAT(k42v65 < k42v99, true);
  EXPECT_THAT(k42v65, Lt(k42v99));
  EXPECT_THAT(k42 < k42v99, true);
  EXPECT_THAT(k42, Lt(k42v99));
  EXPECT_THAT(k42v99 < k42v65, false);
  EXPECT_THAT(k42v99, Not(Lt(k42v65)));
  EXPECT_THAT(k42v99 < k42, false);
  EXPECT_THAT(k42v99, Not(Lt(k42)));

  EXPECT_THAT(k42v65 <= k42v65, true);
  EXPECT_THAT(k42v65, Le(k42v65));
  EXPECT_THAT(k42v65 <= k42v99, true);
  EXPECT_THAT(k42v65, Le(k42v99));
  EXPECT_THAT(k42 <= k42v99, true);
  EXPECT_THAT(k42, Le(k42v99));

  EXPECT_THAT(k42v99 > k42v65, true);
  EXPECT_THAT(k42v99, Gt(k42v65));

  EXPECT_THAT(k42v65 >= k42, true);
  EXPECT_THAT(k42v65, Ge(k42));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
