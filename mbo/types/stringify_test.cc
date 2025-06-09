// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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

#include "mbo/types/stringify.h"

#include <array>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"  // IWYU pragma: keep
#include "absl/log/absl_log.h"    // IWYU pragma: keep
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/container/limited_vector.h"
#include "mbo/testing/matchers.h"
#include "mbo/types/traits.h"
#include "mbo/types/tuple_extras.h"

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wunused-local-typedef"
# // There is an issue with ADL members here.
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
# pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif  // defined(__clang__)

namespace mbo_other {  // Not using namespace mbo::types

namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::testing::EqualsText;
using ::mbo::types::HasMboTypesStringifyDoNotPrintFieldNames;
using ::mbo::types::HasMboTypesStringifyFieldNames;
using ::mbo::types::HasMboTypesStringifyOptions;
using ::mbo::types::HasMboTypesStringifySupport;
using ::mbo::types::Stringify;
using ::mbo::types::StringifyNameHandling;
using ::mbo::types::StringifyOptions;
using ::mbo::types::StringifyWithFieldNames;
using ::mbo::types::types_internal::GetFieldNames;
using ::mbo::types::types_internal::kStructNameSupport;
using ::mbo::types::types_internal::SupportsFieldNames;
using ::mbo::types::types_internal::SupportsFieldNamesConstexpr;
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

struct StringifyTest : ::testing::Test {
  struct Tester {
    MOCK_METHOD(std::vector<std::string>, FieldNames, ());
    MOCK_METHOD(StringifyOptions, FieldOptions, (std::size_t, std::string_view));
  };

  StringifyTest() { tester = new Tester(); }  // NOLINT(cppcoreguidelines-owning-memory)

  StringifyTest(const StringifyTest&) noexcept = delete;
  StringifyTest& operator=(const StringifyTest&) noexcept = delete;
  StringifyTest(StringifyTest&&) noexcept = delete;
  StringifyTest& operator=(StringifyTest&&) noexcept = delete;

  ~StringifyTest() override {
    delete tester;  // NOLINT(cppcoreguidelines-owning-memory)
    tester = nullptr;
  }

  template<typename T>
  static std::string DecomposeInfo() {
    return absl::StrCat("  DecomposeInfo: {\n", mbo::types::types_internal::DecomposeInfo<T>::Debug("\n    "), "  \n}");
  }

  static Tester* tester;
};

StringifyTest::Tester* StringifyTest::tester = nullptr;

TEST_F(StringifyTest, AllExtensionApiPointsAbsent) {
  struct TestStruct {
    int one = 11;
  };

  EXPECT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<TestStruct>);
  EXPECT_FALSE(HasMboTypesStringifyFieldNames<TestStruct>);
  EXPECT_FALSE(HasMboTypesStringifyOptions<TestStruct>);
  EXPECT_FALSE(HasMboTypesStringifySupport<TestStruct>);
}

TEST_F(StringifyTest, SuppressFieldNames) {
  struct SuppressFieldNames {
    using MboTypesStringifyDoNotPrintFieldNames = void;

    int one = 25;
    std::string_view two = "42";
  };

  ASSERT_TRUE(HasMboTypesStringifyDoNotPrintFieldNames<SuppressFieldNames>);
  EXPECT_FALSE(HasMboTypesStringifyFieldNames<SuppressFieldNames>);
  EXPECT_FALSE(HasMboTypesStringifyOptions<SuppressFieldNames>);
  EXPECT_TRUE(HasMboTypesStringifySupport<SuppressFieldNames>);
  EXPECT_THAT(Stringify().ToString(SuppressFieldNames{}), R"({25, "42"})")
      << "  NOTE: Here no compiler should print any field name.";
}

struct AddFieldNames {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesStringifyFieldNames(const AddFieldNames&) { return std::array<std::string_view, 2>{"1", "2"}; }
};

TEST_F(StringifyTest, AddFieldNames) {
  // Proves the straight forward type: a `std::array<std::string, N>` works.
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<AddFieldNames>);
  ASSERT_TRUE(HasMboTypesStringifyFieldNames<AddFieldNames>);
  ASSERT_FALSE(HasMboTypesStringifyOptions<AddFieldNames>);
  EXPECT_TRUE(HasMboTypesStringifySupport<AddFieldNames>);
  EXPECT_THAT(Stringify().ToString(AddFieldNames{}), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
  EXPECT_THAT(Stringify::AsJsonPretty().ToString(AddFieldNames{}), EqualsText(R"({
  "1": 25,
  "2": "42"
}
)"));
}

