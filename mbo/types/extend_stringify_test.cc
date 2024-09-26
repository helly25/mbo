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

#include <map>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "absl/log/absl_check.h"  // IWYU pragma: keep
#include "absl/log/absl_log.h"    // IWYU pragma: keep
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/extend.h"
#include "mbo/types/extender.h"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wunused-local-typedef"
# if __APPLE_CC__
#  // There is an issue with ADL members here.
#  pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#  pragma clang diagnostic ignored "-Wunused-function"
# endif
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

// namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::types::AbslStringifyNameHandling;
using ::mbo::types::AbslStringifyOptions;
using ::mbo::types::HasMboTypesExtendDoNotPrintFieldNames;
using ::mbo::types::HasMboTypesExtendStringifyOptions;
using ::mbo::types::types_internal::kStructNameSupport;
using ::testing::ElementsAre;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

// Matcher that checks the field name matches if field names are supported, or verifies that the
// passed field_name is in fact empty.
MATCHER_P(HasFieldName, field_name, "") {
  if constexpr (kStructNameSupport) {
    return ::testing::ExplainMatchResult(field_name, arg, result_listener);
  } else {
    return ::testing::ExplainMatchResult(IsEmpty(), arg, result_listener);
  }
}

struct ExtenderStringifyTest : ::testing::Test {
  struct Tester {
    MOCK_METHOD(AbslStringifyOptions, FieldOptions, (std::size_t, std::string_view));
  };

  ExtenderStringifyTest() { tester = new Tester(); }

  ExtenderStringifyTest(const ExtenderStringifyTest&) noexcept = delete;
  ExtenderStringifyTest& operator=(const ExtenderStringifyTest&) noexcept = delete;
  ExtenderStringifyTest(ExtenderStringifyTest&&) noexcept = delete;
  ExtenderStringifyTest& operator=(ExtenderStringifyTest&&) noexcept = delete;

  ~ExtenderStringifyTest() override {
    delete tester;
    tester = nullptr;
  }

  static Tester* tester;
};

ExtenderStringifyTest::Tester* ExtenderStringifyTest::tester = nullptr;

TEST_F(ExtenderStringifyTest, SuppressFieldNames) {
  struct SuppressFieldNames : ::mbo::types::Extend<SuppressFieldNames> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    int one{0};
    std::string_view two;
  };

  constexpr SuppressFieldNames kTest{
      .one = 25,
      .two = "42",
  };
  ASSERT_THAT(HasMboTypesExtendDoNotPrintFieldNames<decltype(kTest)>, true);
  EXPECT_THAT(kTest.ToString(), R"({25, "42"})") << "  NOTE: Here no compiler should print any field name.";
}

TEST_F(ExtenderStringifyTest, NoMboTypesExtendStringifyOptions) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
  };

  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<TestStruct>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<TestStruct>);
}

struct TestStructKeyNames : mbo::types::Extend<TestStructKeyNames> {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructKeyNames&,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& /*defaults*/) {
    return ExtenderStringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(ExtenderStringifyTest, KeyNames) {
  // Demonstrates different parameters can be routed differently.
  // The test verifies that:
  // * based on field index (or name if available) different control can be returned.
  // * fields `one` and `two` have different key control, including field name overriding.
  // * field `tre` will be fully suppressed.
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructKeyNames>);

  EXPECT_CALL(*tester, FieldOptions(0, HasFieldName("one")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
          .key_use_name = "first",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, HasFieldName("two")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
          .key_use_name = "second",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, HasFieldName("tre")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = true,
      }));

  EXPECT_THAT(TestStructKeyNames{}.ToString(), "{__first..==11++__second..==25}");
}

struct TestStruct : mbo::types::Extend<TestStruct> {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStruct& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(
        AbslStringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
        },
        {"ONE", "TWO", "three"}, AbslStringifyNameHandling::kOverwrite)(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, FieldNames) {
  // Field names taken from a static array.
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStruct>);
  EXPECT_THAT(TestStruct{}.ToString(), "{ONE = 11, TWO = 25, three = 33}");
}

