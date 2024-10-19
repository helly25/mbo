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

#include "mbo/testing/matchers.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::testing {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Key;
using ::testing::Not;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;
using ::testing::WhenSorted;

class MatcherTest : public ::testing::Test {
 public:
  template<typename Matcher>
  static std::string Describe(const Matcher& matcher) {
    std::ostringstream os;
    matcher.DescribeTo(&os);
    return os.str();
  }

  template<typename Matcher>
  static std::string DescribeNegation(const Matcher& matcher) {
    std::ostringstream os;
    matcher.DescribeNegationTo(&os);
    return os.str();
  }

  template<typename Matcher, typename V>
  static std::pair<bool, std::string> MatchAndExplain(const Matcher& matcher, const V& value) {
    ::testing::StringMatchResultListener os;
    const bool result = matcher.MatchAndExplain(value, &os);
    return {result, os.str()};
  }
};

TEST_F(MatcherTest, CapacityIs) {
  EXPECT_THAT(std::vector<int>{}, CapacityIs(Ge(0)));
  EXPECT_THAT((std::vector<int>{1, 2}), CapacityIs(Ge(2)));
  std::vector<int> vector;
  vector.shrink_to_fit();
  const std::size_t before = vector.capacity();
  EXPECT_THAT(vector, CapacityIs(before));
  vector.reserve((1 + before) * 2);
  const std::size_t after = vector.capacity();
  ASSERT_THAT(before, Not(after));
  EXPECT_THAT(vector, CapacityIs(after));
}

TEST_F(MatcherTest, CapacityIsDescriptions) {
  {
    const ::testing::Matcher<std::vector<int>> matcher = CapacityIs(1);
    EXPECT_THAT(Describe(matcher), "has a capacity that is equal to 1");
    EXPECT_THAT(DescribeNegation(matcher), "has a capacity that isn't equal to 1");
    EXPECT_THAT(MatchAndExplain(matcher, std::vector<int>(1)), Pair(true, "whose capacity 1 matches"));
    EXPECT_THAT(MatchAndExplain(matcher, std::vector<int>(2, 2)), Pair(false, "whose capacity 2 doesn't match"));
  }
  {
    const ::testing::Matcher<std::vector<int>> matcher = CapacityIs(Ge(2));
    EXPECT_THAT(Describe(matcher), "has a capacity that is >= 2");
    EXPECT_THAT(DescribeNegation(matcher), "has a capacity that isn't >= 2");
  }
}

TEST_F(MatcherTest, WhenTransformedSameType) {
  {
    const std::set<int> set{1, 2};
    EXPECT_THAT(set, WhenTransformedBy([](int v) { return v + 2; }, ElementsAre(3, 4)));
  }
  {
    const std::vector<int> set{1, 2};
    EXPECT_THAT(set, WhenTransformedBy([](int v) { return v + 2; }, ElementsAre(3, 4)));
  }
  {
    const std::map<int, int> map{{1, 2}, {3, 4}};
    EXPECT_THAT(
        map, WhenTransformedBy(
                 [](const std::pair<int, int>& v) { return std::make_pair(v.second, v.first); },
                 ElementsAre(Pair(2, 1), Pair(4, 3))));
  }
}

TEST_F(MatcherTest, WhenTransformedConversion) {
  {
    std::set<int> set{1, 2};
    EXPECT_THAT(set, WhenTransformedBy([](int v) { return absl::StrCat(v + 2); }, ElementsAre("3", "4")));
  }
  {
    std::vector<int> set{1, 2};
    EXPECT_THAT(set, WhenTransformedBy([](int v) { return absl::StrCat(v + 2); }, ElementsAre("3", "4")));
  }
  {
    std::vector<int> set{1, 2};
    EXPECT_THAT(
        set, WhenTransformedBy([](int v) { return std::make_pair(v, v + 2); }, ElementsAre(Pair(1, 3), Pair(2, 4))));
  }
  {
    const std::map<int, int> map{{1, 2}, {3, 4}};
    EXPECT_THAT(
        map,
        WhenTransformedBy(
            [](const std::pair<int, int>& v) { return std::make_pair(absl::StrCat(v.second), absl::StrCat(v.first)); },
            ElementsAre(Pair("2", "1"), Pair("4", "3"))));
  }
  {
    const std::map<int, int> map{{1, 2}, {3, 4}};
    EXPECT_THAT(
        map,
        WhenTransformedBy(
            [](const std::pair<int, int>& v) { return std::make_pair(absl::StrCat(v.second), absl::StrCat(v.first)); },
            ElementsAre(Key("2"), Key("4"))));
  }
  {
    const std::map<int, int> map{{1, 2}, {3, 4}};
    EXPECT_THAT(
        map, WhenTransformedBy(
                 [](const std::pair<int, int>& v) { return absl::StrCat(v.first + v.second); }, ElementsAre("3", "7")));
  }
  {
    const std::vector<int> vector{4, 1, 2, 3, 0};
    EXPECT_THAT(vector, WhenTransformedBy([](int v) { return v; }, UnorderedElementsAre(0, 1, 2, 3, 4)));
    EXPECT_THAT(vector, WhenTransformedBy([](int v) { return v; }, WhenSorted(ElementsAre(0, 1, 2, 3, 4))));
  }
}

TEST_F(MatcherTest, WhenTransformedDescriptions) {
  {
    const ::testing::Matcher<std::vector<int>> matcher = WhenTransformedBy([](int) { return 0; }, IsEmpty());
    EXPECT_THAT(Describe(matcher), "when transformed is empty");
    EXPECT_THAT(DescribeNegation(matcher), "when transformed isn't empty");
    EXPECT_THAT(MatchAndExplain(matcher, std::vector<int>()), Pair(true, "which (when transformed) matches"));
    EXPECT_THAT(
        MatchAndExplain(matcher, std::vector<int>(1)),
        Pair(false, "which (when transformed) doesn't match, whose size is 1"));
    EXPECT_THAT(
        MatchAndExplain(matcher, std::vector<int>(2, 2)),
        Pair(false, "which (when transformed) doesn't match, whose size is 2"));
  }
  {
    const ::testing::Matcher<std::vector<int>> matcher = WhenTransformedBy([](int) { return 0; }, ElementsAre(0));
    EXPECT_THAT(Describe(matcher), "when transformed has 1 element that is equal to 0");
    EXPECT_THAT(DescribeNegation(matcher), "when transformed doesn't have 1 element, or\nelement #0 isn't equal to 0");
    EXPECT_THAT(MatchAndExplain(matcher, std::vector<int>()), Pair(false, "which (when transformed) doesn't match"));
    EXPECT_THAT(MatchAndExplain(matcher, std::vector<int>(1)), Pair(true, "which (when transformed) matches"));
    EXPECT_THAT(
        MatchAndExplain(matcher, std::vector<int>(2, 2)),
        Pair(false, "which (when transformed) doesn't match, which has 2 elements"));
  }
  {
    const ::testing::Matcher<std::vector<int>> matcher = WhenTransformedBy([](int v) { return v; }, ElementsAre(0));
    EXPECT_THAT(
        MatchAndExplain(matcher, std::vector<int>{0, 1, 2, 3, 4}),
        Pair(false, "which (when transformed) doesn't match, which has 5 elements"));
  }
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::testing
