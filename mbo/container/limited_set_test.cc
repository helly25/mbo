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
using ::testing::Ne;
using ::testing::Not;
using ::testing::Pair;
using ::testing::SizeIs;

static_assert(std::ranges::range<LimitedSet<int, 1>>);
static_assert(std::contiguous_iterator<LimitedSet<int, 2>::iterator>);
static_assert(std::contiguous_iterator<LimitedSet<int, 3>::const_iterator>);

struct LimitedSetTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedSetTest, MakeNoArg) {
  constexpr auto kTest = MakeLimitedSet<int>();
  EXPECT_THAT(kTest, IsEmpty());
  EXPECT_THAT(kTest, SizeIs(0));
  EXPECT_THAT(kTest, CapacityIs(0));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedSetTest, MakeOneArg) {
  constexpr auto kTest = MakeLimitedSet(42);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(1));
  EXPECT_THAT(kTest, CapacityIs(1));
  EXPECT_THAT(kTest, ElementsAre(42));
}

TEST_F(LimitedSetTest, MakeInitArgCTAD) {
  {
    constexpr LimitedSet kTest{1};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(1));
    EXPECT_THAT(kTest, CapacityIs(1));
    EXPECT_THAT(kTest, ElementsAre(1));
  }
  {
    constexpr LimitedSet kTest{1, 2};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(2));
    EXPECT_THAT(kTest, CapacityIs(2));
    EXPECT_THAT(kTest, ElementsAre(1, 2));
  }
  {
    constexpr LimitedSet kTest{1, 2, 3};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(3));
    EXPECT_THAT(kTest, CapacityIs(3));
    EXPECT_THAT(kTest, ElementsAre(1, 2, 3));
  }
  {
    constexpr LimitedSet kTest{"a", "b", "c", "d"};
    EXPECT_THAT(kTest, Not(IsEmpty()));
    EXPECT_THAT(kTest, SizeIs(4));
    EXPECT_THAT(kTest, CapacityIs(4));
    EXPECT_THAT(kTest, ElementsAre("a", "b", "c", "d"));
  }
}

TEST_F(LimitedSetTest, MakeInitArgFind) {
  constexpr auto kTest = MakeLimitedSet<5>({1, 3, 5});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(5));
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
  EXPECT_THAT(test, CapacityIs(7));
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
  EXPECT_THAT(test, CapacityIs(3)) << "There are duplicates, and so the construction with M=3 works.";
  EXPECT_THAT(test, ElementsAre(1, 3, 5));
}

TEST_F(LimitedSetTest, MakeInitArg) {
  constexpr auto kTest = MakeLimitedSet<3>({1, 2, 0});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(3));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedSetTest, MakeInitArgLarger) {
  constexpr auto kTest = MakeLimitedSet<5>({1, 0, 2});
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(3));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2));
}

TEST_F(LimitedSetTest, MakeMultiArg) {
  constexpr auto kTest = MakeLimitedSet(0, 3, 2, 1);
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedSetTest, CustomCompare) {
  constexpr auto kTest = MakeLimitedSet<4>({0, 3, 2, 1}, std::greater());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(3, 2, 1, 0));
}

