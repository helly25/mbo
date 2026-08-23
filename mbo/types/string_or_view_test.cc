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

#include "mbo/types/string_or_view.h"

#include <compare>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo {
namespace {

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Not;

constexpr StringOrView kEmptyLiteral{""};
constexpr StringOrView kLiteral{"literal"};
static_assert(kEmptyLiteral.view().empty());
static_assert(kLiteral.view() == "literal");
static_assert(!kLiteral.owns_string());
static_assert(std::is_nothrow_move_constructible_v<StringOrView>);
static_assert(std::is_nothrow_move_assignable_v<StringOrView>);
static_assert(!std::is_constructible_v<StringOrView, const char*>);

struct StringOrViewTest : ::testing::Test {};

TEST_F(StringOrViewTest, DefaultIsEmptyBorrowedView) {
  constexpr StringOrView value;
  EXPECT_THAT(value.view(), IsEmpty());
  EXPECT_THAT(value.owns_string(), IsFalse());
}

TEST_F(StringOrViewTest, BorrowsStringViewAndLiteral) {
  constexpr std::string_view kText = "borrowed";
  const StringOrView borrowed{kText};
  const StringOrView literal{"literal"};

  EXPECT_THAT(borrowed.view(), Eq(kText));
  EXPECT_THAT(&borrowed.view().front(), Eq(&kText.front()));
  EXPECT_THAT(borrowed.owns_string(), IsFalse());
  EXPECT_THAT(literal.view(), Eq("literal"));
  EXPECT_THAT(literal.view().size(), Eq(7));
  EXPECT_THAT(literal.owns_string(), IsFalse());
}

TEST_F(StringOrViewTest, OwnsCopiedLvalueAndMovedRvalue) {
  std::string source = "copied source";
  const StringOrView copied{source};
  source.assign("changed");

  std::string moved_source = "moved source";
  const StringOrView moved{std::move(moved_source)};

  EXPECT_THAT(copied.view(), Eq("copied source"));
  EXPECT_THAT(copied.owns_string(), IsTrue());
  EXPECT_THAT(moved.view(), Eq("moved source"));
  EXPECT_THAT(moved.owns_string(), IsTrue());
}

TEST_F(StringOrViewTest, CopyPreservesRepresentationAndLifetime) {
  constexpr std::string_view kBorrowedText = "borrowed";
  const StringOrView borrowed{kBorrowedText};
  const StringOrView borrowed_copy{borrowed};  // NOLINT(performance-unnecessary-copy-initialization)
  EXPECT_THAT(&borrowed_copy.view().front(), Eq(&borrowed.view().front()));
  EXPECT_THAT(borrowed_copy.owns_string(), IsFalse());

  const StringOrView owned{std::string(128, 'x')};
  StringOrView owned_copy{owned};
  EXPECT_THAT(&owned_copy.view().front(), Not(Eq(&owned.view().front())));
  owned_copy = StringOrView{"replacement"};
  EXPECT_THAT(owned.view(), Eq(std::string(128, 'x')));
  EXPECT_THAT(owned.owns_string(), IsTrue());
}

TEST_F(StringOrViewTest, CopyAndMoveAssignmentPreserveRepresentation) {
  constexpr std::string_view kBorrowedText = "borrowed";
  const StringOrView borrowed{kBorrowedText};
  StringOrView assigned{std::string{"owned"}};
  assigned = borrowed;
  EXPECT_THAT(&assigned.view().front(), Eq(&kBorrowedText.front()));
  EXPECT_THAT(assigned.owns_string(), IsFalse());

  const StringOrView owned{std::string{"owned"}};
  assigned = owned;
  EXPECT_THAT(assigned.view(), Eq("owned"));
  EXPECT_THAT(assigned.owns_string(), IsTrue());

  StringOrView moved;
  moved = std::move(assigned);
  EXPECT_THAT(moved.view(), Eq("owned"));
  EXPECT_THAT(moved.owns_string(), IsTrue());

  StringOrView moved_borrowed;
  moved_borrowed = StringOrView{kBorrowedText};
  EXPECT_THAT(&moved_borrowed.view().front(), Eq(&kBorrowedText.front()));
  EXPECT_THAT(moved_borrowed.owns_string(), IsFalse());
}

TEST_F(StringOrViewTest, DistinguishesOwnedAndBorrowedEmptyValues) {
  const StringOrView borrowed{std::string_view{}};
  const StringOrView owned{std::string{}};
  EXPECT_THAT(borrowed.view(), IsEmpty());
  EXPECT_THAT(borrowed.owns_string(), IsFalse());
  EXPECT_THAT(owned.view(), IsEmpty());
  EXPECT_THAT(owned.owns_string(), IsTrue());
  EXPECT_THAT(owned, Eq(borrowed));
}

TEST_F(StringOrViewTest, PreservesEmbeddedNulBytes) {
  constexpr StringOrView value{"a\0b"};
  EXPECT_THAT(value.view(), Eq(std::string_view{"a\0b", 3}));
}

TEST_F(StringOrViewTest, ComparesTextIndependentOfOwnership) {
  const StringOrView owned{std::string{"same"}};
  const StringOrView borrowed{std::string_view{"same"}};
  EXPECT_THAT(owned, Eq(borrowed));
  EXPECT_THAT(owned == std::string_view{"same"}, IsTrue());
  EXPECT_THAT(std::string_view{"same"} == owned, IsTrue());
  EXPECT_THAT(owned == std::string{"same"}, IsTrue());
  EXPECT_THAT(owned == "same", IsTrue());
  EXPECT_THAT(owned <=> "z", Eq(std::strong_ordering::less));
  EXPECT_THAT("a" <=> owned, Eq(std::strong_ordering::less));
}

TEST_F(StringOrViewTest, SupportsAbslStringify) {
  const StringOrView value{std::string{"formatted"}};
  EXPECT_THAT(absl::StrFormat("%v", value), Eq("formatted"));
}

}  // namespace
}  // namespace mbo