struct TestStructShorten : mbo::types::Extend<TestStructShorten> {
  std::string_view one = "1";
  std::string_view two = "22";
  std::string_view three = "333";
  std::string_view four = "4444";
  std::string_view five;

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructShorten& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(
        AbslStringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_max_length = idx >= 3 && idx <= 4 ? 0U : 1U,
            .value_cutoff_suffix = idx < 2 ? AbslStringifyOptions::AsDefault().value_cutoff_suffix : "**",
        },
        {"one", "two", "three", "four", "five"})(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, Shorten) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructShorten>);
  EXPECT_THAT(TestStructShorten{}.ToString(), R"({one = "1", two = "2...", three = "3**", four = "**", five = ""})");
}

struct TestStructValueReplacement : mbo::types::Extend<TestStructValueReplacement> {
  int one = 1;
  std::string_view two = "22";
  std::vector<int> three = {331, 332, 333};
  std::vector<std::string_view> four{"41", "42", "43"};

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructValueReplacement& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(
        AbslStringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_replacement_str = "<XX>",
            .value_replacement_other = "<YY>",
        },
        {"one", "two", "three", "four"})(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, ValueReplacement) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructValueReplacement>);
  EXPECT_THAT(
      TestStructValueReplacement{}.ToString(),
      R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}

struct TestStructContainer : mbo::types::Extend<TestStructContainer> {
  std::vector<int> one = {1, 2, 3};
  std::vector<int> two = {};
  std::vector<int> tre = {1, 2, 3};

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructContainer& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(
        AbslStringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_container_prefix = "[",
            .value_container_suffix = "]",
            .value_container_max_len = idx == 1 ? 0U : 2U,
        },
        {"one", "two", "three"}, AbslStringifyNameHandling::kOverwrite)(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, Container) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructContainer>);
  EXPECT_THAT(TestStructContainer{}.ToString(), R"({one = [1, 2], two = [], three = [1, 2]})");
}

struct TestStructMoreTypes : mbo::types::Extend<TestStructMoreTypes> {
  float one = 1.1;
  double two = 2.2;
  unsigned three = 3;
  char four = '4';

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructMoreTypes& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, MoreTypes) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructMoreTypes>);
  EXPECT_THAT(TestStructMoreTypes{}.ToString(), R"({"one": 1.1, "two": 2.2, "three": 3, "four": '4'})");
}

struct TestStructMoreContainers : mbo::types::Extend<TestStructMoreContainers> {
  std::set<int> one = {1, 2};
  std::map<int, int> two = {{1, 2}, {3, 4}};
  std::vector<std::pair<int, int>> three = {{5, 6}};

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructMoreContainers& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    auto ret = WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
    if (idx == 2) {
      ret.special_pair_first = "Key";
      ret.special_pair_second = "Val";
    }
    return ret;
  }
};

TEST_F(ExtenderStringifyTest, MoreContainers) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructMoreContainers>);
  EXPECT_THAT(
      TestStructMoreContainers{}.ToString(),
      R"({"one": [1, 2], "two": [{.first: 1, .second: 2}, {.first: 3, .second: 4}], "three": [{.Key: 5, .Val: 6}]})")
      << "  NOTE: Here we are not providing the defualt Json options down do the pairs. However, in `three` we have "
         "the provided key/value names.";
  EXPECT_THAT(
      TestStructMoreContainers{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"one": [1, 2], "two": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "three": [{"Key": 5, "Val": 6}]})");
}

struct TestStructMoreContainersWithDirectFieldNames : mbo::types::Extend<TestStructMoreContainersWithDirectFieldNames> {
  std::set<int> one = {1, 2};
  std::map<int, int> two = {{1, 2}, {3, 4}};
  std::vector<std::pair<int, int>> three = {{5, 6}};

  friend auto MboTypesExtendFieldNames(const TestStructMoreContainersWithDirectFieldNames& v) {
    (void)v;
    return std::array<std::string_view, 3>{"1", "2", "3"};
  }

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructMoreContainersWithDirectFieldNames&,
      std::size_t idx,
      std::string_view,
      const AbslStringifyOptions& defaults) {
    AbslStringifyOptions ret = defaults;
    if (idx == 2) {
      ret.special_pair_first = "Key";
      ret.special_pair_second = "Val";
    }
    return ret;
  }
};

