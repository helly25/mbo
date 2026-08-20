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

#include "mbo/json/json.h"

#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/matchers.h"
#include "mbo/types/typed_view.h"

// NOLINTBEGIN(*-magic-*,*-avoid-unchecked-container-access)
// The subscript operators ARE the API under test here: Json::at() delegates to
// operator[], so rewriting these calls would only stop testing what they test.
// Neither overload can go out of range - the property one inserts, the index one
// bounds-checks via MBO_CONFIG_REQUIRE.

namespace mbo::json {
namespace {

using ::mbo::testing::EqualsText;
using ::mbo::testing::IsNullopt;
using ::mbo::types::TypedView;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pair;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAreArray;

struct JsonTest : ::testing::Test {};

TEST_F(JsonTest, Test) {
  EXPECT_THAT(Json{}, IsNullopt());
  EXPECT_THAT(Json{}.Serialize(), EqualsText(R"({}
)"));
}

TEST_F(JsonTest, Comparison) {
  static_assert(std::three_way_comparable_with<Json, Json>);
  static_assert(types::ThreeWayComparableTo<Json, int>);
  static_assert(types::ThreeWayComparableTo<Json, unsigned>);
  EXPECT_THAT(Json{}, std::nullopt);
  EXPECT_THAT(Json{}, IsNullopt());
  EXPECT_THAT(Json{1}, Not(std::nullopt));
  EXPECT_THAT(Json{1}, Not(IsNullopt()));
  EXPECT_THAT(Json{2}, 2);
  EXPECT_THAT(Json{"yes"}, "yes");
  EXPECT_THAT(Json{3}, Not("nope"));
}

TEST_F(JsonTest, ScalarComparisonsCoverSupportedRepresentations) {
  EXPECT_EQ(Json{false}, false);
  EXPECT_EQ(Json{true}, true);
  EXPECT_EQ(Json{std::int8_t{-8}}, std::int8_t{-8});
  EXPECT_EQ(Json{std::int16_t{-16}}, std::int16_t{-16});
  EXPECT_EQ(Json{std::int32_t{-32}}, std::int32_t{-32});
  EXPECT_EQ(Json{std::int64_t{-64}}, std::int64_t{-64});
  EXPECT_EQ(Json{std::uint8_t{8}}, std::uint8_t{8});
  EXPECT_EQ(Json{std::uint16_t{16}}, std::uint16_t{16});
  EXPECT_EQ(Json{std::uint32_t{32}}, std::uint32_t{32});
  EXPECT_EQ(Json{std::uint64_t{64}}, std::uint64_t{64});
  EXPECT_EQ(Json{1.25F}, 1.25F);
  EXPECT_EQ(Json{2.5}, 2.5);
  EXPECT_EQ(Json{"text"}, "text");
  EXPECT_EQ(Json{std::string{"text"}}, std::string{"text"});
  EXPECT_EQ(Json{std::string_view{"text"}}, std::string_view{"text"});

  EXPECT_LT(Json{-1}, 0U);
  EXPECT_LT(Json{0U}, 1.0);
  EXPECT_LT(Json{1.0}, 2);
  EXPECT_GT(Json{std::numeric_limits<std::uint64_t>::max()}, std::numeric_limits<std::int64_t>::max());
}

TEST_F(JsonTest, JsonComparisonsCoverEveryStoredKind) {
  Json array_lhs;
  array_lhs.MakeArray();
  array_lhs.emplace_back(1);
  Json array_rhs;
  array_rhs.MakeArray();
  array_rhs.emplace_back(2);

  Json object_lhs;
  object_lhs["value"] = 1;
  Json object_rhs;
  object_rhs["value"] = 2;

  EXPECT_EQ(Json{} <=> Json{}, std::strong_ordering::equal);
  EXPECT_LT(Json{false}, Json{true});
  EXPECT_LT(Json{-1}, Json{1U});
  EXPECT_LT(Json{1U}, Json{2.0});
  EXPECT_LT(Json{"a"}, Json{"b"});
  EXPECT_LT(array_lhs, array_rhs);
  EXPECT_LT(object_lhs, object_rhs);
  EXPECT_LT(Json{}, array_lhs);
  EXPECT_LT(array_lhs, Json{false});
  EXPECT_LT(Json{false}, Json{0});
  EXPECT_LT(Json{0}, object_lhs);
  EXPECT_LT(object_lhs, Json{"value"});
}

TEST_F(JsonTest, CopyOperationsDeepCopyArraysAndObjects) {
  Json original;
  original["array"].emplace_back(1);
  original["nested"]["value"] = "before";

  Json copy(original);
  copy["array"][0] = 2;
  copy["nested"]["value"] = "after";

  EXPECT_EQ(original["array"][0], 1);
  EXPECT_EQ(original["nested"]["value"], "before");
  EXPECT_EQ(copy["array"][0], 2);
  EXPECT_EQ(copy["nested"]["value"], "after");

  Json assigned;
  assigned = original;
  assigned["array"][0] = 3;
  const Json* same = &assigned;
  assigned = *same;
  EXPECT_EQ(original["array"][0], 1);
  EXPECT_EQ(assigned["array"][0], 3);

  const std::unique_ptr<Json> empty;
  assigned = empty;
  EXPECT_TRUE(assigned.IsNull());
}

TEST_F(JsonTest, ContainerModifiersAndAccessors) {
  Json array;
  array.MakeArray();
  array.resize(2, 7);
  EXPECT_EQ(array.size(), 2);
  EXPECT_EQ(array.at(0), 7);
  EXPECT_EQ(array.at(1), 7);
  array.pop_back();
  array.push_back(8);
  array.emplace_back(9);
  EXPECT_THAT(array, ElementsAre(7, 8, 9));
  array.erase(array.begin() + 1);
  EXPECT_THAT(array, ElementsAre(7, 9));
  array.erase(array.begin(), array.end());
  EXPECT_THAT(array, IsEmpty());
  array.resize(2);
  array.clear();
  EXPECT_THAT(array, IsEmpty());

  Json object;
  object.emplace("one", 1);
  object.emplace("two", 2);
  EXPECT_TRUE(object.contains("one"));
  EXPECT_EQ(object.at("one"), 1);
  EXPECT_EQ(object.erase("missing"), 0);
  EXPECT_EQ(object.erase("one"), 1);
  EXPECT_FALSE(object.contains("one"));
  EXPECT_EQ(object.erase("two"), 1);
  EXPECT_TRUE(object.empty());

  Json scalar{1};
  EXPECT_TRUE(scalar);
  EXPECT_TRUE(scalar.empty());
  EXPECT_EQ(scalar.size(), 0);
  EXPECT_EQ(scalar.erase("none"), 0);
  scalar.clear();
  EXPECT_TRUE(scalar.IsNull());
}

TEST_F(JsonTest, BasicsAndSerialize) {
  Json data;
  data["foo"] = "bar";
  data["bar"] = "baz";
  ASSERT_THAT(data["bar"], IsEmpty());
  EXPECT_THAT(
      data.Serialize(),  //
      AnyOf(
          EqualsText(R"({"bar":"baz","foo":"bar"}
)"),
          EqualsText(R"({"foo":"bar","bar":"baz"}
)")));
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "bar": "baz",
  "foo": "bar"
}
)"));
  data["null"] = std::nullopt;
  ASSERT_THAT(data["null"].IsNull(), true);
  ASSERT_THAT(data["null"], IsEmpty());
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "bar": "baz",
  "foo": "bar",
  "null": null
}
)"));
  data["null"].MakeObject();
  ASSERT_THAT(data["null"].IsObject(), true);
  ASSERT_THAT(data["null"], IsEmpty());
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "bar": "baz",
  "foo": "bar",
  "null": {
  }
}
)"));
  data["null"].Reset();
  ASSERT_THAT(data["null"].IsNull(), true);
  ASSERT_THAT(data["null"], IsEmpty());
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "bar": "baz",
  "foo": "bar",
  "null": null
}
)"));
  data["array"].MakeArray();
  ASSERT_THAT(data["array"].IsArray(), true);
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "array": [
  ],
  "bar": "baz",
  "foo": "bar",
  "null": null
}
)"));
  data["array"].emplace_back(25);
  data["array"].emplace_back("42");
  ASSERT_THAT(data["array"], SizeIs(2));
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "array": [
    25,
    "42"
  ],
  "bar": "baz",
  "foo": "bar",
  "null": null
}
)"));
  data["object"].MakeObject();
  ASSERT_THAT(data["object"].IsObject(), true);
  ASSERT_THAT(data["object"], IsEmpty());
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "array": [
    25,
    "42"
  ],
  "bar": "baz",
  "foo": "bar",
  "null": null,
  "object": {
  }
}
)"));
  data["object"]["one"] = 33;
  data["object"]["two"].MakeString("Two");
  ASSERT_THAT(data["object"], SizeIs(2));
  EXPECT_THAT(data.Serialize(Json::SerializeMode::kPretty), EqualsText(R"({
  "array": [
    25,
    "42"
  ],
  "bar": "baz",
  "foo": "bar",
  "null": null,
  "object": {
    "one": 33,
    "two": "Two"
  }
}
)"));
}