struct AddFieldVectorOfString {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesStringifyFieldNames(const AddFieldVectorOfString&) { return std::vector<std::string>{"1", "2"}; }
};

TEST_F(StringifyTest, AddFieldVectorOfString) {
  // This test proves that conversion from temp strings works through lifetime extension.
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<AddFieldVectorOfString>);
  ASSERT_TRUE(HasMboTypesStringifyFieldNames<AddFieldVectorOfString>);
  ASSERT_FALSE(HasMboTypesStringifyOptions<AddFieldVectorOfString>);
  EXPECT_TRUE(HasMboTypesStringifySupport<AddFieldVectorOfString>);
  EXPECT_THAT(Stringify().ToString(AddFieldVectorOfString{}), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
}

struct AddFieldNamesLimitedVector {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesStringifyFieldNames(const AddFieldNamesLimitedVector&) {
    return mbo::container::MakeLimitedVector("1", "2");
  }
};

TEST_F(StringifyTest, AddFieldNamesLimitedVector) {
  // Proves other types can be compatible.
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<AddFieldNamesLimitedVector>);
  ASSERT_TRUE(HasMboTypesStringifyFieldNames<AddFieldNamesLimitedVector>);
  ASSERT_FALSE(HasMboTypesStringifyOptions<AddFieldNamesLimitedVector>);
  EXPECT_TRUE(HasMboTypesStringifySupport<AddFieldNamesLimitedVector>);
  EXPECT_THAT(Stringify().ToString(AddFieldNamesLimitedVector{}), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
}

struct TestStructFieldOptions {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructFieldOptions&,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& /*defaults*/) {
    return StringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(StringifyTest, FieldOptions) {
  // Demonstrates different parameters can be routed differently.
  // The test verifies that:
  // * based on field index (or name if available) different control can be returned.
  // * fields `one` and `two` have different key control, including field name overriding.
  // * field `tre` will be fully suppressed.
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<TestStructFieldOptions>);
  ASSERT_FALSE(HasMboTypesStringifyFieldNames<TestStructFieldOptions>);
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructFieldOptions>);
  EXPECT_TRUE(HasMboTypesStringifySupport<TestStructFieldOptions>);

  EXPECT_CALL(*tester, FieldOptions(0, HasFieldName("one")))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = false,
          .field_separator = "+1+",
          .key_prefix = "_1_",
          .key_suffix = ".1.",
          .key_value_separator = "=1=",
          .key_use_name = "first",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, HasFieldName("two")))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = false,
          .field_separator = "+2+",
          .key_prefix = "_2_",
          .key_suffix = ".2.",
          .key_value_separator = "=2=",
          .key_use_name = "second",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, HasFieldName("tre")))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = true,
      }));

  EXPECT_THAT(Stringify().ToString(TestStructFieldOptions{}), "{_1_first.1.=1=11+2+_2_second.2.=2=25}");
}

struct TestStructFieldNames {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend std::vector<std::string> MboTypesStringifyFieldNames(const TestStructFieldNames&) {
    return StringifyTest::tester->FieldNames();
  }

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructFieldNames&,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& /*defaults*/) {
    return StringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(StringifyTest, FieldNames) {
  // Demonstrates different parameters can be routed differently.
  // Unlike test `FieldOptions` here we first fetch the field names which override compiler provided ones.
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<TestStructFieldNames>);
  ASSERT_TRUE(HasMboTypesStringifyFieldNames<TestStructFieldNames>);
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructFieldNames>);
  EXPECT_TRUE(HasMboTypesStringifySupport<TestStructFieldNames>);

  // 1st ToString call.
  EXPECT_CALL(*tester, FieldNames()).WillOnce(::testing::Return(std::vector<std::string>{"First", "Second", "Third"}));
  EXPECT_CALL(*tester, FieldOptions(0, "First"))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_separator = "+1+",
          .key_prefix = "_1_",
          .key_suffix = ".1.",
          .key_value_separator = "=1=",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, "Second"))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = true,
          .field_separator = "+2+",
          .key_prefix = "_2_",
          .key_suffix = ".2.",
          .key_value_separator = "=2=",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, "Third"))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_separator = "+3+",
          .key_prefix = "_3_",
          .key_suffix = ".3.",
          .key_value_separator = "=3=",
      }));

  EXPECT_THAT(Stringify().ToString(TestStructFieldNames{}), "{_1_First.1.=1=11+3+_3_Third.3.=3=33}");

  // 2nd ToString call.
  EXPECT_CALL(*tester, FieldNames()).WillOnce(::testing::Return(std::vector<std::string>{"Fourth"}));
  EXPECT_CALL(*tester, FieldOptions(0, "Fourth"))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_separator = "+4+",
          .key_prefix = "_4_",
          .key_suffix = ".4.",
          .key_value_separator = "=4=",
      }));
  // 2nd and 3rd field get printed. But 2nd has no key name, so related options get ignored.
  EXPECT_CALL(*tester, FieldOptions(1, IsEmpty()))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_separator = "+5+",
          .key_prefix = "_5_",
          .key_suffix = ".5.",
          .key_value_separator = "=5=",
      }));
  // For the 3rd field, the options provide the field name through `key_use_name`.
  EXPECT_CALL(*tester, FieldOptions(2, IsEmpty()))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_separator = "+6+",
          .key_prefix = "_6_",
          .key_suffix = ".6.",
          .key_value_separator = "=6=",
          .key_use_name = "Sixth",
      }));
  EXPECT_THAT(Stringify().ToString(TestStructFieldNames{}), "{_4_Fourth.4.=4=11+5+25+6+_6_Sixth.6.=6=33}");
}

