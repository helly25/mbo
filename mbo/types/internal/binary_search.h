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

#include <cstddef>
#include <type_traits>

namespace mbo::types::types_internal {

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

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_BINARY_SEARCH_H_
