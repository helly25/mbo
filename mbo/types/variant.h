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

#ifndef MBO_TYPES_VARIANT_H
#define MBO_TYPES_VARIANT_H

#include <type_traits>
#include <variant>

namespace mbo::types {
namespace types_internal {

template<typename T>
struct IsVariantImpl : std::false_type {};

template<typename... Args>
struct IsVariantImpl<std::variant<Args...>> : std::true_type {};

}  // namespace types_internal

// Determine whether `T` is a `std::variant<...>` type.
template<typename T>
concept IsVariant = types_internal::IsVariantImpl<T>::value;

namespace types_internal {

template<
    typename V,
    typename T,
    std::size_t Index,
    bool = Index<std::variant_size_v<V>> struct IsVariantMemberType : std::false_type {};

template<typename V, typename T, std::size_t Index>
struct IsVariantMemberType<V, T, Index, false>
    : std::bool_constant<
          std::same_as<T, std::variant_alternative_t<Index, V>> || IsVariantMemberType<V, T, Index + 1>::value> {};

}  // namespace types_internal

// Determine whether a `Type` is any of the types in a `Variant`.
template<typename Variant, typename Type>
concept IsVariantMemberType = types_internal::IsVariantMemberType<Variant, Type, 0>::value;

// Overload handler for `std::visit(std::variant<...>)` and `std::variant::visit`.
//
// Example:
// ```
// std::visit(
//   ::mbo::types::Overloaded{
//     [&](const VariantType0& val) {},
//     [&](const VariantType1& val) {},
//     // ...
//   },
//   variant_value
// )
// ```
template<class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace mbo::types

#endif  // MBO_TYPES_VARIANT_H
