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

#ifndef MBO_TYPES_INTERNAL_IS_BRACES_CONSTRUCTIBLE_H_
#define MBO_TYPES_INTERNAL_IS_BRACES_CONSTRUCTIBLE_H_

// IWYU pragma: private, include "mbo/types/traits.h"

#include <type_traits>

namespace mbo::types::types_internal {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

struct AnyType {
  template<typename T>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex>
using AnyTypeN = AnyType;

template<typename D>
struct AnyBaseType {
  using RawD = std::remove_cvref_t<D>;
  template<typename T, typename = std::enable_if_t<std::is_base_of_v<std::remove_const_t<T>, RawD>>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<typename D>
struct AnyNonBaseType {
  using RawD = std::remove_cvref_t<D>;
  template<typename T, typename = std::enable_if_t<!std::is_base_of_v<std::remove_const_t<T>, RawD>>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex, typename D>
using AnyNonBaseTypeN = AnyNonBaseType<D>;

template<typename D, bool kBaseOrNot, bool kRequireNonEmpty, bool kAllowNonEmpty = kRequireNonEmpty>
struct AnyTypeIf {
  using RawD = std::remove_cvref_t<D>;
  template<
      typename T,
      typename = std::enable_if_t<                                         //
          (kBaseOrNot == std::is_base_of_v<std::remove_cvref_t<T>, RawD>)  //
          &&(!kBaseOrNot || !kRequireNonEmpty || (!std::is_empty_v<std::remove_cvref_t<T>> && kAllowNonEmpty))>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex, typename D, bool kBaseOrNot, bool kRequireNonEmpty, bool kAllowNonEmpty = kRequireNonEmpty>
using AnyTypeIfN = AnyTypeIf<D, kBaseOrNot, kRequireNonEmpty, kAllowNonEmpty>;

template<typename D>
struct AnyEmptyBase {
  using RawD = std::remove_cvref_t<D>;
  template<
      typename T,
      typename =
          std::enable_if_t<std::is_base_of_v<std::remove_cvref_t<T>, RawD> && std::is_empty_v<std::remove_cvref_t<T>>>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex, typename D>
using AnyEmptyBaseN = AnyEmptyBase<D>;

template<typename D>
struct AnyEmptyBaseOrNonBase {
  using RawD = std::remove_cvref_t<D>;
  template<
      typename T,
      typename =
          std::enable_if_t<!std::is_base_of_v<std::remove_cvref_t<T>, RawD> || std::is_empty_v<std::remove_cvref_t<T>>>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex, typename D>
using AnyEmptyBaseOrNonBaseN = AnyEmptyBaseOrNonBase<D>;

template<typename D, bool kIsEmpty, bool kAllowNonEmpty = false>
struct AnyBaseMaybeEmpty {
  using RawD = std::remove_cvref_t<D>;
  template<
      typename T,
      typename = std::enable_if_t<
          std::is_base_of_v<std::remove_cvref_t<T>, RawD>
          && (kIsEmpty == std::is_empty_v<std::remove_cvref_t<T>>
              || (kAllowNonEmpty && !std::is_empty_v<std::remove_cvref_t<T>>))>>
  constexpr operator T() const noexcept;  // NOLINT(*-explicit-constructor, *-explicit-conversions)
};

template<std::size_t kIndex, typename D, bool kIsEmpty, bool kAllowNonEmpty = false>
using AnyBaseMaybeEmptyN = AnyBaseMaybeEmpty<D, kIsEmpty, kAllowNonEmpty>;

template<typename T, typename... Args>
inline decltype(void(T{std::declval<Args>()...}), std::true_type()) IsBracesConstructibleFunc(int);

template<typename T, typename... Args>
inline std::false_type IsBracesConstructibleFunc(...);

template<typename T, typename... Args>
struct IsBracesConstructibleImpl : decltype(types_internal::IsBracesConstructibleFunc<T, Args...>(0)) {};

template<typename T, typename... Args>
using IsBracesConstructibleImplT = typename IsBracesConstructibleImpl<T, Args...>::type;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_IS_BRACES_CONSTRUCTIBLE_H_