struct TestStructDoNotPrintFieldNames {
  int one = 11;
  int two = 25;
  int tre = 33;

  using MboTypesStringifyDoNotPrintFieldNames = void;

  // It is not allowed to also have the API extension point `MboTypesStringifyFieldNames`.

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructDoNotPrintFieldNames&,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& /*defaults*/) {
    return StringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(StringifyTest, DoNotPrintFieldNames) {
  // Demonstrates different parameters can be routed differently.
  // Unlike test `FieldOptions` here we first fetch the field names which override compiler provided ones.
  ASSERT_TRUE(HasMboTypesStringifyDoNotPrintFieldNames<TestStructDoNotPrintFieldNames>);
  ASSERT_FALSE(HasMboTypesStringifyFieldNames<TestStructDoNotPrintFieldNames>);
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructDoNotPrintFieldNames>);
  EXPECT_TRUE(HasMboTypesStringifySupport<TestStructDoNotPrintFieldNames>);

  EXPECT_CALL(*tester, FieldOptions(0, IsEmpty()))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, IsEmpty()))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, IsEmpty()))
      .WillOnce(::testing::Return(StringifyOptions{
          .field_suppress = true,
      }));

  EXPECT_THAT(Stringify().ToString(TestStructDoNotPrintFieldNames{}), "{11++25}");
}

struct TestStructShorten {
  std::string_view one = "1";
  std::string_view two = "22";
  std::string_view three = "333";
  std::string_view four = "4444";
  std::string_view five;

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructShorten& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(
        StringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_max_length = idx >= 3 && idx <= 4 ? 0U : 1U,
            .value_cutoff_suffix = idx < 2 ? StringifyOptions::AsDefault().value_cutoff_suffix : "**",
        },
        {"one", "two", "three", "four", "five"})(v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, Shorten) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructShorten>);
  EXPECT_TRUE(::mbo::types::CanCreateTuple<TestStructShorten>);
  EXPECT_THAT(mbo::types::DecomposeCountV<TestStructShorten>, 5) << DecomposeInfo<TestStructShorten>();
  if constexpr (mbo::types::DecomposeCountV<TestStructShorten> == 5) {
    EXPECT_FALSE(::mbo::types::HasUnionMember<TestStructShorten>);
    EXPECT_THAT(SupportsFieldNames<TestStructShorten>, kStructNameSupport);
    EXPECT_THAT(SupportsFieldNamesConstexpr<TestStructShorten>, kStructNameSupport);
    if constexpr (SupportsFieldNames<TestStructShorten>) {
      EXPECT_THAT(GetFieldNames<TestStructShorten>(), ElementsAre("one", "two", "three", "four", "five"));
    }
    EXPECT_THAT(
        Stringify().ToString(TestStructShorten{}),
        R"({one = "1", two = "2...", three = "3**", four = "**", five = ""})");
  }
}

