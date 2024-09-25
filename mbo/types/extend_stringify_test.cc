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
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::types::types_internal::kStructNameSupport;
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

    int first{0};
    std::string_view second;
  };

  constexpr SuppressFieldNames kTest{
      .first = 25,
      .second = "42",
  };
  EXPECT_THAT(kTest.ToString(), R"({25, "42"})") << "HERE no compiler should print any field name.";
}

TEST_F(ExtenderStringifyTest, NoGetAbslStringifyOptions) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
  };

  ASSERT_FALSE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);
}

TEST_F(ExtenderStringifyTest, KeyNames) {
  // Demonstrates different parameters can be routed differently.
  // The test verifies that:
  // * based on field index (or name if available) different control can be returned.
  // * fields `one` and `two` have different key control, including field name overriding.
  // * field `tre` will be fully suppressed.
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
    int two = 25;
    int tre = 33;

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type&,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& /*defaults*/) {
      return tester->FieldOptions(idx, name);
    }
  };

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

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

  EXPECT_THAT(TestStruct{}.ToString(), "{__first..==11++__second..==25}");
}

TEST_F(ExtenderStringifyTest, FieldNames) {
  // Field names taken from a static array.
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
    int two = 25;
    int tre = 33;

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
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

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), "{ONE = 11, TWO = 25, three = 33}");
}

TEST_F(ExtenderStringifyTest, Shorten) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::string_view one = "1";
    std::string_view two = "22";
    std::string_view three = "333";
    std::string_view four = "4444";
    std::string_view five;

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
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

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = "1", two = "2...", three = "3**", four = "**", five = ""})");
}

TEST_F(ExtenderStringifyTest, ValueReplacement) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 1;
    std::string_view two = "22";
    std::vector<int> three = {331, 332, 333};
    std::vector<std::string_view> four{"41", "42", "43"};

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
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

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}

TEST_F(ExtenderStringifyTest, Container) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::vector<int> one = {1, 2, 3};
    std::vector<int> two = {};
    std::vector<int> tre = {1, 2, 3};

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
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

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = [1, 2], two = [], three = [1, 2]})");
}

TEST_F(ExtenderStringifyTest, MoreTypes) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    float one = 1.1;
    double two = 2.2;
    unsigned three = 3;
    char four = '4';

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& defaults) {
      return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
    }
  };

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({"one": 1.1, "two": 2.2, "three": 3, "four": '4'})");
}

TEST_F(ExtenderStringifyTest, MoreContainers) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::set<int> one = {1, 2};
    std::map<int, int> two = {{1, 2}, {3, 4}};
    std::vector<std::pair<int, int>> three = {{5, 6}};

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& defaults) {
      auto ret =
          WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
      if (idx == 2) {
        ret.special_pair_first = "Key";
        ret.special_pair_second = "Val";
      }
      return ret;
    }
  };

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({"one": [1, 2], "two": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "three": [{"Key": 5, "Val": 6}]})");
}

TEST_F(ExtenderStringifyTest, ContainersOfPairs) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::map<std::string_view, int> one = {{"a", 1}, {"b", 2}};
    std::vector<std::pair<std::string_view, int>> two = {{"c", 3}, {"d", 4}};

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& defaults) {
      return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
    }
  };

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({"one": {"a": 1, "b": 2}, "two": {"c": 3, "d": 4}})");
}

TEST_F(ExtenderStringifyTest, PrintWithControl) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 25;
  };

  const TestStruct v;
  EXPECT_THAT(v.ToString(WithFieldNames(AbslStringifyOptions::AsCpp(), {"one"})), R"({.one = 25})");
  EXPECT_THAT(v.ToString(WithFieldNames(AbslStringifyOptions::AsJson(), {"one"})), R"({"one": 25})");
  if constexpr (kStructNameSupport) {
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), R"({.one = 25})");
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), R"({"one": 25})");
  } else {
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), R"({25})");
    EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), R"({25})");
  }
}

TEST_F(ExtenderStringifyTest, NonLiteralFields) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::map<int, int> one = {{1, 2}, {2, 3}};
    std::unordered_map<int, int> two = {{3, 4}};
    std::string three = "three";

    static AbslStringifyOptions GetAbslStringifyOptions(const Type& v, std::size_t idx, std::string_view name) {
      return WithFieldNames(AbslStringifyOptions::AsCpp(), {"one", "two", "three"})(v, idx, name);
    }
  };

  if constexpr (kStructNameSupport) {
    ASSERT_THAT(types_internal::SupportsFieldNames<TestStruct>, true);
    ASSERT_THAT(types_internal::SupportsFieldNamesConstexpr<TestStruct>, false);
    ASSERT_THAT(::mbo::types::types_internal::GetFieldNames<TestStruct>(), ElementsAre("one", "two", "three"));
  }

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({.one = {{.first = 1, .second = 2}, {.first = 2, .second = 3}}, .two = {{.first = 3, .second = 4}}, .three = "three"})");
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

  static constexpr std::string_view kExpectedDef = R"({.one: 11, .two: 25, .three: {.four: 42}})";
  static constexpr std::string_view kExpectedCpp = R"({.one = 11, .two = 25, .three = {.four = 42}})";
  static constexpr std::string_view kExpectedJson = R"({"one": 11, "two": 25, "three": {"four": 42}})";
  EXPECT_THAT(v.ToString(), kExpectedDef);
  EXPECT_THAT(v.ToString(AbslStringifyOptions::AsCpp()), kExpectedCpp);
  EXPECT_THAT(v.ToString(AbslStringifyOptions::AsJson()), kExpectedJson);
  EXPECT_THAT(v.ToJsonString(), kExpectedJson);
}

TEST_F(ExtenderStringifyTest, CustomNestedJson) {
  struct TestNested : mbo::types::Extend<TestNested> {
    int first = 0;
    std::string second = "nested";

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& defaults) {
      return WithFieldNames(
          AbslStringifyOptions::AsJson(), {"NESTED_1", "NESTED_2"}, AbslStringifyNameHandling::kOverwrite)(
          v, idx, name, defaults);
    }
  };

  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 123;
    std::string two = "test";
    std::array<bool, 2> three = {false, true};
    std::vector<TestNested> four = {{.first = 25, .second = "foo"}, {.first = 42, .second = "bar"}};
    std::pair<int, int> five = {25, 42};

    static AbslStringifyOptions GetAbslStringifyOptions(
        const Type& v,
        std::size_t idx,
        std::string_view name,
        const AbslStringifyOptions& defaults) {
      return WithFieldNames(AbslStringifyOptions::AsJson(), {"one", "two", "three", "four", "five"})(
          v, idx, name, defaults);
    }
  };

  ASSERT_TRUE(mbo::types::HasGetAbslStringifyOptions<TestStruct>);

  // Keys for nested "four" should get their key names as "first" and "second" since they are not provided.
  // No handover of concrete values to defaults should occur.
  // BUT We want to check whether the new defaults are being picked up!
  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {"first": 25, "second": 42}})");
  //               {"one": 123, "two": "test", "three": [false, true], "four": [{"four": 25, "four": "foo"}, {"four":42,
  //               "four": "bar"}],        "five": {"first": 25, "second": 42}}
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace
}  // namespace mbo::types

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
