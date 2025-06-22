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

#ifndef MBO_TYPES_INTERNAL_EXTEND_H_
#define MBO_TYPES_INTERNAL_EXTEND_H_

// IWYU pragma: private, include "mbo/types/extend.h"

#include <array>
#include <concepts>  // IWYU pragma: keep
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "mbo/types/extender.h"
#include "mbo/types/internal/extender.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep
#include "mbo/types/tuple.h"   // IWYU pragma: keep

namespace mbo::types::types_internal {

struct NoRequirement {};

// Identify `Extender` classes by the fact that they have a member named
// `GetExtenderName()` that can be converted into a `std::string_view`.
template<typename MaybeExtender>
concept IsExtender = requires {
  { MaybeExtender::GetExtenderName() } -> std::convertible_to<std::string_view>;
};

template<typename T, typename... Ts>
concept TypeInList = (... || (std::is_same_v<T, Ts>));

// Test whether type list `T, Ts...` has duplicates.
template<typename T, typename... Ts>
struct HasDuplicateTypesImpl : std::bool_constant<HasDuplicateTypesImpl<Ts...>::value || TypeInList<T, Ts...>> {};

template<typename T>
struct HasDuplicateTypesImpl<T> : std::false_type {};

template<typename... Ts>
concept HasDuplicateTypes = (sizeof...(Ts) > 1 && HasDuplicateTypesImpl<Ts...>::value);

template<typename Extender>
concept HasRequirement = requires {
  typename Extender::RequiredExtender;
  requires(!std::is_same_v<typename Extender::RequiredExtender, void>);
};

template<typename Extender, bool = HasRequirement<Extender>>
struct GetRequirementImpl {
  using type = void;
};

template<typename Extender>
struct GetRequirementImpl<Extender, true> {
  using type = typename Extender::RequiredExtender;
};

template<typename Extender>
using GetRequirement = typename GetRequirementImpl<Extender>::type;

template<std::size_t N, typename... Ts>
using GetType = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template<std::size_t N, typename Required, typename... Extenders>
struct RequiredPresentForIndexImpl
    : std::bool_constant<
          std::is_same_v<Required, GetType<N, Extenders...>>
          || (N == 0 || RequiredPresentForIndexImpl<N - 1, Required, Extenders...>::value)> {};

template<typename Required, typename... Extenders>
struct RequiredPresentForIndexImpl<0, Required, Extenders...> : std::is_same<Required, GetType<0, Extenders...>> {};

template<std::size_t N, typename... Extenders>
struct RequiredPresentForIndex
    : std::bool_constant<
          (!HasRequirement<GetType<N, Extenders...>>
           || RequiredPresentForIndexImpl<N, GetRequirement<GetType<N, Extenders...>>, Extenders...>::value)
          && (N == 0 || RequiredPresentForIndex<N - 1, Extenders...>::value)> {};

// Do not allow first type to require itself.
template<typename... Extenders>
struct RequiredPresentForIndex<0, Extenders...> : std::bool_constant<!HasRequirement<GetType<0, Extenders...>>> {};

template<typename... Extenders>
concept AllRequiredPresent =
    sizeof...(Extenders) == 0 || RequiredPresentForIndex<sizeof...(Extenders) - 1, Extenders...>::value;

// Verify a tuple of expanded extenders is valid:
template<typename ExtenderTuple>
struct ExtenderTupleValid;

template<typename... Extender>
struct ExtenderTupleValid<std::tuple<Extender...>>
    : std::bool_constant<(
          IsExtender<Extender> && ... && (!HasDuplicateTypes<Extender...> && AllRequiredPresent<Extender...>))> {};

template<typename T>
concept HasExtenderTuple = requires { typename T::ExtenderTuple; };

// Expand an element of a extender tuple. A non tuple element becomes a tuple of the element.
// There is also the special case of a shorthand extender which has a `ExtenderTuple` member type.
template<typename T, bool = HasExtenderTuple<T>>
struct ExtendExtenderTupleElemT {
  using type = typename T::ExtenderTuple;
};

template<typename T>
struct ExtendExtenderTupleElemT<T, false> {
  using type = std::tuple<T>;
};

template<typename T>
struct ExtendExtenderTupleElem {
  using type = ExtendExtenderTupleElemT<T>::type;
};

// A tuple just is the tuple.
template<typename... T>
struct ExtendExtenderTupleElem<std::tuple<T...>> {
  using type = std::tuple<T...>;
};

// Expand an actual given extender tuple
template<typename T>
struct ExtendExtenderTuple;

// We expand the first tuple element, so if it is a tuple we keep that and otherwise we make the
// element a tuple. That way the first element given to TupleCat is always a tuple.
// The remainder we always recursively, so stripping one off at a time.
// That way we push expanded to the left while processing to the left.
// The final result is a tuple where possible direct tuple members have been inlined aka expanded.
template<typename T, typename... U>
struct ExtendExtenderTuple<std::tuple<T, U...>> {
  using type = mbo::types::
      TupleCat<typename ExtendExtenderTupleElem<T>::type, typename ExtendExtenderTuple<std::tuple<U...>>::type>;
};

template<>
struct ExtendExtenderTuple<std::tuple<>> {
  using type = std::tuple<>;
};

template<typename ExtenderTuple>
using ExtendExtenderTupleT = ExtendExtenderTuple<ExtenderTuple>::type;

// Check whether the expanded `ExtenderList` is valid.
template<typename... ExtenderList>
concept ExtenderListValid = ExtenderTupleValid<ExtendExtenderTupleT<std::tuple<ExtenderList...>>>::value;

// Check whether `Extended` has `Extender`.
template<typename Extender, typename Extended>
struct HasExtenderImpl;

template<typename Extender, typename... ExtenderList>
struct HasExtenderImpl<Extender, std::tuple<ExtenderList...>>
    : std::bool_constant<(std::is_same_v<Extender, std::remove_cvref_t<ExtenderList>> || ...)> {};

// Determine whether type `Extended` is an `Extend`ed type that has been
// extended with `Extender`.
template<typename Extended, typename Extender>
concept HasExtender = IsExtended<Extended>
                      && HasExtenderImpl<std::remove_cvref_t<Extender>, typename Extended::RegisteredExtenders>::value;

// Build CRTP chains from the Extender list.
template<typename Base, typename Extender, typename... MoreExtender>
struct ExtendBuildChain : ExtendBuildChain<mbo::types::types_internal::UseExtender<Base, Extender>, MoreExtender...> {};

template<typename T>
struct ExtendBuildChain<T, void> : T {};

}  // namespace mbo::types::types_internal