struct TestStructValueReplacement {
  int one = 1;
  std::string_view two = "22";
  std::vector<int> three = {331, 332, 333};
  std::vector<std::string_view> four{"41", "42", "43"};

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructValueReplacement& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(
        StringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_replacement_str = "<XX>",
            .value_replacement_other = "<YY>",
        },
        {"one", "two", "three", "four"})(v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, ValueReplacement) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructValueReplacement>);
  EXPECT_THAT(
      Stringify().ToString(TestStructValueReplacement{}),
      R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}

struct TestStructContainer {
  std::vector<int> one = {1, 2, 3};
  std::vector<int> two;
  std::vector<int> tre = {1, 2, 3};

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructContainer& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(
        StringifyOptions{
            .key_prefix = "",
            .key_value_separator = " = ",
            .value_container_prefix = "[",
            .value_container_suffix = "]",
            .value_container_max_len = idx == 1 ? 0U : 2U,
        },
        {"one", "two", "three"}, StringifyNameHandling::kOverwrite)(v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, Container) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructContainer>);
  EXPECT_THAT(Stringify().ToString(TestStructContainer{}), R"({one = [1, 2], two = [], three = [1, 2]})");
}

struct TestStructMoreTypes {
  float one = 1.1;
  double two = 2.2;
  unsigned three = 3;
  char four = '4';
  unsigned char five = '5';

  friend auto MboTypesStringifyFieldNames(const TestStructMoreTypes&) {
    return std::array<std::string_view, 5>{"one", "two", "three", "four", "five"};
  }
};

TEST_F(StringifyTest, MoreTypes) {
  ASSERT_TRUE(HasMboTypesStringifyFieldNames<TestStructMoreTypes>);
  ASSERT_FALSE(HasMboTypesStringifyOptions<TestStructMoreTypes>);
  EXPECT_THAT(
      Stringify().ToString(TestStructMoreTypes{}), R"({.one: 1.1, .two: 2.2, .three: 3, .four: '4', .five: '5'})");
}

struct TestStructMoreContainers {
  std::set<int> one = {1, 2};
  std::map<int, int> two = {{1, 2}, {3, 4}};
  std::vector<std::pair<int, int>> three = {{5, 6}};

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructMoreContainers& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    auto ret =
        StringifyWithFieldNames(StringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
    if (idx == 2) {
      ret.special_pair_first = "Key";
      ret.special_pair_second = "Val";
    }
    return ret;
  }
};

TEST_F(StringifyTest, MoreContainers) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructMoreContainers>);
  EXPECT_THAT(
      Stringify().ToString(TestStructMoreContainers{}),
      R"({"one": [1, 2], "two": [{.first: 1, .second: 2}, {.first: 3, .second: 4}], "three": [{.Key: 5, .Val: 6}]})")
      << "  NOTE: Here we are not providing the defualt Json options down do the pairs. However, in `three` we have "
         "the provided key/value names.";
  EXPECT_THAT(
      Stringify::AsJson().ToString(TestStructMoreContainers{}),
      R"({"one": [1, 2], "two": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "three": [{"Key": 5, "Val": 6}]})");
  EXPECT_THAT(
      Stringify::AsJsonPretty().ToString(TestStructMoreContainers{}),
      EqualsText(
          R"({"one": [1, 2], "two": [{"first": 1,"second": 2}, {"first": 3,"second": 4}], "three": [{"Key": 5,"Val": 6}]
}
)")) << "The new line comes from the outer default options.";
}

struct TestStructMoreContainersWithDirectFieldNames {
  std::set<int> one = {1, 2};
  std::map<int, int> two = {{1, 2}, {3, 4}};
  std::vector<std::pair<int, int>> three = {{5, 6}};

  friend auto MboTypesStringifyFieldNames(const TestStructMoreContainersWithDirectFieldNames&) {
    return std::array<std::string_view, 3>{"1", "2", "3"};
  }

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructMoreContainersWithDirectFieldNames&,
      std::size_t idx,
      std::string_view,
      const StringifyOptions& defaults) {
    StringifyOptions ret = defaults;
    if (idx == 2) {
      ret.special_pair_first = "Key";
      ret.special_pair_second = "Val";
    }
    return ret;
  }
};

