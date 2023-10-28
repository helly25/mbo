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

#include "mbo/container/limited_map.h"

#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"

// Clang has issues with exception tracing in ASAN, so corresponding tests must
// be disabled. But we do so for all known ASAN identification methods.
#ifndef HAS_ADDRESS_SANITIZER
# if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#   define HAS_ADDRESS_SANITIZER 1
#  endif
# elif defined(__SANITIZE_ADDRESS__)
#  define HAS_ADDRESS_SANITIZER 1
# endif
#endif

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
using ::testing::Pair;
using ::testing::SizeIs;

static_assert(std::ranges::range<LimitedMap<int, int, 3>>);
static_assert(std::contiguous_iterator<LimitedMap<int, int, 3>::iterator>);
static_assert(std::contiguous_iterator<LimitedMap<int, int, 3>::const_iterator>);

struct LimitedMapTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedMapTest, ConstructEmpty) {
  constexpr auto kTest = LimitedMap<int, int, 0>();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(0));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedMapTest, MakeNoArg) {
  constexpr auto kTest = MakeLimitedMap<int, int>();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(0));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedMapTest, TypedInit) {
  {
    constexpr LimitedMap<int, int, 2> kTest{{{25, 33}, {42, 99}}};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(2));
    EXPECT_THAT(kTest, CapacityIs(2));
    EXPECT_THAT(kTest, ElementsAre(Pair(25, 33), Pair(42, 99)));
  }
  {
    constexpr auto kTest = LimitedMap<int, int, 2>{{{25, 33}, {42, 99}}};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(2));
    EXPECT_THAT(kTest, CapacityIs(2));
    EXPECT_THAT(kTest, ElementsAre(Pair(25, 33), Pair(42, 99)));
  }
}

TEST_F(LimitedMapTest, MakeFromPairs) {
  constexpr auto kTest = MakeLimitedMap(std::make_pair(25, 33), std::make_pair(42, 99));
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(2));
  EXPECT_THAT(kTest, CapacityIs(2));
  EXPECT_THAT(kTest, ElementsAre(Pair(25, 33), Pair(42, 99)));
}

TEST_F(LimitedMapTest, MakeInitListOfPairs) {
  constexpr auto kTest = MakeLimitedMap<3>({std::make_pair(25, 33), std::make_pair(42, 99)});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(2));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre(Pair(25, 33), Pair(42, 99)));
}

TEST_F(LimitedMapTest, MakeInitArgFind) {
  constexpr auto kTest = MakeLimitedMap(std::make_pair(1, 11), std::make_pair(2, 22), std::make_pair(3, 33));
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33)));
  EXPECT_THAT(kTest.index_of(1), 0);
  EXPECT_THAT(kTest.find(1) - kTest.begin(), 0);
  EXPECT_THAT(kTest.index_of(2), 1);
  EXPECT_THAT(kTest.find(2) - kTest.begin(), 1);
  EXPECT_THAT(kTest.index_of(3), 2);
  EXPECT_THAT(kTest.find(3) - kTest.begin(), 2);
  EXPECT_THAT(kTest.index_of(0), kTest.npos);
  EXPECT_THAT(kTest.find(0), kTest.end());
  EXPECT_THAT(kTest.index_of(4), kTest.npos);
  EXPECT_THAT(kTest.find(4), kTest.end());
}

TEST_F(LimitedMapTest, MakeInitArgBasics) {
  auto test = MakeLimitedMap<7>({std::make_pair(1, 11), std::make_pair(3, 33), std::make_pair(5, 55)});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(3));
  EXPECT_THAT(test, CapacityIs(7));
  EXPECT_THAT(test, ElementsAre(Pair(1, 11), Pair(3, 33), Pair(5, 55)));
  EXPECT_THAT(test.find(1), test.begin());
  EXPECT_THAT(test.find(1) - test.begin(), 0);
  EXPECT_THAT(test.find(3), test.begin() + 1);
  EXPECT_THAT(test.find(3) - test.begin(), 1);
  EXPECT_THAT(test.find(5), test.begin() + 2);
  EXPECT_THAT(test.find(5), test.end() - 1);
  EXPECT_THAT(test.find(5) - test.begin(), 2);
  EXPECT_THAT(test.find(0), test.end());
  EXPECT_THAT(test.emplace(0, 0), Pair(test.begin(), true));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 11), Pair(3, 33), Pair(5, 55)));
  EXPECT_THAT(test.find(2), test.end());
  EXPECT_THAT(test.emplace(2, 22), Pair(test.begin() + 2, true));
  EXPECT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 11), Pair(2, 22), Pair(3, 33), Pair(5, 55)));
  EXPECT_THAT(test.find(6), test.end());
  EXPECT_THAT(test.emplace(6, 66), Pair(test.end(), true));
  EXPECT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 11), Pair(2, 22), Pair(3, 33), Pair(5, 55), Pair(6, 66)));
  EXPECT_THAT(test.find(4), test.end());
  EXPECT_THAT(test.emplace(4, 44), Pair(test.begin() + 4, true));
  EXPECT_THAT(
      test, ElementsAre(Pair(0, 0), Pair(1, 11), Pair(2, 22), Pair(3, 33), Pair(4, 44), Pair(5, 55), Pair(6, 66)));
  EXPECT_THAT(test.find(0), test.begin());
  EXPECT_THAT(test.find(1), test.begin() + 1);
  EXPECT_THAT(test.find(2), test.begin() + 2);
  EXPECT_THAT(test.find(3), test.begin() + 3);
  EXPECT_THAT(test.find(4), test.begin() + 4);
  EXPECT_THAT(test.find(5), test.begin() + 5);
  EXPECT_THAT(test.find(6), test.begin() + 6);
}

