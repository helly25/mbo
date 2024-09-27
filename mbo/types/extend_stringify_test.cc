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
#include "mbo/container/limited_vector.h"
#include "mbo/types/extend.h"
#include "mbo/types/extender.h"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wunused-local-typedef"
# // There is an issue with ADL members here.
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
# pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

// Not using namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::types::AbslStringifyNameHandling;
using ::mbo::types::AbslStringifyOptions;
using ::mbo::types::Extend;
using ::mbo::types::extender::WithFieldNames;
using ::mbo::types::HasMboTypesExtendDoNotPrintFieldNames;
using ::mbo::types::HasMboTypesExtendFieldNames;
using ::mbo::types::HasMboTypesExtendStringifyOptions;
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

struct ExtenderStringifyTest : ::testing::Test {
  struct Tester {
    MOCK_METHOD(std::vector<std::string>, FieldNames, ());
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

TEST_F(ExtenderStringifyTest, AllExtensionApiPointsAbsent) {
  struct TestStruct : Extend<TestStruct> {
    int one = 11;
  };

  EXPECT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<TestStruct>);
  EXPECT_FALSE(HasMboTypesExtendFieldNames<TestStruct>);
  EXPECT_FALSE(HasMboTypesExtendStringifyOptions<TestStruct>);
}

TEST_F(ExtenderStringifyTest, SuppressFieldNames) {
  struct SuppressFieldNames : ::Extend<SuppressFieldNames> {
    using MboTypesExtendDoNotPrintFieldNames = void;

    int one = 25;
    std::string_view two = "42";
  };

  ASSERT_TRUE(HasMboTypesExtendDoNotPrintFieldNames<SuppressFieldNames>);
  EXPECT_FALSE(HasMboTypesExtendFieldNames<SuppressFieldNames>);
  EXPECT_FALSE(HasMboTypesExtendStringifyOptions<SuppressFieldNames>);
  EXPECT_THAT(SuppressFieldNames{}.ToString(), R"({25, "42"})")
      << "  NOTE: Here no compiler should print any field name.";
}

struct AddFieldNames : ::Extend<AddFieldNames> {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesExtendFieldNames(const AddFieldNames&) { return std::array<std::string_view, 2>{"1", "2"}; }
};

TEST_F(ExtenderStringifyTest, AddFieldNames) {
  // Proves the straight forward type: a `std::array<std::string, N>` works.
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<AddFieldNames>);
  ASSERT_TRUE(HasMboTypesExtendFieldNames<AddFieldNames>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<AddFieldNames>);
  EXPECT_THAT(AddFieldNames{}.ToString(), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
}

struct AddFieldVectorOfString : ::Extend<AddFieldVectorOfString> {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesExtendFieldNames(const AddFieldVectorOfString&) { return std::vector<std::string>{"1", "2"}; }
};

TEST_F(ExtenderStringifyTest, AddFieldVectorOfString) {
  // This test proves that conversion from temp strings works through lifetime extension.
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<AddFieldVectorOfString>);
  ASSERT_TRUE(HasMboTypesExtendFieldNames<AddFieldVectorOfString>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<AddFieldVectorOfString>);
  EXPECT_THAT(AddFieldVectorOfString{}.ToString(), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
}

struct AddFieldNamesLimitedVector : ::Extend<AddFieldNamesLimitedVector> {
  int one = 25;
  std::string_view two = "42";

  friend auto MboTypesExtendFieldNames(const AddFieldNamesLimitedVector&) {
    return mbo::container::MakeLimitedVector("1", "2");
  }
};

TEST_F(ExtenderStringifyTest, AddFieldNamesLimitedVector) {
  // Proves other types can be compatible.
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<AddFieldNamesLimitedVector>);
  ASSERT_TRUE(HasMboTypesExtendFieldNames<AddFieldNamesLimitedVector>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<AddFieldNamesLimitedVector>);
  EXPECT_THAT(AddFieldNamesLimitedVector{}.ToString(), R"({.1: 25, .2: "42"})")
      << "  NOTE: Here we inject the field names and override any possibly compiler provided names.";
}

