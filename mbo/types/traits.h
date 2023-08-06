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

#include <cstddef>  // IWYU pragma: keep

#include "mbo/types/internal/decompose_count.h"          // IWYU pragma: export
#include "mbo/types/internal/is_braces_constructible.h"  // IWYU pragma: export

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
template<typename T>
inline constexpr std::size_t DecomposeCountV = types_internal::DecomposeCountImpl<T>::value;

// If a `T` cannot be decomposed, then `decompose_count` returns this value.
inline constexpr std::size_t NotDecomposableV = types_internal::NotDecomposableImpl::value;

// Returns whether `T` can be decomposed.
// This is only true if a non empty structured binding of T is possible:
//   const auto& [a0....] = T()
template <typename T>
concept IsDecomposable = types_internal::IsAggregate<T> &&
                         (DecomposeCountV<T> != 0) &&
                         (DecomposeCountV<T> != NotDecomposableV);

// Returns whether `T` can be constructed from `Args...`.
template<typename T, typename... Args>
inline constexpr bool IsBracesConstructibleV = types_internal::IsBracesConstructibleImplT<T, Args...>::value;

// Identifies std like `Container` types that are at least iteratable.
template<typename Container>
concept ContainerIsForwardIteratableRaw = requires (Container container, const Container const_container) {
  requires std::forward_iterator<typename Container::iterator>;
  requires std::forward_iterator<typename Container::const_iterator>;
  requires std::same_as<typename Container::reference, typename Container::value_type &>;
  requires std::same_as<typename Container::const_reference, const typename Container::value_type &>;
  requires std::forward_iterator<typename Container::iterator>;
  requires std::forward_iterator<typename Container::const_iterator>;
  requires std::signed_integral<typename Container::difference_type>;
  requires std::same_as<typename Container::difference_type, typename std::iterator_traits<typename Container::iterator>::difference_type>;
  requires std::same_as<typename Container::difference_type, typename std::iterator_traits<typename Container::const_iterator>::difference_type>;
  { container.begin() } -> std::same_as<typename Container::iterator>;
  { container.end() } -> std::same_as<typename Container::iterator>;
  { const_container.begin() } -> std::same_as<typename Container::const_iterator>;
  { const_container.end() } -> std::same_as<typename Container::const_iterator>;
  { container.cbegin() } -> std::same_as<typename Container::const_iterator>;
  { container.cend() } -> std::same_as<typename Container::const_iterator>;
  { container.size() } -> std::same_as<typename Container::size_type>;
  { container.empty() } -> std::same_as<bool>;
};

template<typename Container>
concept ContainerIsForwardIteratable = ContainerIsForwardIteratableRaw<std::remove_cvref_t<Container>>;

// Identifies std like `Container` types that support `emplace` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasEmplace = requires(Container container, ValueType new_value) {
  ContainerIsForwardIteratable<Container>;
  container.emplace(new_value);
};

// Identifies std like `Container` types that support `emplace_back` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasEmplaceBack = requires(Container container, ValueType new_value) {
  ContainerIsForwardIteratable<Container>;
  container.emplace_back(new_value);
};

// Identifies std like `Container` types that support `emplace` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasInsert = requires(Container container, ValueType new_value) {
  ContainerIsForwardIteratable<Container>;
  container.insert(new_value);
};

// Identifies std like `Container` types that support `emplace_back` with `ValueType`.
template<typename Container, typename ValueType>
concept ContainerHasPushBack = requires(Container container, ValueType new_value) {
  ContainerIsForwardIteratable<Container>;
  container.push_back(new_value);
};

}  // namespace mbo::types

#endif  // MBO_TYPES_TRAITS_H_
