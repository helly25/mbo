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

#ifndef MBO_TYPES_TRAITS_H_
#define MBO_TYPES_TRAITS_H_

#include <concepts>
#include <cstddef>  // IWYU pragma: keep
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "mbo/types/internal/decompose_count.h"          // IWYU pragma: export
#include "mbo/types/internal/is_braces_constructible.h"  // IWYU pragma: export
#include "mbo/types/tuple.h"                             // IWYU pragma: export

namespace mbo::types {

// Determines whether `std::is_aggrgate_v<T> == true`.
template<typename T>
concept IsAggregate = types_internal::IsAggregate<T>;

// If `T` is a struct and the below conditions are met, return the number of
// variable fields it decomposes into.
//
// Requirements:
// * T is an aggregate.
// * T has no or only empty base classes.
// * Has at least one field.
//
// This templated constant does not answer whether a tuple can be constructed,
// because en empty tuple can be created for an empty aggregate, but the latter
// cannot be decomposed with structured bindings.
//
// The empty aggregate to empty tuple can be checked with `CanCreateTuple`.
template<typename T>
inline constexpr std::size_t DecomposeCountV = types_internal::DecomposeCountImpl<T>::value;

// If a `T` cannot be decomposed, then `decompose_count` returns this value.
inline constexpr std::size_t NotDecomposableV = types_internal::NotDecomposableImpl::value;

// Returns whether `T` can be decomposed.
// This is only true if a non empty structured binding of T is possible:
//   const auto& [a0....] = T()
template<typename T>
concept IsDecomposable =
    types_internal::IsAggregate<T> && (DecomposeCountV<T> != 0) && (DecomposeCountV<T> != NotDecomposableV);

// Returns whether `T` can be constructed from `Args...`.
template<typename T, typename... Args>
inline constexpr bool IsBracesConstructibleV = types_internal::IsBracesConstructibleImplT<T, Args...>::value;

namespace types_internal {
template<typename Container>
concept IsForwardIteratableRaw = requires(Container container, const Container const_container) {
  requires std::forward_iterator<typename Container::iterator>;
  requires std::forward_iterator<typename Container::const_iterator>;
  requires std::same_as<typename Container::reference, typename Container::value_type&>
               || std::same_as<typename Container::reference, const typename Container::value_type&>;
  requires std::same_as<typename Container::const_reference, const typename Container::value_type&>;
  requires std::forward_iterator<typename Container::iterator>;
  requires std::forward_iterator<typename Container::const_iterator>;
  { container.begin() } -> std::same_as<typename Container::iterator>;
  { container.end() } -> std::same_as<typename Container::iterator>;
  { const_container.begin() } -> std::same_as<typename Container::const_iterator>;
  { const_container.end() } -> std::same_as<typename Container::const_iterator>;
};
}  // namespace types_internal

// Identifies `Container` types that are at least forward iteratable (this include `std::initializer_list`).
template<typename Container>
concept IsForwardIteratable = types_internal::IsForwardIteratableRaw<std::remove_cvref_t<Container>>;

namespace types_internal {

template<typename Container>
concept ContainerIsForwardIteratableRaw = requires(Container container, const Container const_container) {
  requires IsForwardIteratableRaw<Container>;
  requires std::signed_integral<typename Container::difference_type>;
  requires std::same_as<
      typename Container::difference_type,
      typename std::iterator_traits<typename Container::iterator>::difference_type>;
  requires std::same_as<
      typename Container::difference_type,
      typename std::iterator_traits<typename Container::const_iterator>::difference_type>;
  // Technically `cbegin`, `cend`, `empty` and `size` are not required.
  // But we require them nonetheless (for now) as decent container implementation will provide them.
  { container.cbegin() } -> std::same_as<typename Container::const_iterator>;
  { container.cend() } -> std::same_as<typename Container::const_iterator>;
  { container.empty() } -> std::same_as<bool>;
  { container.size() } -> std::same_as<typename Container::size_type>;
};

template<typename Container>
concept ContainerHasInputIteratorImpl = requires(Container container, const Container const_container) {
  typename Container::iterator;
  typename Container::const_iterator;
  requires std::input_iterator<typename Container::iterator>;
  requires std::input_iterator<typename Container::const_iterator>;
  { container.begin() } -> std::same_as<typename Container::iterator>;
  { container.end() } -> std::same_as<typename Container::iterator>;
  { const_container.begin() } -> std::same_as<typename Container::const_iterator>;
  { const_container.end() } -> std::same_as<typename Container::const_iterator>;
};

template<typename Container>
concept ContainerHasForwardIteratorImpl = requires(Container container, const Container const_container) {
  typename Container::iterator;
  typename Container::const_iterator;
  requires std::forward_iterator<typename Container::iterator>;
  requires std::forward_iterator<typename Container::const_iterator>;
  { container.begin() } -> std::same_as<typename Container::iterator>;
  { container.end() } -> std::same_as<typename Container::iterator>;
  { const_container.begin() } -> std::same_as<typename Container::const_iterator>;
  { const_container.end() } -> std::same_as<typename Container::const_iterator>;
};
}  // namespace types_internal

// Identifies STL like `Container` types that are at least iteratable.
template<typename Container>
concept ContainerIsForwardIteratable = types_internal::ContainerIsForwardIteratableRaw<std::remove_cvref_t<Container>>;

// Similar to `ContainerIsForwardIteratable` but only checks for `begin`, `end` and iterators.
// Here the iterators must comply with `std::input_iterator`.
template<typename Container>
concept ContainerHasInputIterator = types_internal::ContainerHasInputIteratorImpl<std::remove_cvref_t<Container>>;

// Similar to `ContainerIsForwardIteratable` but only checks for `begin`, `end` and iterators.
// Here the iterators must comply with `std::forward_iterator`.
template<typename Container>
concept ContainerHasForwardIterator = types_internal::ContainerHasForwardIteratorImpl<std::remove_cvref_t<Container>>;

template<typename T>
concept HasDifferenceType = requires { typename T::difference_type; };

namespace types_internal {

template<typename T, bool = HasDifferenceType<T>>
struct GetDifferenceTypeImpl {
  using type = std::ptrdiff_t;
};

template<typename T>
struct GetDifferenceTypeImpl<T, true> {
  using type = T::difference_type;
};

}  // namespace types_internal

template<typename T>
using GetDifferenceType = types_internal::GetDifferenceTypeImpl<T>::type;

// Identifies std like `Container` types that support `emplace` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasEmplace = requires(Container container, ValueType new_value) {
  requires ContainerIsForwardIteratable<Container>;
  requires std::constructible_from<typename Container::value_type, ValueType>;
  { container.emplace(new_value) };  // Only tests whether emplace(<arg>) exists, not whether new_value can be that arg.
};

