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

#include "mbo/container/limited_vector.h"

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

namespace mbo::container {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::mbo::testing::CapacityIs;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Not;
using ::testing::SizeIs;

struct LimitedVectorTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedVectorTest, MakeNoArg) {
  constexpr auto kTest = MakeLimitedVector<int>();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(0));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedVectorTest, MakeOneArg) {
  constexpr auto kTest = MakeLimitedVector(42);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(1));
  EXPECT_THAT(kTest, CapacityIs(1));
  EXPECT_THAT(kTest, ElementsAre(42));
}

TEST_F(LimitedVectorTest, MakeInitArg) {
  constexpr auto kTest = MakeLimitedVector<3>({0, 1, 2});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedVectorTest, MakeInitArgLarger) {
  constexpr auto kTest = MakeLimitedVector<5>({0, 1, 2});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedVectorTest, MakeMultiArg) {
  constexpr auto kTest = MakeLimitedVector(0, 1, 2, 3);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedVectorTest, MakeNumArg) {
  // auto kTest0 = MakeLimitedVector<0>(42);
  // EXPECT_THAT(kTst0, IsEmpty());
  // EXPECT_THAT(kTest0, SizeIs(0));
  // EXPECT_THAT(kTest0, ElementsAre());
  constexpr auto kTest1 = MakeLimitedVector<1>(42);
  EXPECT_THAT(kTest1, Not(IsEmpty()));
  EXPECT_THAT(kTest1, SizeIs(1));
  EXPECT_THAT(kTest1, CapacityIs(1));
  EXPECT_THAT(kTest1, ElementsAre(42));
  constexpr auto kTest2 = MakeLimitedVector<2>(42);
  EXPECT_THAT(kTest2, Not(IsEmpty()));
  EXPECT_THAT(kTest2, SizeIs(2));
  EXPECT_THAT(kTest2, CapacityIs(2));
  EXPECT_THAT(kTest2, ElementsAre(42, 42));
}

TEST_F(LimitedVectorTest, MakeIteratorArg) {
  constexpr std::array<int, 4> kVec{0, 1, 2, 3};
  constexpr auto kTest = MakeLimitedVector<5>(kVec.begin(), kVec.end());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedVectorTest, MakeWithStrings) {
  const std::vector<std::string> data{{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = MakeLimitedVector<4>(data.begin(), data.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
}

TEST_F(LimitedVectorTest, ConstructAssignFromSmaller) {
  {
    constexpr LimitedVector<unsigned, 3> kSource({0U, 1U, 2U});
    LimitedVector<int, 5> target(kSource);
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    constexpr LimitedVector<unsigned, 3> kSource({0U, 1U, 2U});
    LimitedVector<int, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = kSource;
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    LimitedVector<unsigned, 4> source({0U, 1U, 2U});
    LimitedVector<int, 5> target(std::move(source));
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
  {
    LimitedVector<unsigned, 3> source({0U, 1U, 2U});
    LimitedVector<int, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = std::move(source);
    EXPECT_THAT(target, ElementsAre(0, 1, 2));
  }
}

TEST_F(LimitedVectorTest, ToLimitedVector) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  constexpr int kArray[4] = {0, 1, 2, 3};
  constexpr auto kTest = ToLimitedVector(kArray);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedVectorTest, ToLimitedVectorStringCopy) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  const std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedVector(array);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedVectorTest, ToLimitedVectorStringMove) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedVector(std::move(array));
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedVectorTest, ConstexprMakeClear) {
  constexpr auto kTest = [] {
    auto test = MakeLimitedVector<5>({0, 1, 2});
    test.clear();
    return test;
  }();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedVectorTest, ConstexprMakePushPop) {
  constexpr auto kTest = [] {
    auto test = MakeLimitedVector<6>({0, 1, 2});
    test.pop_back();
    test.emplace_back(3);
    test.push_back(4);
    return test;
  }();
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(6));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 3, 4));
}