TEST_F(JsonTest, ArrayIteration) {
  Json json;
  json.MakeArray();
  json.push_back(0);
  json.push_back("hello");
  json.emplace_back("world");
  json.emplace_back(std::nullopt);
  json.emplace_back(true);
  json.emplace_back(false);
  const std::initializer_list<Json> expected = {Json{0},    Json{"hello"}, Json{"world"}, Json{std::nullopt},
                                                Json{true}, Json{false}};
  const auto elements_are = ElementsAreArray<Json>(expected);
  EXPECT_THAT(TypedView(json.array_values()), elements_are);
  EXPECT_THAT(TypedView(json.array_values()), ElementsAre(0, "hello", "world", std::nullopt, true, false));
  EXPECT_THAT(json, elements_are);
  EXPECT_THAT(std::vector<Json>(json.begin(), json.end()), elements_are);
  const auto values = json.values();
  EXPECT_THAT(values, Not(IsEmpty()));
  EXPECT_THAT(values.begin(), Not(values.end()));
  EXPECT_THAT(json.values(), elements_are);
}

TEST_F(JsonTest, PropertyIteration) {
  Json json;
  json["a"] = 1;
  json["b"] = 2;
  json["c"] = 3;
  EXPECT_THAT(TypedView(json.property_names()), UnorderedElementsAreArray({"a", "b", "c"}));
  EXPECT_THAT(TypedView(json.property_pairs()), UnorderedElementsAreArray({Pair("a", 1), Pair("b", 2), Pair("c", 3)}));
  EXPECT_THAT(TypedView(json.property_values()), UnorderedElementsAreArray({1, 2, 3}));
  // EXPECT_THAT(json.values(), UnorderedElementsAreArray({1, 2, 3}));
}