// Identifies std like `Container` types that support `emplace_back` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasEmplaceBack = requires(Container container, ValueType new_value) {
  requires ContainerIsForwardIteratable<Container>;
  requires std::constructible_from<typename Container::value_type, ValueType>;
  { container.emplace_back(new_value) };
};

// Identifies std like `Container` types that support `emplace` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasInsert = requires(Container container, ValueType new_value) {
  requires ContainerIsForwardIteratable<Container>;
  requires std::convertible_to<ValueType, typename Container::value_type>;
  { container.insert(new_value) };
};

// Identifies std like `Container` types that support `emplace_back` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasPushBack = requires(Container container, ValueType new_value) {
  requires ContainerIsForwardIteratable<Container>;
  requires std::convertible_to<ValueType, typename Container::value_type>;
  { container.push_back(new_value) };
};

namespace types_internal {

template<typename T>
struct IsInitializerListImpl : std::false_type {};

template<typename T>
struct IsInitializerListImpl<std::initializer_list<T>> : std::true_type {};

}  // namespace types_internal

template<typename T>
concept IsInitializerList = types_internal::IsInitializerListImpl<std::remove_cvref_t<T>>::value;

template<typename T>
concept NotInitializerList = !IsInitializerList<T>;

template<typename T>
concept HasValueType = requires { typename T::value_type; };