TEST_F(LimitedVectorTest, ReadAccess) {
  constexpr auto kTest = MakeLimitedVector<6>({0, 1, 2, 3});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(6));
  ASSERT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  EXPECT_THAT(kTest.front(), 0);
  EXPECT_THAT(kTest.at(1), 1);
  EXPECT_THAT(kTest.at(2), 2);
  EXPECT_THAT(kTest.back(), 3);
}

TEST_F(LimitedVectorTest, WriteAccess) {
  auto test = MakeLimitedVector<6>({0, 1, 2, 3});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(6));
  ASSERT_THAT(test, ElementsAre(0, 1, 2, 3));
  test.front() = 10;
  test.at(1) = 11;
  test.at(2) = 12;
  test.back() = 13;
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(6));
  ASSERT_THAT(test, ElementsAre(10, 11, 12, 13));
}

TEST_F(LimitedVectorTest, Emplace) {
  auto test = MakeLimitedVector<7>({1, 3});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, CapacityIs(7));
  ASSERT_THAT(test, ElementsAre(1, 3));
  EXPECT_THAT(test.emplace(test.begin() + 1, 20), test.begin() + 1);
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, ElementsAre(1, 20, 3));
  EXPECT_THAT(test.emplace(test.end(), 40), test.begin() + 3);
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, ElementsAre(1, 20, 3, 40));
  EXPECT_THAT(test.emplace(test.begin(), 0), test.begin());
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, ElementsAre(0, 1, 20, 3, 40));
}

TEST_F(LimitedVectorTest, Erase) {
  auto test = MakeLimitedVector(0, 1, 20, 3, 40);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, CapacityIs(5));
  ASSERT_THAT(test, ElementsAre(0, 1, 20, 3, 40));
  EXPECT_THAT(test.erase(test.begin() + 2), test.begin() + 2);
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, ElementsAre(0, 1, 3, 40));
  EXPECT_THAT(test.erase(test.end() - 1), test.begin() + 3);
  EXPECT_THAT(test.begin() + 3, test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, ElementsAre(0, 1, 3));
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, ElementsAre(1, 3));
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.begin(), test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, IsEmpty());
}

TEST_F(LimitedVectorTest, EraseRange) {
  auto test = MakeLimitedVector(0, 1, 20, 3, 40, 50, 60, 70, 8, 9);
  using Type = decltype(test);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(10));
  EXPECT_THAT(test, CapacityIs(10));
  ASSERT_THAT(test, ElementsAre(0, 1, 20, 3, 40, 50, 60, 70, 8, 9));
  Type::const_iterator first = test.begin() + 4;
  Type::const_iterator last = test.begin() + 8;
  Type::const_iterator it = test.erase(first, last);
  EXPECT_THAT(it, test.begin() + 4);
  EXPECT_THAT(test, SizeIs(6));
  EXPECT_THAT(test, ElementsAre(0, 1, 20, 3, 8, 9));
  EXPECT_THAT(test.erase(test.begin() + 3, test.end()), test.begin() + 3);
  EXPECT_THAT(test.begin() + 3, test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, ElementsAre(0, 1, 20));
  EXPECT_THAT(test.erase(test.begin(), test.end()), test.begin());
  EXPECT_THAT(test.begin(), test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, IsEmpty());
}