TEST_F(ExtenderStringifyTest, MoreContainersWithDirectFieldNames) {
  static_assert(!mbo::types::HasMboTypesExtendDoNotPrintFieldNames<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(mbo::types::HasMboTypesExtendStringifyOptions<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(mbo::types::HasMboTypesExtendFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_TRUE(mbo::types::HasMboTypesExtendFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_THAT(MboTypesExtendFieldNames(TestStructMoreContainersWithDirectFieldNames{}), ElementsAre("1", "2", "3"));
  EXPECT_THAT(
      TestStructMoreContainersWithDirectFieldNames{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"1": [1, 2], "2": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "3": [{"Key": 5, "Val": 6}]})");
}

struct TestStructContainersOfPairs : mbo::types::Extend<TestStructContainersOfPairs> {
  std::map<std::string_view, int> one = {{"a", 1}, {"b", 2}};
  std::vector<std::pair<std::string_view, int>> two = {{"c", 3}, {"d", 4}};

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructContainersOfPairs& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, ContainersOfPairs) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructContainersOfPairs>);
  EXPECT_THAT(TestStructContainersOfPairs{}.ToString(), R"({"one": {"a": 1, "b": 2}, "two": {"c": 3, "d": 4}})");
}

TEST_F(ExtenderStringifyTest, PrintWithControl) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 25;
  };

  const TestStruct v;
  if constexpr (kStructNameSupport) {
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), R"({.one = 25})");
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), R"({"one": 25})");
  } else {
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), R"({25})");
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), R"({25})");
  }
}

TEST_F(ExtenderStringifyTest, NestedDefaults) {
  struct TestSub : mbo::types::Extend<TestSub> {
    int four = 42;
  };

  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
    int two = 25;
    TestSub three = {.four = 42};
  };

  const TestStruct v;

  if constexpr (kStructNameSupport) {
    static constexpr std::string_view kExpectedDef = R"({.one: 11, .two: 25, .three: {.four: 42}})";
    static constexpr std::string_view kExpectedCpp = R"({.one = 11, .two = 25, .three = {.four = 42}})";
    static constexpr std::string_view kExpectedJson = R"({"one": 11, "two": 25, "three": {"four": 42}})";
    EXPECT_THAT(v.ToString(), kExpectedDef);
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), kExpectedCpp);
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), kExpectedJson);
    EXPECT_THAT(v.ToJsonString(), kExpectedJson);
  }
}

struct TestStructCustomNestedJsonNested : mbo::types::Extend<TestStructCustomNestedJsonNested> {
  int first = 0;
  std::string second = "nested";

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructCustomNestedJsonNested& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(
        AbslStringifyOptions::AsJson(), {"NESTED_1", "NESTED_2"}, AbslStringifyNameHandling::kOverwrite)(
        v, idx, name, defaults);
  }
};

struct TestStructCustomNestedJson : mbo::types::Extend<TestStructCustomNestedJson> {
  int one = 123;
  std::string two = "test";
  std::array<bool, 2> three = {false, true};
  std::vector<TestStructCustomNestedJsonNested> four = {{.first = 25, .second = "foo"}, {.first = 42, .second = "bar"}};
  std::pair<int, int> five = {25, 42};

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructCustomNestedJson& v,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& defaults) {
    return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four", "five"})(
        v, idx, name, defaults);
  }
};

TEST_F(ExtenderStringifyTest, CustomNestedJson) {
  ASSERT_TRUE(mbo::types::HasMboTypesExtendStringifyOptions<TestStructCustomNestedJson>);

  // Keys for nested "four" should get their key names as "first" and "second" since they are not provided.
  // No handover of concrete values to defaults should occur.
  // BUT: Five uses non JSON mode as we fallback to the default options which were not set.
  EXPECT_THAT(
      TestStructCustomNestedJson{}.ToString(),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {.first: 25, .second: 42}})");
  //               "{"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"},
  //               {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {"first": 25, "second": 42}}"

  //               "{"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"},
  //               {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {.first: 25, .second: 42}}"
  EXPECT_THAT(
      TestStructCustomNestedJson{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {"first": 25, "second": 42}})");
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace

//}  // namespace mbo::types

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