TEST_F(LimitedMapTest, MakeInitWithDuplicates) {
  constexpr auto kTest1 =
      MakeLimitedMap(std::make_pair(1, 11), std::make_pair(3, 33), std::make_pair(3, 33), std::make_pair(5, 55));
  EXPECT_THAT(kTest1, Not(IsEmpty()));
  EXPECT_THAT(kTest1, SizeIs(3));
  EXPECT_THAT(kTest1, CapacityIs(4)) << "Autodetected.";
  EXPECT_THAT(kTest1, ElementsAre(Pair(1, 11), Pair(3, 33), Pair(5, 55)));
  constexpr auto kTest2 =
      MakeLimitedMap<3>({std::make_pair(1, 11), std::make_pair(3, 33), std::make_pair(3, 33), std::make_pair(5, 55)});
  EXPECT_THAT(kTest2, Not(IsEmpty()));
  EXPECT_THAT(kTest2, SizeIs(3));
  EXPECT_THAT(kTest2, CapacityIs(3)) << "There are duplicates, and so the construction with M=3 works.";
  EXPECT_THAT(kTest2, ElementsAre(Pair(1, 11), Pair(3, 33), Pair(5, 55)));
}

TEST_F(LimitedMapTest, CustomCompare) {
  constexpr auto kTest = MakeLimitedMap<4>(
      {std::make_pair(0, 1), std::make_pair(3, 2), std::make_pair(2, 5), std::make_pair(1, 7)}, std::greater());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(Pair(3, 2), Pair(2, 5), Pair(1, 7), Pair(0, 1)));
}

TEST_F(LimitedMapTest, MakeIteratorArg) {
  constexpr std::array<std::pair<int, int>, 4> kVec{{{0, 0}, {1, 11}, {2, 22}, {3, 33}}};
  constexpr auto kTest = MakeLimitedMap<5>(kVec.begin(), kVec.end());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre(Pair(0, 0), Pair(1, 11), Pair(2, 22), Pair(3, 33)));
}

TEST_F(LimitedMapTest, MakeWithStrings) {
  const std::vector<std::pair<std::string, std::string>> data{{"0", "a"}, {"1", "b"}, {"2", "c"}, {"3", "d"}};
  auto test = MakeLimitedMap<4>(data.begin(), data.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "b"), Pair("2", "c"), Pair("3", "d")));
}

TEST_F(LimitedMapTest, Update) {
  const std::vector<std::pair<std::string, std::string>> data{{"0", "a"}, {"1", "b"}, {"2", "c"}, {"3", "d"}};
  auto test = MakeLimitedMap<7>(data.begin(), data.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(7));
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "b"), Pair("2", "c"), Pair("3", "d")));
  test["1"] = "bb";
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "bb"), Pair("2", "c"), Pair("3", "d")));
  test["4"] = "eeee";
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "bb"), Pair("2", "c"), Pair("3", "d"), Pair("4", "eeee")));
  test.at("0") = "zero";
  EXPECT_THAT(test, ElementsAre(Pair("0", "zero"), Pair("1", "bb"), Pair("2", "c"), Pair("3", "d"), Pair("4", "eeee")));
  EXPECT_THAT(test, CapacityIs(Gt(test.size())));
  test[" "] = "space";
  EXPECT_THAT(
      test,
      ElementsAre(
          Pair(" ", "space"), Pair("0", "zero"), Pair("1", "bb"), Pair("2", "c"), Pair("3", "d"), Pair("4", "eeee")));
}

