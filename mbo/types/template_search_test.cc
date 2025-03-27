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

#include "mbo/types/template_search.h"

#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>

// NOLINTBEGIN(*-magic-numbers)

namespace mbo::types {
namespace {

template<int... Value>
struct Test {
  static constexpr std::array kValues = {Value..., 99'999};

  static constexpr std::size_t size() { return sizeof...(Value); }  // NOLINT(*-naming)

  static constexpr int at(std::size_t index, int bad = std::numeric_limits<int>::min()) {  // NOLINT(*-naming)
    return 0 <= index && index < size() ? kValues[index] : bad;  // NOLINT(*-array-index,*-redundant-expression)
  }
};

template<typename Test>
struct IsNonZero {
  template<std::size_t kIndex>
      struct TestFunc : std::bool_constant < kIndex<Test::size() && Test::at(kIndex)> {};
};

template<
    std::size_t Expected,
    template<
        template<std::size_t> typename,
        std::size_t Start,
        std::size_t End,
        std::size_t NotFound> typename Algorithm,
    template<typename> typename TestFunc,
    typename TestType,
    std::size_t Start = 0,
    std::size_t End = TestType::size(),
    std::size_t NotFound = End>
concept TestEqual = Expected == Algorithm<TestFunc<TestType>::template TestFunc, Start, End, NotFound>::value;

// BinarySearch

static_assert(TestEqual<0, BinarySearch, IsNonZero, Test<>>);
static_assert(TestEqual<99, BinarySearch, IsNonZero, Test<>, 2, 3, 99>);

static_assert(TestEqual<1, BinarySearch, IsNonZero, Test<0>>);
static_assert(TestEqual<0, BinarySearch, IsNonZero, Test<1>>);

static_assert(TestEqual<5, BinarySearch, IsNonZero, Test<0, 0, 0, 0, 0>>);
static_assert(TestEqual<99, BinarySearch, IsNonZero, Test<0, 0, 0, 0, 0>, 0, 5, 99>);
static_assert(TestEqual<0, BinarySearch, IsNonZero, Test<1, 0, 0, 0, 0>>);
static_assert(TestEqual<1, BinarySearch, IsNonZero, Test<1, 2, 0, 0, 0>>);
static_assert(TestEqual<2, BinarySearch, IsNonZero, Test<1, 2, 3, 0, 0>>);
static_assert(TestEqual<3, BinarySearch, IsNonZero, Test<1, 2, 3, 4, 0>>);
static_assert(TestEqual<4, BinarySearch, IsNonZero, Test<1, 2, 3, 4, 5>>);

// BinarySearch ignores values it does not test :-)
static_assert(TestEqual<4, BinarySearch, IsNonZero, Test<0, 2, 3, 4, 5>>);

// The next finds 5 because `6 - 1 = 5`: It finds the last position allowed which is 6 - 1:
static_assert(TestEqual<5, BinarySearch, IsNonZero, Test<1, 2, 3, 4, 5, 6, 7>, 3, 6, 99>);

// LinearSearch

static_assert(TestEqual<0, LinearSearch, IsNonZero, Test<>>);
static_assert(TestEqual<99, LinearSearch, IsNonZero, Test<>, 2, 3, 99>);

static_assert(TestEqual<1, LinearSearch, IsNonZero, Test<0>>);
static_assert(TestEqual<0, LinearSearch, IsNonZero, Test<1>>);

static_assert(TestEqual<5, LinearSearch, IsNonZero, Test<0, 0, 0, 0, 0>>);
static_assert(TestEqual<99, LinearSearch, IsNonZero, Test<0, 0, 0, 0, 0>, 0, 5, 99>);
static_assert(TestEqual<0, LinearSearch, IsNonZero, Test<1, 0, 0, 0, 0>>);
static_assert(TestEqual<1, LinearSearch, IsNonZero, Test<0, 2, 0, 0, 0>>);
static_assert(TestEqual<2, LinearSearch, IsNonZero, Test<0, 0, 3, 0, 0>>);
static_assert(TestEqual<3, LinearSearch, IsNonZero, Test<0, 0, 0, 4, 0>>);
static_assert(TestEqual<4, LinearSearch, IsNonZero, Test<0, 0, 0, 0, 5>>);
static_assert(TestEqual<99, LinearSearch, IsNonZero, Test<0, 0, 0, 0, 5>, 2, 4, 99>);
static_assert(TestEqual<5, LinearSearch, IsNonZero, Test<0, 0, 0, 0, 0, 6, 7>, 3, 6, 99>);

// MaxSearch

static_assert(TestEqual<0, MaxSearch, IsNonZero, Test<>>);
static_assert(TestEqual<99, MaxSearch, IsNonZero, Test<>, 2, 3, 99>);

static_assert(TestEqual<1, MaxSearch, IsNonZero, Test<0>>);
static_assert(TestEqual<0, MaxSearch, IsNonZero, Test<1>>);

static_assert(TestEqual<5, MaxSearch, IsNonZero, Test<0, 0, 0, 0, 0>>);
static_assert(TestEqual<99, MaxSearch, IsNonZero, Test<0, 0, 0, 0, 0>, 0, 5, 99>);
static_assert(TestEqual<0, MaxSearch, IsNonZero, Test<1, 0, 0, 0, 0>>);
static_assert(TestEqual<1, MaxSearch, IsNonZero, Test<1, 2, 0, 0, 0>>);
static_assert(TestEqual<2, MaxSearch, IsNonZero, Test<1, 2, 3, 0, 0>>);
static_assert(TestEqual<3, MaxSearch, IsNonZero, Test<1, 2, 3, 4, 0>>);
static_assert(TestEqual<4, MaxSearch, IsNonZero, Test<1, 2, 3, 4, 5>>);

// The next finds 5 because `6 - 1 = 5`: It finds the last position allowed which is 6 - 1:
static_assert(TestEqual<5, MaxSearch, IsNonZero, Test<1, 2, 3, 4, 5, 6, 7>, 3, 6, 99>);

// ReverseSearch

static_assert(TestEqual<0, ReverseSearch, IsNonZero, Test<>>);
static_assert(TestEqual<99, ReverseSearch, IsNonZero, Test<>, 2, 3, 99>);

static_assert(TestEqual<1, ReverseSearch, IsNonZero, Test<0>>);
static_assert(TestEqual<0, ReverseSearch, IsNonZero, Test<1>>);

static_assert(TestEqual<5, ReverseSearch, IsNonZero, Test<0, 0, 0, 0, 0>>);
static_assert(TestEqual<99, ReverseSearch, IsNonZero, Test<0, 0, 0, 0, 0>, 0, 5, 99>);
static_assert(TestEqual<0, ReverseSearch, IsNonZero, Test<1, 0, 0, 0, 0>>);
static_assert(TestEqual<1, ReverseSearch, IsNonZero, Test<0, 2, 0, 0, 0>>);
static_assert(TestEqual<2, ReverseSearch, IsNonZero, Test<0, 2, 3, 0, 0>>);
static_assert(TestEqual<3, ReverseSearch, IsNonZero, Test<0, 2, 3, 4, 0>>);
static_assert(TestEqual<4, ReverseSearch, IsNonZero, Test<0, 2, 3, 4, 5>>);

// The next finds 5 because `6 - 1 = 5`: It finds the last position allowed which is 6 - 1:
static_assert(TestEqual<5, ReverseSearch, IsNonZero, Test<0, 2, 3, 4, 5, 6, 7>, 3, 6, 99>);
// Similar:
static_assert(TestEqual<4, ReverseSearch, IsNonZero, Test<0, 2, 3, 4, 5, 0, 0>, 3, 6, 99>);
// Not found in range:
static_assert(TestEqual<99, ReverseSearch, IsNonZero, Test<0, 2, 3, 0, 0, 0, 0>, 3, 6, 99>);

}  // namespace
}  // namespace mbo::types

// NOLINTEND(*-magic-numbers)
