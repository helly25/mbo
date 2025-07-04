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

#ifndef MBO_TYPES_TUPLE_EXTRAS_H_
#define MBO_TYPES_TUPLE_EXTRAS_H_

#include <utility>

#include "mbo/types/internal/decompose_count.h"  // IWYU pragma: export
#include "mbo/types/traits.h"

namespace mbo::types {

// Requires aggregates that have no or only empty base classes, see
// https://en.cppreference.com/w/cpp/language/aggregate_initialization.
//
// There could be additional requirements:
// - on `std::default_initializable<T>`: but that is technically not necessary.
// - on `!types_internal::StructHasNonEmptyBase<T>`:
//
// Note that we also consider empty aggregates here as we are able to create an
// empty tuple for them.
template<typename T>
concept CanCreateTuple = types_internal::DecomposeCondition<T> || (IsAggregate<T> && IsEmptyType<T>);

// Constructs a tuple from any eligible struct `T`
//
// The resulting tuple is made up of references!
template<typename T>
requires CanCreateTuple<std::remove_cvref_t<T>>
inline constexpr auto StructToTuple(T&& v) {
  return types_internal::DecomposeHelper::ToTuple(std::forward<T>(v));
}

namespace types_internal {

template<typename Tuple, std::size_t Index>
concept IsUnionMemberAt = std::is_union_v<std::tuple_element_t<Index, Tuple>>;

template<typename Tuple, typename Indices>
struct HasUnionMemberIdxImpl;

template<typename Tuple, std::size_t... kIndex>
struct HasUnionMemberIdxImpl<Tuple, std::index_sequence<kIndex...>>
    : std::bool_constant<(... || IsUnionMemberAt<Tuple, kIndex>)> {};

template<typename Tuple>
concept HasUnionMemberImpl = HasUnionMemberIdxImpl<Tuple, std::make_index_sequence<std::tuple_size_v<Tuple>>>::value;

template<typename Tuple, std::size_t Index>
concept IsVariantMemberAt = IsVariant<std::tuple_element_t<Index, Tuple>>;

template<typename Tuple, typename Indices>
struct HasVariantMemberIdxImpl;

template<typename Tuple, std::size_t... kIndex>
struct HasVariantMemberIdxImpl<Tuple, std::index_sequence<kIndex...>>
    : std::bool_constant<(... || IsVariantMemberAt<Tuple, kIndex>)> {};

template<typename Tuple>
concept HasVariantMemberImpl =
    HasVariantMemberIdxImpl<Tuple, std::make_index_sequence<std::tuple_size_v<Tuple>>>::value;

}  // namespace types_internal

// Determine whether a type that can be converted into a `tuple` using `StructToTuple` has at least one `union` member.
// Such types cannot be used with `__builtin_dump_struct` in a `constexpr` context.
template<typename T>
concept HasUnionMember =
    ::mbo::types::CanCreateTuple<T>
    && types_internal::HasUnionMemberImpl<decltype(::mbo::types::StructToTuple(std::declval<T>()))>;

// Determine whether a type that can be converted into a `tuple` using `StructToTuple` has at least one `std::variant`
// member.  Such types cannot be used with `__builtin_dump_struct` in a `constexpr` context.
template<typename T>
concept HasVariantMember =
    ::mbo::types::CanCreateTuple<T>
    && types_internal::HasVariantMemberImpl<decltype(::mbo::types::StructToTuple(std::declval<T>()))>;

namespace types_internal {

template<typename Fields, typename Args, typename IS>
struct TupleFieldsConstructibleImpl;

template<typename Fields, typename Args, std::size_t... Idx>
struct TupleFieldsConstructibleImpl<Fields, Args, std::index_sequence<Idx...>>
    : std::bool_constant<(
          ConstructibleFrom<std::tuple_element_t<Idx, Fields>, std::tuple_element_t<Idx, Args>> && ...)> {};

}  // namespace types_internal

// Determine whether each element in `Fields` can be constructed from their `Args` counterpart.
template<typename Fields, typename Args>
concept TupleFieldsConstructible =
    IsTuple<Fields> && IsTuple<Args> && std::tuple_size_v<Fields> == std::tuple_size_v<Args>
    && types_internal::TupleFieldsConstructibleImpl<Fields, Args, std::make_index_sequence<std::tuple_size_v<Fields>>>::
        value;

}  // namespace mbo::types

#endif  // MBO_TYPES_TUPLE_EXTRAS_H_