namespace types_internal {

template<std::forward_iterator It, bool = HasValueType<It>>
struct ForwardIteratorValueTypeImpl {
  using type = It::value_type;
};

template<std::forward_iterator It>
struct ForwardIteratorValueTypeImpl<It, false> {
  using type = std::remove_reference_t<decltype(*std::declval<It&>())>;
};

}  // namespace types_internal

template<std::forward_iterator It>
using ForwardIteratorValueType = types_internal::ForwardIteratorValueTypeImpl<It>::type;

template<ContainerHasInputIterator Container>
using ContainerConstIteratorValueType =
    std::iterator_traits<typename std::remove_cvref_t<Container>::const_iterator>::value_type;

namespace types_internal {

template<typename T>
struct IsCharArrayImpl : std::false_type {};

template<>
struct IsCharArrayImpl<char*> : std::true_type {};

template<>
struct IsCharArrayImpl<const char*> : std::true_type {};

}  // namespace types_internal

template<typename T>
concept IsCharArray = types_internal::IsCharArrayImpl<std::decay_t<T>>::value;

template<typename T>
concept NotIsCharArray = !IsCharArray<T>;

namespace types_internal {

struct NoFunc final {};

template<typename T, typename Func>
struct ValueOrResult {
  using type = std::invoke_result_t<Func, T>;
};

template<typename T>
struct ValueOrResult<T, NoFunc> {
  using type = T;
};

template<typename T, typename Func>
using ValueOrResultT = ValueOrResult<T, Func>::type;

template<typename ContainerIn, typename ContainerOut, typename Func = NoFunc>
concept ContainerCopyConvertibleRaw =                                              //
    (ContainerIsForwardIteratable<ContainerIn> || IsInitializerList<ContainerIn>)  //
    &&ContainerIsForwardIteratable<ContainerOut>
    && (ContainerHasEmplace<ContainerOut, ValueOrResultT<typename ContainerIn::value_type, Func>>
        || ContainerHasEmplaceBack<ContainerOut, ValueOrResultT<typename ContainerIn::value_type, Func>>
        || ContainerHasInsert<ContainerOut, ValueOrResultT<typename ContainerIn::value_type, Func>>
        || ContainerHasPushBack<ContainerOut, ValueOrResultT<typename ContainerIn::value_type, Func>>);

}  // namespace types_internal

template<typename ContainerIn, typename ContainerOut, typename Func = types_internal::NoFunc>
concept ContainerCopyConvertible = types_internal::
    ContainerCopyConvertibleRaw<std::remove_cvref_t<ContainerIn>, std::remove_reference_t<ContainerOut>, Func>;

template<typename T>
concept IsPair = requires(T pair) {
  typename T::first_type;
  typename T::second_type;
  requires std::same_as<T, std::pair<typename T::first_type, typename T::second_type>>;
};

template<typename T>
concept IsPairFirstStr = IsPair<T> && requires(T pair) {
  typename T::first_type;
  requires std::is_convertible_v<typename T::first_type, std::string_view>;
};

template<typename T>
concept IsSet = requires {
  typename T::value_type;
  typename T::key_type;
  typename T::allocator_type;
  requires std::same_as<T, std::set<typename T::value_type, typename T::key_type, typename T::allocator_type>>;
};

template<typename T>
concept IsVector = requires {
  typename T::value_type;
  typename T::allocator_type;
  requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
};

namespace types_internal {

template<typename SameAs, typename... Ts>
concept IsSameAsAnyOfRawImpl = (std::same_as<SameAs, std::remove_cvref_t<Ts>> || ...);

}  // namespace types_internal

