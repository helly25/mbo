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

#ifndef MBO_TYPES_INTERNAL_EXTEND_H_
#define MBO_TYPES_INTERNAL_EXTEND_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "mbo/types/extender.h"
#include "mbo/types/internal/extender.h"
#include "mbo/types/tuple.h"  // IWYU pragma: keep

namespace mbo::types_internal {

struct NoRequirement {};

// Identify `Extender` classes by the fact that they have a member named
// `kExtenderName` that can be converted into a `std::string_view`.
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

template<typename... Extender>
concept ExtenderListValid =
    (IsExtender<Extender> && ... && (!HasDuplicateTypes<Extender...> && AllRequiredPresent<Extender...>));

template<typename... Extenders>
concept HasDefaultExtender = (std::is_same_v<Extenders, ::mbo::types::extender::Default> || ...);

// Build CRTP chains from the Extender list.
template<typename Base, typename Extender, typename... MoreExtender>
struct ExtendBuildChain
    : ExtendBuildChain<typename ::mbo::types::types_internal::UseExtender<Base, Extender>::type, MoreExtender...> {};

template<typename T>
struct ExtendBuildChain<T, void> : T {};

// Determine whether type `Extended` is an `Extend`ed type.
template<typename Extended>
concept IsExtended = requires { typename Extended::RegisteredExtenders; };

template<std::size_t N, typename Extended, typename Extender>
struct HasExtenderImpl
    : std::bool_constant<
          std::is_same_v<
              std::remove_cvref_t<std::tuple_element_t<N - 1, typename Extended::RegisteredExtenders>>,
              std::remove_cvref_t<Extender>>
          || HasExtenderImpl<N - 1, Extended, Extender>::value> {};

template<typename Extended, typename Extender>
struct HasExtenderImpl<0, Extended, Extender> : std::false_type {};

// Determine whether type `Extended` is an `Extend`ed type that has been
// extended with `Extender`.
template<typename Extended, typename Extender>
concept HasExtender =
    IsExtended<Extended> && std::tuple_size_v<typename Extended::RegisteredExtenders> != 0
    && HasExtenderImpl<std::tuple_size_v<typename Extended::RegisteredExtenders>, Extended, Extender>::value;

}  // namespace mbo::types_internal

namespace mbo::extender {

// Base Extender implementation for CRTP functionality injection.
// This must always be present.
template<typename ActualType>
struct ExtendBase {
 public:
  using Type = ActualType;  // Used for type chaining in `UseExtender`

 protected:  // DO NOT expose anything publicly.
  auto ToTuple() const { return StructToTuple(static_cast<const ActualType&>(*this)); }

 private:  // DO NOT expose anything publicly.
  template<typename U, typename Extender>
  friend struct UseExtender;
};

template<typename T, typename... ExtenderList>
requires ::mbo::types_internal::ExtenderListValid<ExtenderList...>
struct Extend
    : ::mbo::types_internal::ExtendBuildChain<
          ExtendBase<T>,  // CRTP functionality injection.
          ExtenderList...,
          void /* Sentinel to stop `ExtendBuildChain` */> {
  using RegisteredExtenders = std::conditional_t<
      ::mbo::types_internal::HasDefaultExtender<ExtenderList...>,
      std::tuple<
          // The list of default extenders must be in sync with the implementation of `types::Default`.
          ::mbo::types::extender::AbslStringify,
          ::mbo::types::extender::AbslHashable,
          ::mbo::types::extender::Comparable,
          ::mbo::types::extender::Printable,
          ::mbo::types::extender::Streamable,
          // Actual provided extenders go last:
          ExtenderList...>,
      std::tuple<ExtenderList...>>;

  static constexpr std::array<std::string_view, std::tuple_size_v<RegisteredExtenders>> RegisteredExtenderNames() {
    if constexpr (::mbo::types_internal::HasDefaultExtender<ExtenderList...>) {
      return {
          // The list of default extenders must be in sync with the implementation of `types::Default`.
          ::mbo::types::extender::AbslStringify::GetExtenderName(),
          ::mbo::types::extender::AbslHashable::GetExtenderName(),
          ::mbo::types::extender::Comparable::GetExtenderName(),
          ::mbo::types::extender::Printable::GetExtenderName(),
          ::mbo::types::extender::Streamable::GetExtenderName(),
          // Actual provided extenders go last:
          ExtenderList::GetExtenderName()...,
      };
    } else {
      return {ExtenderList::GetExtenderName()...};
    }
  }
};

}  // namespace mbo::extender

#endif  // MBO_TYPES_INTERNAL_EXTEND_H_