TEST_F(StringifyTest, MoreContainersWithDirectFieldNames) {
  static_assert(!HasMboTypesStringifyDoNotPrintFieldNames<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(HasMboTypesStringifyOptions<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(HasMboTypesStringifyFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_TRUE(HasMboTypesStringifyFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_THAT(MboTypesStringifyFieldNames(TestStructMoreContainersWithDirectFieldNames{}), ElementsAre("1", "2", "3"));
  EXPECT_THAT(
      Stringify(StringifyOptions::AsJson()).ToString(TestStructMoreContainersWithDirectFieldNames{}),
      R"({"1": [1, 2], "2": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "3": [{"Key": 5, "Val": 6}]})");
}

struct TestStructContainersOfPairs {
  std::map<std::string_view, int> one = {{"a", 1}, {"b", 2}};
  std::vector<std::pair<std::string_view, int>> two = {{"c", 3}, {"d", 4}};

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructContainersOfPairs& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(StringifyOptions::AsJson(), {"one", "two", "three", "four"})(v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, ContainersOfPairs) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructContainersOfPairs>);
  EXPECT_THAT(
      Stringify().ToString(TestStructContainersOfPairs{}), R"({"one": {"a": 1, "b": 2}, "two": {"c": 3, "d": 4}})");
}

TEST_F(StringifyTest, PrintWithControl) {
  struct TestStruct {
    int one = 25;
  };

  const TestStruct v;
  if constexpr (kStructNameSupport) {
    EXPECT_THAT(Stringify(StringifyOptions::AsCpp()).ToString(v), R"({.one = 25})");
    EXPECT_THAT(Stringify(StringifyOptions::AsJson()).ToString(v), R"({"one": 25})");
    EXPECT_THAT(Stringify::AsJsonPretty().ToString(v), EqualsText(R"({
  "one": 25
}
)"));
  } else {
    EXPECT_THAT(Stringify(StringifyOptions::AsCpp()).ToString(v), R"({25})");
    EXPECT_THAT(Stringify(StringifyOptions::AsJson()).ToString(v), R"({"0": 25})");
    EXPECT_THAT(Stringify::AsJsonPretty().ToString(v), EqualsText(R"({
  "0": 25
}
)"));
  }
}

TEST_F(StringifyTest, NestedDefaults) {
  struct TestSub {
    int four = 42;
  };

  struct TestStruct {
    int one = 11;
    int two = 25;
    TestSub three = {.four = 42};
  };

  const TestStruct v;

  if constexpr (kStructNameSupport) {
    static constexpr std::string_view kExpectedDef = R"({.one: 11, .two: 25, .three: {.four: 42}})";
    static constexpr std::string_view kExpectedCpp = R"({.one = 11, .two = 25, .three = {.four = 42}})";
    static constexpr std::string_view kExpectedJson = R"({"one": 11, "two": 25, "three": {"four": 42}})";
    EXPECT_THAT(Stringify().ToString(v), kExpectedDef);
    EXPECT_THAT(Stringify(StringifyOptions::AsCpp()).ToString(v), kExpectedCpp);
    EXPECT_THAT(Stringify(StringifyOptions::AsJson()).ToString(v), kExpectedJson);
    EXPECT_THAT(Stringify::AsJson().ToString(v), kExpectedJson);
    EXPECT_THAT(Stringify::AsJsonPretty().ToString(v), EqualsText(R"({
  "one": 11,
  "two": 25,
  "three": {
    "four": 42
  }
}
)"));
  } else {
    EXPECT_THAT(Stringify::AsJsonPretty().ToString(v), EqualsText(R"({
  "0": 11,
  "1": 25,
  "2": {
    "0": 42
  }
}
)"));
  }
}

TEST_F(StringifyTest, NestedJsonNumericFallback) {
  struct TestSub {
    int four = 42;

    using MboTypesStringifyDoNotPrintFieldNames = void;
  };

  struct TestStruct {
    int one = 11;
    int two = 25;
    TestSub three = {.four = 42};

    using MboTypesStringifyDoNotPrintFieldNames = void;
  };

  const TestStruct v{};
  static constexpr std::string_view kExpectedCpp = R"({11, 25, {42}})";
  static constexpr std::string_view kExpectedJson = R"({"0": 11, "1": 25, "2": {"0": 42}})";
  EXPECT_THAT(Stringify(StringifyOptions::AsCpp()).ToString(v), kExpectedCpp);
  EXPECT_THAT(Stringify::AsCpp().ToString(v), kExpectedCpp);
  EXPECT_THAT(Stringify(StringifyOptions::AsJson()).ToString(v), kExpectedJson);
  EXPECT_THAT(Stringify::AsJson().ToString(v), kExpectedJson);
}

#if !defined(__clang__)  // TODO(helly25): Clang takes aim at this.
struct TestStructCustomNestedJsonNested {
  int first = 0;
  std::string second = "nested";

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructCustomNestedJsonNested& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(
        StringifyOptions::AsJson(), {"NESTED_1", "NESTED_2"}, StringifyNameHandling::kOverwrite)(
        v, idx, name, defaults);
  }
};

struct TestStructCustomNestedJson {
  int one = 123;
  std::string two = "test";
  std::array<bool, 2> three = {false, true};
  std::vector<TestStructCustomNestedJsonNested> four = {{.first = 25, .second = "foo"}, {.first = 42, .second = "bar"}};
  std::pair<int, int> five = {25, 42};

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructCustomNestedJson& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(StringifyOptions::AsJson(), {"one", "two", "three", "four", "five"})(
        v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, CustomNestedJson) {
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructCustomNestedJson>);

  // Keys for nested "four" should get their key names as "first" and "second" since they are not provided.
  // No handover of concrete values to defaults should occur.
  // BUT: Five uses non JSON mode as we fallback to the default options which were not set.
  EXPECT_THAT(
      Stringify().ToString(TestStructCustomNestedJson{}),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {.first: 25, .second: 42}})");

  EXPECT_THAT(
      Stringify::AsJson().ToString(TestStructCustomNestedJson{}),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {"first": 25, "second": 42}})");
}
#endif
struct TestStructNonLiteralFields {
  std::map<int, int> one = {{1, 2}, {2, 3}};
  std::unordered_map<int, int> two = {{3, 4}};
  std::string three = "three";

