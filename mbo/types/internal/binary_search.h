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

#ifndef MBO_TYPES_INTERNAL_BINARY_SEARCH_H_
#define MBO_TYPES_INTERNAL_BINARY_SEARCH_H_

// IWYU pragma private, include "mbo/types/internal/decompose_count.h"

#include <cstddef>  // IWYU pragma: keep
#include <type_traits>

namespace mbo::types::types_internal {

// Template `BinarySearch` - finds a value in [Start, End[ for which Predicate is true.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct BinarySearch;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
using BinarySearchImpl = std::conditional_t<
    (End - Start <= 1),
    std::integral_constant<std::size_t, Start>,
    std::conditional_t<
        Predicate<(Start + End) / 2>::value,
        BinarySearch<Predicate, (Start + End) / 2, End>,
        BinarySearch<Predicate, Start, (Start + End) / 2>>>;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct BinarySearch : BinarySearchImpl<Predicate, Start, End> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
constexpr std::size_t BinarySearchV = BinarySearch<Predicate, Start, End>::value;

// Template `LinearSearch`
//
// Finds the first value in [Start, End[ for which Predicate is true.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct LinearSearch;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
using LinearSearchImpl = std::conditional_t<
    (Start + 1 >= End),
    std::integral_constant<std::size_t, Start>,
    std::conditional_t<
        Predicate<Start>::value,
        std::integral_constant<std::size_t, Start>,
        LinearSearch<Predicate, Start + 1, End>>>;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct LinearSearch : LinearSearchImpl<Predicate, Start, End> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
constexpr std::size_t LinearSearchV = LinearSearch<Predicate, Start, End>::value;

// Template `MaxSearch`
//
// Finds the highest value in [Start, End[ for which Predicate is true.
// Requires that [Start] is true. Otherwise returns Start.
// In other words it find the last for which the Predicate returns true.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct MaxSearch;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
using MaxSearchImpl = std::conditional_t<
    (Start + 1 >= End),
    std::integral_constant<std::size_t, Start>,
    std::conditional_t<
        Predicate<Start + 1>::value,
        MaxSearch<Predicate, Start + 1, End>,
        std::integral_constant<std::size_t, Start>>>;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct MaxSearch : MaxSearchImpl<Predicate, Start, End> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
constexpr std::size_t MaxSearchV = MaxSearch<Predicate, Start, End>::value;

// Template `ReverseSearch`
//
// Reverse linear search from End - 1 to Start.
//
// Returns `End` if no value was found.
// The smallest value that can be returned is Start, independent of conditions.

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct ReverseSearch;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
using ReverseSearchImpl = std::conditional_t<
    (End <= Start),
    std::integral_constant<std::size_t, Start>,
    std::conditional_t<
        Predicate<End - 1>::value,
        std::integral_constant<std::size_t, End - 1>,
        ReverseSearch<Predicate, Start, End - 1>>>;

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
struct ReverseSearch : ReverseSearchImpl<Predicate, Start, End> {};

template<template<std::size_t> typename Predicate, std::size_t Start, std::size_t End>
constexpr std::size_t ReverseSearchV = ReverseSearch<Predicate, Start, End>::value;

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_BINARY_SEARCH_H_