TEST_F(LimitedSetTest, MakeIteratorArg) {
  constexpr std::array<int, 4> kVec{0, 1, 2, 3};
  constexpr auto kTest = MakeLimitedSet<5>(kVec.begin(), kVec.end());
  EXPECT_THAT(kTest, Not(IsEmpty()));
  EXPECT_THAT(kTest, SizeIs(4));
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedSetTest, MakeWithStrings) {
  const std::vector<std::string> data{{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = MakeLimitedSet<4>(data.begin(), data.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
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
  EXPECT_THAT(kTest, CapacityIs(4));
  EXPECT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedSetTest, ToLimitedSetStringCopy) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  const std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedSet(array);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
  EXPECT_THAT(test, ElementsAre("0", "1", "2", "3"));
  // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(LimitedSetTest, ToLimitedSetStringMove) {
  // NOLINTBEGIN(*-avoid-c-arrays)
  std::string array[4] = {{"0"}, {"1"}, {"2"}, {"3"}};
  auto test = ToLimitedSet(std::move(array));
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, CapacityIs(4));
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
  EXPECT_THAT(kTest, CapacityIs(5));
  EXPECT_THAT(kTest, ElementsAre());
}

TEST_F(LimitedSetTest, Erase) {
  auto test = MakeLimitedSet(0, 1, 2, 3, 4);
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, CapacityIs(5));
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
  EXPECT_THAT(kTest, CapacityIs(6));
  ASSERT_THAT(kTest, ElementsAre(0, 1, 2, 3));
  EXPECT_THAT(kTest.contains(0), true);
  EXPECT_THAT(kTest.contains(4), false);
  EXPECT_THAT(kTest.contains_all(std::vector<int>{1, 2}), true);
  EXPECT_THAT(kTest.contains_all({1, 5}), false);
  EXPECT_THAT(kTest.contains_any(std::vector<int>{5, 2}), true);
  EXPECT_THAT(kTest.contains_any({4, 5}), false);
}

TEST_F(LimitedSetTest, Insert) {
  auto test = MakeLimitedSet<6>({0, 3});
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(2));
  EXPECT_THAT(test, CapacityIs(6));
  ASSERT_THAT(test, ElementsAre(0, 3));
  const std::vector<int> other{1, 2, 4};
  test.insert(other.begin(), other.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(5));
  EXPECT_THAT(test, CapacityIs(6));
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

template <std::size_t Size>
constexpr void CompareAllTheSizes() {  // NOLINT(readability-function-cognitive-complexity)
  LimitedSet<int, Size> data;
  for (std::size_t len = 0; len < Size; ++len) {
    data.emplace(len * 100);
  }
  while (!data.empty()) {
    for (std::size_t pos = 0; pos < Size + 1; ++pos) {
      int v = 100 * static_cast<int>(pos + Size - data.size());
      if (pos >= data.size()) {
        ASSERT_THAT(data.index_of(v), data.npos);
        ASSERT_FALSE(data.contains(v));
        ASSERT_THAT(data.find(v), data.end());
      } else {
        ASSERT_THAT(data.index_of(v), pos);
        ASSERT_TRUE(data.contains(v));
        ASSERT_THAT(data.find(v), Ne(data.end()));
      }
    }
    data.erase(data.begin());
  }
}

TEST_F(LimitedSetTest, CompareAllTheSizes) {
  CompareAllTheSizes<1>();
  CompareAllTheSizes<2>();
  CompareAllTheSizes<3>();
  CompareAllTheSizes<4>();
  CompareAllTheSizes<5>();
  CompareAllTheSizes<6>();
  CompareAllTheSizes<7>();
  CompareAllTheSizes<8>();
  CompareAllTheSizes<9>();
  CompareAllTheSizes<10>();
  CompareAllTheSizes<11>();
  CompareAllTheSizes<12>();
  CompareAllTheSizes<13>();
  CompareAllTheSizes<14>();
  CompareAllTheSizes<15>();
  CompareAllTheSizes<16>();
}


TEST_F(LimitedSetTest, PreSortedInput) {
  constexpr LimitedSet<int, LimitedOptions<4, LimitedOptionsFlag::kRequireSortedInput>{}> kData{0, 1, 2, 42};
  EXPECT_THAT(kData, ElementsAre(0, 1, 2, 42));
}

TEST_F(LimitedSetTest, AtIndex) {
  static constexpr auto kTest = LimitedSet<int, 2>{25, 42};
  EXPECT_THAT(kTest.at_index(0), 25);
  EXPECT_THAT(kTest.at_index(1), 42);
  auto test = LimitedSet<int, 2>{25, 42};
  test.at_index(1) = 99;
  EXPECT_THAT(test, ElementsAre(25, 99));
}

TEST_F(LimitedSetTest, AtIndexNonExistingThrows) {
#if !HAS_ADDRESS_SANITIZER
  // Disabled due to https://github.com/google/sanitizers/issues/749
  const bool caught = []() {
    static constexpr auto kTest = LimitedSet<int, 2>{25, 42};
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

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