TEST_F(LimitedMapTest, UpdateNonExistingThrows) {
#if !HAS_ADDRESS_SANITIZER
  // Disabled due to https://github.com/google/sanitizers/issues/749
  const bool caught = []() {
    auto test = LimitedMap<int, int, 2>();
    try {
      test.at(25) = 42;
    } catch (const std::out_of_range&) {
      return true;
    } catch (...) {
      return false;
    }
    return false;
  }();
  ASSERT_TRUE(caught);
#endif
}

TEST_F(LimitedMapTest, AtIndex) {
  static constexpr auto kTest = LimitedMap<int, int, 2>{{1, 2}, {3, 4}};
  EXPECT_THAT(kTest.at_index(0), Pair(1, 2));
  EXPECT_THAT(kTest.at_index(1), Pair(3, 4));
  auto test = LimitedMap<int, int, 2>{{1, 2}, {3, 4}};
  test.at_index(1).second = 42;
  EXPECT_THAT(test, ElementsAre(Pair(1, 2), Pair(3, 42)));
}

TEST_F(LimitedMapTest, AtIndexNonExistingThrows) {
#if !HAS_ADDRESS_SANITIZER
  // Disabled due to https://github.com/google/sanitizers/issues/749
  const bool caught = []() {
    static constexpr auto kTest = LimitedMap<int, int, 2>{{1, 2}, {3, 4}};
    try {
      kTest.at_index(3);
    } catch (const std::out_of_range&) {
      return true;
    } catch (...) {
      return false;
    }
    return false;
  }();
  ASSERT_TRUE(caught);
#endif
}

TEST_F(LimitedMapTest, ConstructAssignFromSmaller) {
  {
    constexpr LimitedMap<uint32_t, uint32_t, 3> kSource({{0U, 0U}, {1U, 1U}, {2U, 2U}});
    LimitedMap<int64_t, int64_t, 5> target(kSource);
    EXPECT_THAT(target, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
    EXPECT_THAT(kSource, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  }
  {
    constexpr LimitedMap<uint32_t, uint32_t, 3> kSource({{0U, 0U}, {1U, 1U}, {2U, 2U}});
    LimitedMap<int64_t, int64_t, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = kSource;
    EXPECT_THAT(target, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
    EXPECT_THAT(kSource, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  }
  {
    LimitedMap<uint32_t, uint32_t, 3> source({{0U, 0U}, {1U, 1U}, {2U, 2U}});
    LimitedMap<int64_t, int64_t, 5> target(std::move(source));
    EXPECT_THAT(target, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
    EXPECT_THAT(source, IsEmpty());
  }
  {
    LimitedMap<uint32_t, uint32_t, 3> source({{0U, 0U}, {1U, 1U}, {2U, 2U}});
    LimitedMap<int64_t, int64_t, 5> target;
    ASSERT_THAT(target, IsEmpty());
    target = std::move(source);
    EXPECT_THAT(target, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
    EXPECT_THAT(source, IsEmpty());
  }
}

TEST_F(LimitedMapTest, ToLimitedMap) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  constexpr std::pair<int, int> kArray[4] = {{0, 0}, {1, 1}, {2, 2}, {3, 3}};
  constexpr auto kTest = ToLimitedMap(kArray);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2), Pair(3, 3)));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedMapTest, ToLimitedMapStringCopy) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  const std::pair<std::string, std::string> array[4] = {{"0", "a"}, {"1", "b"}, {"2", "c"}, {"3", "d"}};
  auto test = ToLimitedMap(array);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "b"), Pair("2", "c"), Pair("3", "d")));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedMapTest, ToLimitedMapStringMove) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  std::pair<std::string, std::string> array[4] = {{"0", "a"}, {"1", "b"}, {"2", "c"}, {"3", "d"}};
  auto test = ToLimitedMap(std::move(array));
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre(Pair("0", "a"), Pair("1", "b"), Pair("2", "c"), Pair("3", "d")));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedMapTest, ConstexprMakeClear) {
  constexpr auto kTest = [] {
    auto test = ToLimitedMap<int, int>({{0, 0}, {1, 1}, {2, 2}});
    test.clear();
    return test;
  }();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedMapTest, Erase) {
  auto test = LimitedMap<int, int, 5>{{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}}};
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, CapacityIs(5));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2), Pair(3, 3), Pair(4, 4)));
  EXPECT_THAT(test.erase(test.begin() + 2), test.begin() + 2);
  EXPECT_THAT(test, SizeIs(4));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(3, 3), Pair(4, 4)));
  EXPECT_THAT(test.erase(test.end() - 1), test.begin() + 3);
  EXPECT_THAT(test.begin() + 3, test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, SizeIs(3));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(3, 3)));
  EXPECT_THAT(test.erase(1), 1);
  EXPECT_THAT(test.erase(1), 0);
  EXPECT_THAT(test, SizeIs(2));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(3, 3)));
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.erase(test.begin()), test.begin());
  EXPECT_THAT(test.begin(), test.end()) << "Should have returned new `end`.";
  EXPECT_THAT(test, IsEmpty());
}