struct TestStructFieldOptions : Extend<TestStructFieldOptions> {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructFieldOptions&,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& /*defaults*/) {
    return ExtenderStringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(ExtenderStringifyTest, FieldOptions) {
  // Demonstrates different parameters can be routed differently.
  // The test verifies that:
  // * based on field index (or name if available) different control can be returned.
  // * fields `one` and `two` have different key control, including field name overriding.
  // * field `tre` will be fully suppressed.
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<TestStructFieldOptions>);
  ASSERT_FALSE(HasMboTypesExtendFieldNames<TestStructFieldOptions>);
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructFieldOptions>);

  EXPECT_CALL(*tester, FieldOptions(0, HasFieldName("one")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "+1+",
          .key_prefix = "_1_",
          .key_suffix = ".1.",
          .key_value_separator = "=1=",
          .key_use_name = "first",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, HasFieldName("two")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "+2+",
          .key_prefix = "_2_",
          .key_suffix = ".2.",
          .key_value_separator = "=2=",
          .key_use_name = "second",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, HasFieldName("tre")))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = true,
      }));

  EXPECT_THAT(TestStructFieldOptions{}.ToString(), "{_1_first.1.=1=11+2+_2_second.2.=2=25}");
}

struct TestStructFieldNames : Extend<TestStructFieldNames> {
  int one = 11;
  int two = 25;
  int tre = 33;

  friend std::vector<std::string> MboTypesExtendFieldNames(const TestStructFieldNames&) {
    return ExtenderStringifyTest::tester->FieldNames();
  }

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructFieldNames&,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& /*defaults*/) {
    return ExtenderStringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(ExtenderStringifyTest, FieldNames) {
  // Demonstrates different parameters can be routed differently.
  // Unlike test `FieldOptions` here we first fetch the field names which override compiler provided ones.
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<TestStructFieldNames>);
  ASSERT_TRUE(HasMboTypesExtendFieldNames<TestStructFieldNames>);
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructFieldNames>);

  // 1st ToString call.
  EXPECT_CALL(*tester, FieldNames()).WillOnce(::testing::Return(std::vector<std::string>{"First", "Second", "Third"}));
  EXPECT_CALL(*tester, FieldOptions(0, "First"))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_separator = "+1+",
          .key_prefix = "_1_",
          .key_suffix = ".1.",
          .key_value_separator = "=1=",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, "Second"))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = true,
          .field_separator = "+2+",
          .key_prefix = "_2_",
          .key_suffix = ".2.",
          .key_value_separator = "=2=",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, "Third"))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_separator = "+3+",
          .key_prefix = "_3_",
          .key_suffix = ".3.",
          .key_value_separator = "=3=",
      }));

  EXPECT_THAT(TestStructFieldNames{}.ToString(), "{_1_First.1.=1=11+3+_3_Third.3.=3=33}");

  // 2nd ToString call.
  EXPECT_CALL(*tester, FieldNames()).WillOnce(::testing::Return(std::vector<std::string>{"Fourth"}));
  EXPECT_CALL(*tester, FieldOptions(0, "Fourth"))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_separator = "+4+",
          .key_prefix = "_4_",
          .key_suffix = ".4.",
          .key_value_separator = "=4=",
      }));
  // 2nd and 3rd field get printed. But 2nd has no key name, so related options get ignored.
  EXPECT_CALL(*tester, FieldOptions(1, IsEmpty()))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_separator = "+5+",
          .key_prefix = "_5_",
          .key_suffix = ".5.",
          .key_value_separator = "=5=",
      }));
  // For the 3rd field, the options provide the field name through `key_use_name`.
  EXPECT_CALL(*tester, FieldOptions(2, IsEmpty()))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_separator = "+6+",
          .key_prefix = "_6_",
          .key_suffix = ".6.",
          .key_value_separator = "=6=",
          .key_use_name = "Sixth",
      }));
  EXPECT_THAT(TestStructFieldNames{}.ToString(), "{_4_Fourth.4.=4=11+5+25+6+_6_Sixth.6.=6=33}");
}