// Test whether `SameAs` is one of a list of types `Ts` after removing `const`, `volatile` or `&`.
//
// Example: The following example takes any form of `int`
// or `unsigned` where `const`, `volatile` or `&` are removed. So an `int*` would not be acceptable.
//
// ```
// template <IsSameAsAnyOfRaw<int, unsigned> T>
// void Func(T) {};
// ```
template<typename SameAs, typename... Ts>
concept IsSameAsAnyOfRaw = types_internal::IsSameAsAnyOfRawImpl<std::remove_cvref_t<SameAs>, Ts...>;

template<typename SameAs, typename... Ts>
concept IsSameAsAnyOf = (std::same_as<SameAs, Ts> || ...);

// Inverse of the above. So this prevents specific types. This is necessary when building overload
// alternatives.
template<typename SameAs, typename... Ts>
concept NotSameAsAnyOfRaw = !IsSameAsAnyOfRaw<std::remove_cvref_t<SameAs>, Ts...>;

// Test whether two types are identical after removing `const`, `volatile` or `&`.
template<typename SameAs, typename T>
concept IsSameAsRaw = IsSameAsAnyOfRaw<SameAs, T>;

namespace types_internal {

template<typename T>
struct IsVariantImpl : std::false_type {};

template<typename... Args>
struct IsVariantImpl<std::variant<Args...>> : std::true_type {};

}  // namespace types_internal

// Determine whether `T` is a `std::variant<...>` type.
template<typename T>
concept IsVariant = types_internal::IsVariantImpl<T>::value;

// Determines whether `T` can be constructed from an empty base and `Args`.
//
// This is used in mbo::types::Extend<...>::ConstructFrom{Tuple|Args}.
//
// See: https://godbolt.org/z/3nnsG6bEb
template<typename T, typename... Args>
concept IsConstructibleWithEmptyBaseAndArgs = requires(Args&&... args) {
  { T{{}, {std::forward<Args>(args)}...} };
};

template<typename T>
struct IsSmartPtrImpl : std::false_type {};

template<typename T>
struct IsSmartPtrImpl<std::shared_ptr<T>> : std::true_type {};

template<typename T>
struct IsSmartPtrImpl<std::unique_ptr<T>> : std::true_type {};

template<typename T>
struct IsSmartPtrImpl<std::weak_ptr<T>> : std::true_type {};

// Identify smart pointers (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`).
// Can be exptended by providing additional specicializations for `IsSmartPtrImpl`.
template<typename T>
concept IsSmartPtr = IsSmartPtrImpl<T>::value;

template<typename T>
concept IsOptional = requires {
  typename T::value_type;
  requires std::same_as<std::optional<typename T::value_type>, T>;
};

// Verify whether `From` can used used to construct a `Into`.
// That allows the concept to be used for in-place/auto template parameters.
// See Compiler Explorer debug: https://godbolt.org/z/5vMEeGMMP
//
// Motivating example:
//
// A function that is meant to handle any string-like type (std::string/_view,
// char*, ...). That still supports perfect forwarding (or move). In order to
// have a short notation we need the `std::constructible_from` parameter order
// switched. That is what this concept does.
//
// ```
// struct Type {
//   Type(ConstructibleInto<std::string> auto&& v) : name_(std::forward<decltype(v)>(v)) {}
//
//   const std::string name;
// };
// ```
//
// Second variant, doing the same:
//
// ```
// struct Type {
//   template<ConstructibleInto<std::string> Str>
//   Type(Str&& v) : name_(int, std::forward<decltype(v)>(v)) {}
//
//   const std::string name;
// };
// ```
template<typename From, typename Into>
concept ConstructibleInto = std::constructible_from<Into, From>;

}  // namespace mbo::types

#endif  // MBO_TYPES_TRAITS_H_