TEST_F(LimitedMapTest, Contains) {
  constexpr auto kTest = LimitedMap<int, int, 6>{{0, 0}, {1, 1}, {2, 2}, {3, 3}};
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(6));
  ASSERT_THAT(kTest, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2), Pair(3, 3)));
  EXPECT_THAT(kTest.contains(0), true);
  EXPECT_THAT(kTest.contains(4), false);
  EXPECT_THAT(kTest.contains_all(std::vector<int>{1, 2}), true);
  EXPECT_THAT(kTest.contains_all({1, 5}), false);
  EXPECT_THAT(kTest.contains_any(std::vector<int>{5, 2}), true);
  EXPECT_THAT(kTest.contains_any({4, 5}), false);
}

TEST_F(LimitedMapTest, Insert) {
  auto test = LimitedMap<int, int, 6>({{0, 0}, {3, 3}});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, CapacityIs(6));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(3, 3)));
  const std::vector<std::pair<int, int>> other{{1, 1}, {2, 2}, {4, 4}};
  test.insert(other.begin(), other.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, CapacityIs(6));
  ASSERT_THAT(test, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2), Pair(3, 3), Pair(4, 4)));
}

TEST_F(LimitedMapTest, Swap) {
  auto test1 = LimitedMap<int, int, 3>({{0, 0}, {1, 1}, {2, 2}});
  auto test2 = LimitedMap<int, int, 3>({{3, 3}});
  ASSERT_THAT(test1, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  ASSERT_THAT(test2, ElementsAre(Pair(3, 3)));
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre(Pair(3, 3)));
  EXPECT_THAT(test2, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  EXPECT_THAT(test2, ElementsAre(Pair(3, 3)));
  test2.clear();
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre());
  EXPECT_THAT(test2, ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  test2.clear();
  test1.swap(test2);
  EXPECT_THAT(test1, ElementsAre());
  EXPECT_THAT(test2, ElementsAre());
}

TEST_F(LimitedMapTest, Iterators) {
  constexpr auto kTest = LimitedMap<int, int, 3>{{0, 0}, {1, 1}, {2, 2}};
  // Restrictions apply: The two following cannot be constexpr.
  EXPECT_THAT((MakeLimitedMap<3>(kTest.begin(), kTest.end())), ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
  EXPECT_THAT((MakeLimitedMap<3>(kTest.rbegin(), kTest.rend())), ElementsAre(Pair(0, 0), Pair(1, 1), Pair(2, 2)));
}

TEST_F(LimitedMapTest, Compare) {
  constexpr auto k42v65 = LimitedMap<int, int, 2>{{{42, 42}, {65, 65}}};
  constexpr auto k42o65 = LimitedMap<int, int, 2>{{{42, 42}, {65, 65}}};
  constexpr auto k42c42o65c64 = LimitedMap<int, int, 2>{{{42, 42}, {65, 64}}};
  constexpr auto k42v99 = LimitedMap<int, int, 2>{{{42, 42}, {99, 99}}};
  constexpr auto k42 = LimitedMap<int, int, 1>{{{42, 42}}};
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

  EXPECT_THAT(k42v65 != k42c42o65c64, true);
  EXPECT_THAT(k42v65 > k42c42o65c64, true);
  EXPECT_THAT(k42v65 >= k42c42o65c64, true);
  EXPECT_THAT(k42c42o65c64 < k42v65, true);
  EXPECT_THAT(k42c42o65c64 <= k42v65, true);
}

TEST_F(LimitedMapTest, CompareDifferentType) {
  const auto k42v65 = LimitedMap<std::string, int, 2>{{{"42", 42}, {"65", 65}}};
  constexpr auto k42o65 = LimitedMap<std::string_view, int, 2>{{{"42", 42}, {"65", 65}}};
  constexpr auto k42v99 = LimitedMap<std::string_view, int, 2>{{{"42", 42}, {"99", 99}}};
  constexpr auto k42 = LimitedMap<std::string_view, int, 1>{{{"42", 42}}};
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
