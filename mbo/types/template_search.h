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

#ifndef MBO_TYPES_TEMPLATE_SEARCH_H_
#define MBO_TYPES_TEMPLATE_SEARCH_H_

// IWYU pragma: private, include "mbo/types/internal/decompose_count.h"

#include <cstddef>  // IWYU pragma: keep
#include <type_traits>

namespace mbo::types {

// Template `BinarySearch` - find higest value in `[Start, End[` for which `Predicate` is true.
// Returns `NotFound` otherwise.
// The `Predicate` must provide a boolean constant `value`.
template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
struct BinarySearch;

namespace types_internal {

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
using BinarySearchImpl = std::conditional_t < (Start + 1 >= End),
      std::conditional_t<
          Start<
              End && Predicate<Start>::value,
              std::integral_constant<std::size_t, Start>,
              std::integral_constant<std::size_t, NotFound>>,
          std::conditional_t<
              Predicate<(Start + End) / 2>::value,
              BinarySearch<Predicate, (Start + End) / 2, End, NotFound>,
              BinarySearch<Predicate, Start, (Start + End) / 2, NotFound>>>;

}  // namespace types_internal

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
struct BinarySearch : types_internal::BinarySearchImpl<Predicate, Start, End, NotFound> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
constexpr std::size_t BinarySearchV = BinarySearch<Predicate, Start, End, NotFound>::value;

// Template `LinearSearch`
//
// Finds the first value in `[Start, End[` for which `Predicate` is true.
// Returns `NotFound` otherwise.
// The `Predicate` must provide a boolean constant `value`.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
struct LinearSearch;

namespace types_internal {

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
using LinearSearchImpl = std::conditional_t<
    Start >= End,
    std::integral_constant<std::size_t, NotFound>,
    std::conditional_t<
        Predicate<Start>::value,
        std::integral_constant<std::size_t, Start>,
        LinearSearch<Predicate, Start + 1, End, NotFound>>>;

}  // namespace types_internal

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
struct LinearSearch : types_internal::LinearSearchImpl<Predicate, Start, End, NotFound> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
constexpr std::size_t LinearSearchV = LinearSearch<Predicate, Start, End, NotFound>::value;

// Template `MaxSearch`
//
// Finds the highest value in `[Start, End[` for which `Predicate` is true.
// Requires that `[Start]` is true. Otherwise returns `NotFound`.
// In other words it find the last for which the `Predicate` returns true.
// This is brute force `BinarySearch` and in contrast to the former it requires `Predicate` to be true for **all** left
// values.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
struct MaxSearch;

namespace types_internal {

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
using MaxSearchImpl = std::conditional_t<
    (Start >= End),
    std::integral_constant<std::size_t, NotFound>,
    std::conditional_t<
        Predicate<Start>::value,
        MaxSearch<Predicate, Start + 1, End, Start>,
        std::integral_constant<std::size_t, NotFound>>>;

}  // namespace types_internal

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
struct MaxSearch : types_internal::MaxSearchImpl<Predicate, Start, End, NotFound> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
constexpr std::size_t MaxSearchV = MaxSearch<Predicate, Start, End, NotFound>::value;

// Template `ReverseSearch`
//
// Reverse linear search from End - 1 to Start.
//
// Returns `End` if no value was found.
// The smallest value that can be returned is Start, independent of conditions.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
struct ReverseSearch;

namespace types_internal {

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound>
using ReverseSearchImpl = std::conditional_t<
    (End <= Start),
    std::integral_constant<std::size_t, NotFound>,
    std::conditional_t<
        Predicate<End - 1>::value,
        std::integral_constant<std::size_t, End - 1>,
        ReverseSearch<Predicate, Start, End - 1, NotFound>>>;

}  // namespace types_internal

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
struct ReverseSearch : types_internal::ReverseSearchImpl<Predicate, Start, End, NotFound> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End, std::size_t NotFound = End>
constexpr std::size_t ReverseSearchV = ReverseSearch<Predicate, Start, End, NotFound>::value;

}  // namespace mbo::types

#endif  // MBO_TYPES_TEMPLATE_SEARCH_H_