struct TestStructDoNotPrintFieldNames : Extend<TestStructDoNotPrintFieldNames> {
  int one = 11;
  int two = 25;
  int tre = 33;

  using MboTypesExtendDoNotPrintFieldNames = void;

  // It is not allowed to also have the API extension point `MboTypesExtendFieldNames`.

  friend AbslStringifyOptions MboTypesExtendStringifyOptions(
      const TestStructDoNotPrintFieldNames&,
      std::size_t idx,
      std::string_view name,
      const AbslStringifyOptions& /*defaults*/) {
    return ExtenderStringifyTest::tester->FieldOptions(idx, name);
  }
};

TEST_F(ExtenderStringifyTest, DoNotPrintFieldNames) {
  // Demonstrates different parameters can be routed differently.
  // Unlike test `FieldOptions` here we first fetch the field names which override compiler provided ones.
  ASSERT_TRUE(HasMboTypesExtendDoNotPrintFieldNames<TestStructDoNotPrintFieldNames>);
  ASSERT_FALSE(HasMboTypesExtendFieldNames<TestStructDoNotPrintFieldNames>);
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructDoNotPrintFieldNames>);

  EXPECT_CALL(*tester, FieldOptions(0, IsEmpty()))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
      }));
  EXPECT_CALL(*tester, FieldOptions(1, IsEmpty()))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = false,
          .field_separator = "++",
          .key_prefix = "__",
          .key_suffix = "..",
          .key_value_separator = "==",
      }));
  EXPECT_CALL(*tester, FieldOptions(2, IsEmpty()))
      .WillOnce(::testing::Return(AbslStringifyOptions{
          .field_suppress = true,
      }));

  EXPECT_THAT(TestStructDoNotPrintFieldNames{}.ToString(), "{11++25}");
}

struct TestStructShorten : Extend<TestStructShorten> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructShorten>);
  EXPECT_THAT(TestStructShorten{}.ToString(), R"({one = "1", two = "2...", three = "3**", four = "**", five = ""})");
}

struct TestStructValueReplacement : Extend<TestStructValueReplacement> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructValueReplacement>);
  EXPECT_THAT(
      TestStructValueReplacement{}.ToString(),
      R"({one = <YY>, two = "<XX>", three = {<YY>, <YY>, <YY>}, four = {"<XX>", "<XX>", "<XX>"}})");
}

struct TestStructContainer : Extend<TestStructContainer> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructContainer>);
  EXPECT_THAT(TestStructContainer{}.ToString(), R"({one = [1, 2], two = [], three = [1, 2]})");
}

struct TestStructMoreTypes : Extend<TestStructMoreTypes> {
  float one = 1.1;
  double two = 2.2;
  unsigned three = 3;
  char four = '4';
  unsigned char five = '5';

  friend auto MboTypesExtendFieldNames(const TestStructMoreTypes&) {
    return std::array<std::string_view, 5>{"one", "two", "three", "four", "five"};
  }
};

TEST_F(ExtenderStringifyTest, MoreTypes) {
  ASSERT_TRUE(HasMboTypesExtendFieldNames<TestStructMoreTypes>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<TestStructMoreTypes>);
  EXPECT_THAT(TestStructMoreTypes{}.ToString(), R"({.one: 1.1, .two: 2.2, .three: 3, .four: '4', .five: '5'})");
}

struct TestStructMoreContainers : Extend<TestStructMoreContainers> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructMoreContainers>);
  EXPECT_THAT(
      TestStructMoreContainers{}.ToString(),
      R"({"one": [1, 2], "two": [{.first: 1, .second: 2}, {.first: 3, .second: 4}], "three": [{.Key: 5, .Val: 6}]})")
      << "  NOTE: Here we are not providing the defualt Json options down do the pairs. However, in `three` we have "
         "the provided key/value names.";
  EXPECT_THAT(
      TestStructMoreContainers{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"one": [1, 2], "two": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "three": [{"Key": 5, "Val": 6}]})");
}