TEST_F(LimitedVectorTest, Swap) {
  auto test1 = MakeLimitedVector(0, 1, 2);
  auto test2 = MakeLimitedVector<3>(3);
  test2.pop_back();
  test2.pop_back();
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

TEST_F(LimitedVectorTest, Iterators) {
  constexpr auto kTest = MakeLimitedVector(0, 1, 2);
  // Restrictions apply: The two following cannot be constexpr.
  EXPECT_THAT((MakeLimitedVector<3>(kTest.begin(), kTest.end())), ElementsAre(0, 1, 2));
  EXPECT_THAT((MakeLimitedVector<3>(kTest.rbegin(), kTest.rend())), ElementsAre(2, 1, 0));
}

TEST_F(LimitedVectorTest, Compare) {
  constexpr auto k42v25 = MakeLimitedVector(42, 25);
  constexpr auto k42o25 = MakeLimitedVector(42, 25);
  constexpr auto k42v33 = MakeLimitedVector(42, 33);
  constexpr auto k42 = MakeLimitedVector(42);
  EXPECT_THAT(k42v25 == k42o25, true);
  EXPECT_THAT(k42v25, k42o25);
  EXPECT_THAT(k42v25, Eq(k42o25));

  EXPECT_THAT(k42v25 != k42v33, true);
  EXPECT_THAT(k42v25, Not(k42v33));
  EXPECT_THAT(k42v25, Not(Eq(k42v33)));
  EXPECT_THAT(k42v25 != k42, true);
  EXPECT_THAT(k42v25, Not(k42));
  EXPECT_THAT(k42v25, Not(Eq(k42)));

  EXPECT_THAT(k42v25 < k42v33, true);
  EXPECT_THAT(k42v25, Lt(k42v33));
  EXPECT_THAT(k42 < k42v33, true);
  EXPECT_THAT(k42, Lt(k42v33));
  EXPECT_THAT(k42v33 < k42v25, false);
  EXPECT_THAT(k42v33, Not(Lt(k42v25)));
  EXPECT_THAT(k42v33 < k42, false);
  EXPECT_THAT(k42v33, Not(Lt(k42)));

  EXPECT_THAT(k42v25 <= k42v25, true);
  EXPECT_THAT(k42v25, Le(k42v25));
  EXPECT_THAT(k42v25 <= k42v33, true);
  EXPECT_THAT(k42v25, Le(k42v33));
  EXPECT_THAT(k42 <= k42v33, true);
  EXPECT_THAT(k42, Le(k42v33));

  EXPECT_THAT(k42v33 > k42v25, true);
  EXPECT_THAT(k42v33, Gt(k42v25));

  EXPECT_THAT(k42v25 >= k42, true);
  EXPECT_THAT(k42v25, Ge(k42));
}

TEST_F(LimitedVectorTest, CompareDifferentType) {
  const auto k42v25 = MakeLimitedVector<std::string>("42", "25");
  constexpr auto k42o25 = MakeLimitedVector<std::string_view>("42", "25");
  constexpr auto k42v33 = MakeLimitedVector("42", "33");
  constexpr auto k42 = MakeLimitedVector("42");
  EXPECT_THAT(k42v25 == k42o25, true);
  EXPECT_THAT(k42v25, k42o25);
  EXPECT_THAT(k42v25, Eq(k42o25));

  EXPECT_THAT(k42v25 != k42v33, true);
  EXPECT_THAT(k42v25, Not(k42v33));
  EXPECT_THAT(k42v25, Not(Eq(k42v33)));
  EXPECT_THAT(k42v25 != k42, true);
  EXPECT_THAT(k42v25, Not(k42));
  EXPECT_THAT(k42v25, Not(Eq(k42)));

  EXPECT_THAT(k42v25 < k42v33, true);
  EXPECT_THAT(k42v25, Lt(k42v33));
  EXPECT_THAT(k42 < k42v33, true);
  EXPECT_THAT(k42, Lt(k42v33));
  EXPECT_THAT(k42v33 < k42v25, false);
  EXPECT_THAT(k42v33, Not(Lt(k42v25)));
  EXPECT_THAT(k42v33 < k42, false);
  EXPECT_THAT(k42v33, Not(Lt(k42)));

  EXPECT_THAT(k42v25 <= k42v25, true);
  EXPECT_THAT(k42v25, Le(k42v25));
  EXPECT_THAT(k42v25 <= k42v33, true);
  EXPECT_THAT(k42v25, Le(k42v33));
  EXPECT_THAT(k42 <= k42v33, true);
  EXPECT_THAT(k42, Le(k42v33));

  EXPECT_THAT(k42v33 > k42v25, true);
  EXPECT_THAT(k42v33, Gt(k42v25));

  EXPECT_THAT(k42v25 >= k42, true);
  EXPECT_THAT(k42v25, Ge(k42));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
