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
#endif

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers)

struct ExtenderStringifyTest : ::testing::Test {};

TEST_F(ExtenderStringifyTest, Basic) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    int number = 25;
    std::string str = "42";
  };

  EXPECT_THAT(TestStruct{}.ToString(), "{25, \"42\"}");
}

TEST_F(ExtenderStringifyTest, KeyNames) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    int one = 11;
    int two = 25;
    int tre = 33;

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view field_name) {
      ABSL_CHECK(field_name.empty());
      return {
          .field_suppress = field_index > 1,
          .field_separator = "++",
          .key_prefix = "__",
          .key_value_separator = "..==",
          .key_alternative_name = field_index == 0 ? "first" : "second",
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), "{__first..==11++__second..==25}");
}

TEST_F(ExtenderStringifyTest, FieldNames) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    int one = 11;
    int two = 25;
    int tre = 33;

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 3> kFieldNames{
          "ONE",
          "TWO",
          "three",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_alternative_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), "{ONE = 11, TWO = 25, three = 33}");
}

TEST_F(ExtenderStringifyTest, Shorten) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    using MboTypesExtendDoNotPrintFieldNames = void;

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
          .key_alternative_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
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
    using MboTypesExtendDoNotPrintFieldNames = void;

    int one = 1;
    std::string_view two = "22";
    std::vector<int> tre = {331, 332, 333};
    std::vector<std::string_view> fou{"41", "42", "43"};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 4> kFieldNames{
          "one", "two", "three", "four",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_alternative_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
          .value_replacement_str = "<XX>",
          .value_replacement_other = "<YY>"
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}


TEST_F(ExtenderStringifyTest, Container) {
  struct TestStruct : mbo::types::Extend<TestStruct> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    std::vector<int> one = {1, 2, 3};
    std::vector<int> two = {};
    std::vector<int> tre = {1, 2, 3};

    static struct AbslStringifyFieldOptions AbslStringifyFieldOptions(
        const Type& /*unused*/,  // The type `Type` is provided by `Extend`.
        std::size_t field_index,
        std::string_view /* unused */) {
      static constexpr std::array<std::string_view, 3> kFieldNames{
          "one", "two", "three",
      };
      return {
          .key_prefix = "",
          .key_value_separator = " = ",
          .key_alternative_name = kFieldNames[field_index],  // NOLINT(*-constant-array-index)
          .value_container_prefix = "[",
          .value_container_suffix = "]",
          .value_container_max_len = field_index == 1 ? 0U : 2U,
      };
    }
  };

  ASSERT_TRUE(mbo::types::HasAbslStringifyFieldOptions<TestStruct>);

  EXPECT_THAT(TestStruct{}.ToString(), R"({one = [1, 2], two = [], three = [1, 2]})");
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types

#ifdef __clang__
# pragma clang diagnostic pop
#endif