struct TestStructMoreContainersWithDirectFieldNames : Extend<TestStructMoreContainersWithDirectFieldNames> {
  std::set<int> one = {1, 2};
  std::map<int, int> two = {{1, 2}, {3, 4}};
  std::vector<std::pair<int, int>> three = {{5, 6}};

  friend auto MboTypesExtendFieldNames(const TestStructMoreContainersWithDirectFieldNames&) {
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
  static_assert(!HasMboTypesExtendDoNotPrintFieldNames<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(HasMboTypesExtendStringifyOptions<TestStructMoreContainersWithDirectFieldNames>);
  static_assert(HasMboTypesExtendFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_TRUE(HasMboTypesExtendFieldNames<decltype(TestStructMoreContainersWithDirectFieldNames{})>);
  EXPECT_THAT(MboTypesExtendFieldNames(TestStructMoreContainersWithDirectFieldNames{}), ElementsAre("1", "2", "3"));
  EXPECT_THAT(
      TestStructMoreContainersWithDirectFieldNames{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"1": [1, 2], "2": [{"first": 1, "second": 2}, {"first": 3, "second": 4}], "3": [{"Key": 5, "Val": 6}]})");
}

struct TestStructContainersOfPairs : Extend<TestStructContainersOfPairs> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructContainersOfPairs>);
  EXPECT_THAT(TestStructContainersOfPairs{}.ToString(), R"({"one": {"a": 1, "b": 2}, "two": {"c": 3, "d": 4}})");
}

TEST_F(ExtenderStringifyTest, PrintWithControl) {
  struct TestStruct : Extend<TestStruct> {
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
  struct TestSub : Extend<TestSub> {
    int four = 42;
  };

  struct TestStruct : Extend<TestStruct> {
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

struct TestStructCustomNestedJsonNested : Extend<TestStructCustomNestedJsonNested> {
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

struct TestStructCustomNestedJson : Extend<TestStructCustomNestedJson> {
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
  ASSERT_TRUE(HasMboTypesExtendStringifyOptions<TestStructCustomNestedJson>);

  // Keys for nested "four" should get their key names as "first" and "second" since they are not provided.
  // No handover of concrete values to defaults should occur.
  // BUT: Five uses non JSON mode as we fallback to the default options which were not set.
  EXPECT_THAT(
      TestStructCustomNestedJson{}.ToString(),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {.first: 25, .second: 42}})");

  EXPECT_THAT(
      TestStructCustomNestedJson{}.ToString(AbslStringifyOptions::AsJson()),
      R"({"one": 123, "two": "test", "three": [false, true], "four": [{"NESTED_1": 25, "NESTED_2": "foo"}, {"NESTED_1": 42, "NESTED_2": "bar"}], "five": {"first": 25, "second": 42}})");
}

struct TestStructNonLiteralFields : mbo::types::Extend<TestStructNonLiteralFields> {
  std::map<int, int> one = {{1, 2}, {2, 3}};
  std::unordered_map<int, int> two = {{3, 4}};
  std::string three = "three";

  friend auto MboTypesExtendFieldNames(const TestStructNonLiteralFields&) {
    return std::array<std::string_view, 3>{"one", "two", "three"};
  }
};

TEST_F(ExtenderStringifyTest, NonLiteralFields) {
  ASSERT_TRUE(SupportsFieldNames<TestStructNonLiteralFields>);
  ASSERT_FALSE(SupportsFieldNamesConstexpr<TestStructNonLiteralFields>);
  ASSERT_FALSE(HasMboTypesExtendDoNotPrintFieldNames<TestStructNonLiteralFields>);
  ASSERT_TRUE(HasMboTypesExtendFieldNames<TestStructNonLiteralFields>);
  ASSERT_FALSE(HasMboTypesExtendStringifyOptions<TestStructNonLiteralFields>);

  EXPECT_THAT(
      TestStructNonLiteralFields{}.ToString(AbslStringifyOptions::AsCpp()),
      R"({.one = {{.first = 1, .second = 2}, {.first = 2, .second = 3}}, .two = {{.first = 3, .second = 4}}, .three = "three"})");
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace

// Not using namespace mbo::types

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