TEST_F(JsonTest, ValueIteratorAssignment) {
  Json array;
  array.emplace_back(1);
  array.emplace_back(2);
  auto array_values = array.values();

  Json::value_iterator mutable_it;
  mutable_it = array_values.begin();
  EXPECT_THAT(*mutable_it, 1);
  const auto* mutable_it_ptr = &mutable_it;
  mutable_it = *mutable_it_ptr;
  EXPECT_THAT(*mutable_it, 1);

  Json::value_iterator copied_it;
  copied_it = mutable_it;
  EXPECT_THAT(*copied_it, 1);
  Json::value_iterator moved_it;
  moved_it = Json::value_iterator{array_values.begin()};
  EXPECT_THAT(*moved_it, 1);

  Json::const_value_iterator const_it;
  const_it = mutable_it;
  EXPECT_THAT(*const_it, 1);
  Json::const_value_iterator moved_const_it;
  moved_const_it = array.values().begin();
  EXPECT_THAT(*moved_const_it, 1);

  Json object;
  object["value"] = 3;
  auto object_values = object.values();
  moved_it = object_values.begin();
  EXPECT_THAT(*moved_it, 3);
  const_it = moved_it;
  EXPECT_THAT(*const_it, 3);
}

}  // namespace
}  // namespace mbo::json

// NOLINTEND(*-magic-*,*-avoid-unchecked-container-access)
