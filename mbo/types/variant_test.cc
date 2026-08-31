// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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

#include "mbo/types/variant.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-magic-numbers)

struct VariantTest : ::testing::Test {};

TEST_F(VariantTest, IsVariantIdentifiesVariants) {
  static_assert(IsVariant<std::variant<int>>);
  static_assert(IsVariant<std::variant<int, std::string>>);
  static_assert(IsVariant<std::variant<std::monostate, int>>);
  static_assert(!IsVariant<int>);
  static_assert(!IsVariant<std::vector<int>>);
  // Reference and const qualified types are NOT variants: the concept matches the
  // type exactly, so a caller has to decay before asking.
  static_assert(!IsVariant<std::variant<int>&>);
}

TEST_F(VariantTest, IsVariantMemberTypeFindsAlternatives) {
  using Var = std::variant<int, std::string, double>;
  static_assert(IsVariantMemberType<Var, int>);
  static_assert(IsVariantMemberType<Var, std::string>);
  static_assert(IsVariantMemberType<Var, double>);
  // Not an alternative, even though it converts to one.
  static_assert(!IsVariantMemberType<Var, std::string_view>);
  static_assert(!IsVariantMemberType<Var, float>);
  static_assert(!IsVariantMemberType<Var, char>);
}

TEST_F(VariantTest, IsVariantMemberTypeHandlesTheFirstAndLastAlternative) {
  // The implementation walks alternatives by index, so both ends are worth pinning.
  using Var = std::variant<int, double, std::string>;
  static_assert(IsVariantMemberType<Var, int>);                         // first alternative
  static_assert(IsVariantMemberType<Var, std::string>);                 // last alternative
  static_assert(!IsVariantMemberType<std::variant<int>, std::string>);  // single alternative, no match
}

TEST_F(VariantTest, OverloadedDispatchesToTheMatchingLambda) {
  const std::variant<int, std::string, double> value = std::string("hello");
  const std::string result = std::visit(
      Overloaded{
          [](int /*val*/) -> std::string { return "int"; },
          [](const std::string& val) -> std::string { return "string:" + val; },
          [](double /*val*/) -> std::string { return "double"; },
      },
      value);
  EXPECT_THAT(result, "string:hello");
}

TEST_F(VariantTest, OverloadedDispatchesEachAlternative) {
  using Var = std::variant<int, std::string, double>;
  const auto visitor = Overloaded{
      [](int /*val*/) -> std::string { return "int"; },
      [](const std::string& /*val*/) -> std::string { return "string"; },
      [](double /*val*/) -> std::string { return "double"; },
  };
  EXPECT_THAT(std::visit(visitor, Var(42)), "int");
  EXPECT_THAT(std::visit(visitor, Var(std::string("x"))), "string");
  EXPECT_THAT(std::visit(visitor, Var(1.5)), "double");
}

TEST_F(VariantTest, OverloadedSupportsACatchAllFallback) {
  // A generic lambda absorbs everything the specific ones do not name.
  using Var = std::variant<int, std::string, double>;
  const auto visitor = Overloaded{
      [](const std::string& /*val*/) -> std::string { return "string"; },
      [](auto&& /*val*/) -> std::string { return "other"; },
  };
  const Var str = std::string("x");
  EXPECT_THAT(std::visit(visitor, str), "string") << "the specific overload wins for a const lvalue";
  EXPECT_THAT(std::visit(visitor, Var(42)), "other");
  EXPECT_THAT(std::visit(visitor, Var(1.5)), "other");
}

TEST_F(VariantTest, OverloadedCatchAllBeatsAConstRefOverloadForATemporary) {
  // A trap worth pinning rather than discovering in review: visiting a TEMPORARY
  // passes `std::string&&`, which `auto&&` binds exactly while `const std::string&`
  // has to add const - so the generic lambda wins and the specific one never runs.
  // Take the alternative by value or by `&&` if that matters.
  using Var = std::variant<int, std::string, double>;
  const auto visitor = Overloaded{
      [](const std::string& /*val*/) -> std::string { return "string"; },
      [](auto&& /*val*/) -> std::string { return "other"; },
  };
  EXPECT_THAT(std::visit(visitor, Var(std::string("x"))), "other");

  // Spelling the specific overload `std::string&&` makes it win again.
  const auto rvalue_visitor = Overloaded{
      // The `&&` is the point - it is what makes this overload win - and the argument
      // is deliberately unused, so there is nothing to move from.
      // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
      [](std::string&& /*val*/) -> std::string { return "string"; },
      [](auto&& /*val*/) -> std::string { return "other"; },
  };
  EXPECT_THAT(std::visit(rvalue_visitor, Var(std::string("x"))), "string");
}

TEST_F(VariantTest, OverloadedDeductionGuideDeducesFromLambdas) {
  // Without the deduction guide `Overloaded{...}` would not compile at all.
  const auto visitor = Overloaded{[](int val) { return val * 2; }};
  EXPECT_THAT(visitor(21), 42);
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::types
