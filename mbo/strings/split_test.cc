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

#include "mbo/strings/split.h"

#include <concepts>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/strings/str_split.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep

namespace mbo::strings {
namespace {

using ::absl::SkipEmpty;

template<typename T>
struct SplitTest : ::testing::Test {
  template<typename Arg>
  static T StrSplit(std::string_view text, AtLast sep, Arg&& arg) {
    return T(absl::StrSplit(text, sep, std::forward<Arg>(arg)));
  }

  static T StrSplit(std::string_view text, AtLast sep) { return T(absl::StrSplit(text, sep)); }
};

template<typename T>
concept IsSet = requires {
  typename T::value_type;
  typename T::key_compare;
  typename T::allocator_type;
  requires std::same_as<T, std::set<typename T::value_type, typename T::key_compare, typename T::allocator_type>>;
};

MATCHER(IsEmpty, "") {
  if constexpr (mbo::types::IsPair<std::remove_cvref_t<arg_type>>) {
    return arg.first.empty() && arg.second.empty();
  } else {
    return arg.empty();
  }
}

MATCHER_P(Elements, elements, "") {
  if constexpr (IsSet<std::remove_cvref_t<arg_type>>) {
    const std::set<std::string> expected_set{elements.begin(), elements.end()};
    return ::testing::ExplainMatchResult(::testing::UnorderedElementsAreArray(expected_set), arg, result_listener);
  } else if constexpr (mbo::types::IsPair<std::remove_cvref_t<arg_type>>) {
    if (elements.size() == 2) {
      return elements[0] == arg.first && elements[1] == arg.second;
    } else if (elements.size() == 1) {
      return elements[0] == arg.first && arg.second.empty();
    } else if (elements.size() == 0) {
      return arg.first.empty() && arg.second.empty();
    } else {
      return ::testing::ExplainMatchResult(::testing::Le(2), elements.size(), result_listener);
    }
  } else {
    return ::testing::ExplainMatchResult(::testing::ElementsAreArray(elements), arg, result_listener);
  }
}

using MyTypes = ::testing::Types<         //
    std::list<std::string>,               //
    std::vector<std::string_view>,        //
    std::vector<std::string_view>,        //
    std::pair<std::string, std::string>,  //
    std::set<std::string>>;

TYPED_TEST_SUITE(SplitTest, MyTypes);

TYPED_TEST(SplitTest, SkipEmpty) {
  using I = std::vector<std::string_view>;
  using S = std::remove_cvref_t<decltype(*this)>;
  EXPECT_THAT(S::StrSplit("", AtLast('/'), SkipEmpty()), IsEmpty());
  EXPECT_THAT(S::StrSplit("/", AtLast('/'), SkipEmpty()), IsEmpty());
  EXPECT_THAT(S::StrSplit("//", AtLast('/'), SkipEmpty()), Elements(I{"/"}));
  EXPECT_THAT(S::StrSplit("a//", AtLast('/'), SkipEmpty()), Elements(I{"a/"}));
  EXPECT_THAT(S::StrSplit("a/b/", AtLast('/'), SkipEmpty()), Elements(I{"a/b"}));
  EXPECT_THAT(S::StrSplit("a/b/c", AtLast('/'), SkipEmpty()), Elements(I{"a/b", "c"}));
  EXPECT_THAT(S::StrSplit("a//c", AtLast('/'), SkipEmpty()), Elements(I{"a/", "c"}));
  EXPECT_THAT(S::StrSplit("/b/", AtLast('/'), SkipEmpty()), Elements(I{"/b"}));
  EXPECT_THAT(S::StrSplit("/b/c", AtLast('/'), SkipEmpty()), Elements(I{"/b", "c"}));
  EXPECT_THAT(S::StrSplit("//c", AtLast('/'), SkipEmpty()), Elements(I{"/", "c"}));
}

TYPED_TEST(SplitTest, NoSkipEmpty) {
  using I = std::vector<std::string_view>;
  using S = std::remove_cvref_t<decltype(*this)>;
  EXPECT_THAT(S::StrSplit("", AtLast('/')), Elements(I{""}));
  EXPECT_THAT(S::StrSplit("/", AtLast('/')), Elements(I{"", ""}));
  EXPECT_THAT(S::StrSplit("//", AtLast('/')), Elements(I{"/", ""}));
  EXPECT_THAT(S::StrSplit("a//", AtLast('/')), Elements(I{"a/", ""}));
  EXPECT_THAT(S::StrSplit("a/b/", AtLast('/')), Elements(I{"a/b", ""}));
  EXPECT_THAT(S::StrSplit("a/b/c", AtLast('/')), Elements(I{"a/b", "c"}));
  EXPECT_THAT(S::StrSplit("a//c", AtLast('/')), Elements(I{"a/", "c"}));
  EXPECT_THAT(S::StrSplit("/b/", AtLast('/')), Elements(I{"/b", ""}));
  EXPECT_THAT(S::StrSplit("/b/c", AtLast('/')), Elements(I{"/b", "c"}));
  EXPECT_THAT(S::StrSplit("//c", AtLast('/')), Elements(I{"/", "c"}));
}

}  // namespace
}  // namespace mbo::strings