  friend StringifyOptions MboTypesStringifyOptions(
      const TestStructNonLiteralFields& v,
      std::size_t idx,
      std::string_view name,
      const StringifyOptions& defaults) {
    return StringifyWithFieldNames(defaults, {"one", "two", "three"}, StringifyNameHandling::kVerify)(
        v, idx, name, defaults);
  }
};

TEST_F(StringifyTest, NonLiteralFields) {
  ASSERT_THAT(SupportsFieldNames<TestStructNonLiteralFields>, kStructNameSupport);
  ASSERT_FALSE(SupportsFieldNamesConstexpr<TestStructNonLiteralFields>);
  ASSERT_FALSE(HasMboTypesStringifyDoNotPrintFieldNames<TestStructNonLiteralFields>);
  ASSERT_FALSE(HasMboTypesStringifyFieldNames<TestStructNonLiteralFields>);
  ASSERT_TRUE(HasMboTypesStringifyOptions<TestStructNonLiteralFields>);

  EXPECT_THAT(
      Stringify::AsCpp().ToString(TestStructNonLiteralFields{}),
      R"({.one = {{.first = 1, .second = 2}, {.first = 2, .second = 3}}, .two = {{.first = 3, .second = 4}}, .three = "three"})");
  EXPECT_THAT(Stringify::AsJsonPretty().ToString(TestStructNonLiteralFields{}), EqualsText(R"({
  "one": [
    {
      "first": 1,
      "second": 2
    },
    {
      "first": 2,
      "second": 3
    }
  ],
  "two": [
    {
      "first": 3,
      "second": 4
    }
  ],
  "three": "three"
}
)"));
}

struct TestMboTypesStringifyConvert {
  using MboTypesStringifyDoNotPrintFieldNames = void;

  int value = 25;
  int other = 42;

  bool complex = false;

  static std::string MboTypesStringifyConvert(std::size_t idx, const TestMboTypesStringifyConvert&, const int& value) {
    return absl::StrFormat("value@%d:%d", idx, value);
  }

  static std::pair<std::string, std::string> MboTypesStringifyConvert(
      std::size_t field_index,
      const TestMboTypesStringifyConvert&,
      const bool& value) {
    if (value) {
      return {"two", absl::StrCat(field_index)};
    } else {
      return {"one", absl::StrCat(field_index)};
    }
  }
};

TEST_F(StringifyTest, MboTypesStringifyConvert) {
  TestMboTypesStringifyConvert data;
  EXPECT_THAT(Stringify::AsCpp().ToString(data), R"({"value@0:25", "value@1:42", {"one", "2"}})");
  data.value += 17;
  data.other -= 9;
  data.complex = !data.complex;
  EXPECT_THAT(Stringify::AsCpp().ToString(data), R"({"value@0:42", "value@1:33", {"two", "2"}})");
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace
}  // namespace mbo_other

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif  // defined(__clang__)
