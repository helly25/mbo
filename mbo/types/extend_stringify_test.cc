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

#include <initializer_list>
#include <set>
#include <string_view>
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
    MOCK_METHOD(AbslStringifyFieldOptions, FieldOptions, (std::size_t, std::string_view));
  };

  // Adds fieldnames to the passed in options - BUT only if fields names are not supported.
  static AbslStringifyFieldOptions SetFieldName(
      AbslStringifyFieldOptions options,
      std::size_t index,
      std::initializer_list<std::string_view> field_names) {
    if constexpr (!kStructNameSupport) {
      options.key_use_name = index >= field_names.size() ? std::string_view{} : *(field_names.begin() + index);
    }
    return options;
  }

  static Tester tester;
};

ExtenderStringifyTest::Tester ExtenderStringifyTest::tester;

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
  EXPECT_THAT(kTest.ToString(), R"({25, "42"})") << "No compiler should print any field name.";
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

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type&,
        std::size_t field_index,
        std::string_view field_name) {
      return tester.FieldOptions(field_index, field_name);
    }
  };

  EXPECT_CALL(tester, FieldOptions(0, HasFieldName("one")))
      .WillOnce(::testing::Return(AbslStringifyFieldOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
          .key_use_name = "first",
      }));
  EXPECT_CALL(tester, FieldOptions(1, HasFieldName("two")))
      .WillOnce(::testing::Return(AbslStringifyFieldOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
          .key_use_name = "second",
      }));
  EXPECT_CALL(tester, FieldOptions(2, HasFieldName("tre")))
      .WillOnce(::testing::Return(AbslStringifyFieldOptions{
          .field_suppress = true,
      }));

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), "{__first..==11++__second..==25}");
}

TEST_F(ExtenderStringifyTest, FieldNames) {
  // Field names taken from a static array.
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 11;
    int two = 25;
    int tre = 33;

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(const Type&, std::size_t idx, std::string_view) {
      static constexpr std::array<std::string_view, 3> kFieldNames{
          "ONE",
          "TWO",
          "three",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_use_name = kFieldNames[idx],  // NOLINT(*-constant-array-index)
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), "{ONE = 11, TWO = 25, three = 33}");
}

TEST_F(ExtenderStringifyTest, Shorten) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::string_view one = "1";
    std::string_view two = "22";
    std::string_view tre = "333";
    std::string_view fou = "4444";
    std::string_view fiv;

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 5> kFieldNames{
          "one", "two", "three", "four", "five",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_use_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
          .value_max_length = field_index >= 3 && field_index <= 4 ? 0U : 1U,
          .value_cutoff_suffix = field_index < 2 ? AbslStringifyFieldOptions::Default().value_cutoff_suffix : "**",
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = "1", two = "2...", three = "3**", four = "**", five = ""})");
}

TEST_F(ExtenderStringifyTest, ValueReplacement) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 1;
    std::string_view two = "22";
    std::vector<int> tre = {331, 332, 333};
    std::vector<std::string_view> fou{"41", "42", "43"};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 4> kFieldNames{
          "one",
          "two",
          "three",
          "four",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_use_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
          .value_replacement_str = "<XX>",
          .value_replacement_other = "<YY>"};
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}

TEST_F(ExtenderStringifyTest, Container) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    std::vector<int> one = {1, 2, 3};
    std::vector<int> two = {};
    std::vector<int> tre = {1, 2, 3};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 3> kFieldNames{
          "one",
          "two",
          "three",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_use_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
          .value_container_prefix = "[",
          .value_container_suffix = "]",
          .value_container_max_len = field_index == 1 ? 0U : 2U,
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = [1, 2], two = [], three = [1, 2]})");
}

TEST_F(ExtenderStringifyTest, Json) {
  struct TestNested : mbo::types::Extend<TestNested> {
    int first = 0;
    std::string second = "nested";

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(const Type&, std::size_t idx, std::string_view) {
      return SetFieldName(AbslStringifyFieldOptions::Json(), idx, {"first", "second"});
    }
  };

  struct TestStruct : mbo::types::Extend<TestStruct> {
    int one = 123;
    std::string two = "test";
    std::array<bool, 2> three = {false, true};
    std::vector<TestNested> four = {{.first = 25, .second = "foo"}, {.first = 42, .second = "bar"}};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(const Type&, std::size_t idx, std::string_view) {
      return SetFieldName(AbslStringifyFieldOptions::Json(), idx, {"one", "two", "three", "four"});
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(
      TestStruct{}.ToString(),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"first": 25, "second": "foo"}, {"first": 42, "second": "bar"}]})");
}

TEST_F(ExtenderStringifyTest, MoreTypes) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    float one = 1.1;
    double two = 2.2;
    std::set<unsigned> three = {1, 2};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(const Type&, std::size_t idx, std::string_view) {
      return SetFieldName(AbslStringifyFieldOptions::Json(), idx, {"one", "two", "three", "four"});
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({"one": 1.1, "two": 2.2, "three": [1, 2]})");
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace
}  // namespace mbo::types

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