namespace mbo::extender {

using types::types_internal::ExtenderListValid;

template<typename T, typename ExtenderList>
struct Extend;

template<typename T, typename... ExtenderList>
requires(ExtenderListValid<ExtenderList...>)
struct Extend<T, std::tuple<ExtenderList...>>
    : ::mbo::types::types_internal::ExtendBuildChain<
          mbo::types::ExtendBase<T>,  // CRTP functionality injection.
          ExtenderList...,  // NOT extended, so that shorthand forms `Default` and `NoPrint` result in short names.
          void /* Sentinel to stop `ExtendBuildChain` */> {
  using RegisteredExtenders = mbo::types::types_internal::ExtendExtenderTupleT<std::tuple<ExtenderList...>>;
  using UnexpandedExtenders = std::tuple<ExtenderList...>;

  static constexpr std::array<std::string_view, std::tuple_size_v<RegisteredExtenders>> RegisteredExtenderNames() {
    return RegisteredExtenderNamesIS(std::make_index_sequence<std::tuple_size_v<RegisteredExtenders>>{});
  }

 private:
  template<std::size_t... Idx>
  static constexpr std::array<std::string_view, std::tuple_size_v<RegisteredExtenders>> RegisteredExtenderNamesIS(
      std::index_sequence<Idx...> /*unused*/) {
    return {(std::tuple_element_t<Idx, RegisteredExtenders>::GetExtenderName())...};
  }
};

}  // namespace mbo::extender

#endif  // MBO_TYPES_INTERNAL_EXTEND_H_
