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

#ifndef MBO_TYPES_COMPARE_H_
#define MBO_TYPES_COMPARE_H_

#include <cmath>
#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <functional>
#include <limits>
#include <type_traits>

#include "mbo/types/traits.h"  // IWYU pragma: keep

namespace mbo::types {

// Compare less is a three-way comparator object that can be used as a drop-in
// replacement for `std::less`. While it retains the `bool operator(l,r)` that
// performs `less` comparison, it offers a complimentary `Compare` method which
// performs three-way comparison in less order.
template<std::three_way_comparable T>
struct CompareLess {
  using value_type = T;  // NOLINT(readability-identifier-naming)

  constexpr ~CompareLess() noexcept = default;
  constexpr CompareLess() noexcept = default;
  constexpr CompareLess(const CompareLess&) noexcept = default;
  constexpr CompareLess& operator=(const CompareLess&) noexcept = default;
  constexpr CompareLess(CompareLess&&) noexcept = default;
  constexpr CompareLess& operator=(CompareLess&&) noexcept = default;

  constexpr auto Compare(const value_type& lhs, const value_type& rhs) const noexcept { return lhs <=> rhs; }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr auto Compare(const value_type& lhs, const other_type& rhs) const noexcept {
    return lhs <=> rhs;
  }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr auto Compare(const other_type& lhs, const value_type& rhs) const noexcept {
    return lhs <=> rhs;
  }

  constexpr bool operator()(const value_type& lhs, const value_type& rhs) const noexcept { return (lhs <=> rhs) < 0; }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr bool operator()(const value_type& lhs, const other_type& rhs) const noexcept {
    return (lhs <=> rhs) < 0;
  }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr bool operator()(const other_type& lhs, const value_type& rhs) const noexcept {
    return (lhs <=> rhs) < 0;
  }
};

namespace types_internal {

template<typename T>
struct IsCompareLess : std::false_type {};

template<typename T>
struct IsCompareLess<CompareLess<T>> : std::true_type {};

template<typename T>
struct IsStdLess : std::false_type {};

template<typename T>
struct IsStdLess<std::less<T>> : std::true_type {};

}  // namespace types_internal

template<typename T>
concept IsCompareLess = types_internal::IsCompareLess<T>::value;

template<typename T>
concept IsStdLess = types_internal::IsStdLess<T>::value;

template<std::floating_point Double>
std::strong_ordering CompareFloat(Double lhs, Double rhs) {
  const std::partial_ordering comp = lhs <=> rhs;
  if (comp == std::partial_ordering::unordered) {
    const bool lhs_nan = std::isnan(lhs);
    const bool rhs_nan = std::isnan(rhs);
    return lhs_nan <=> rhs_nan;
  } else if (comp == std::partial_ordering::less) {
    return std::strong_ordering::less;
  } else if (comp == std::partial_ordering::greater) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equivalent;
}

// Compares two values that are scalar-numbers (including float/double and pointers, excluding references).
template<IsScalar Lhs, IsScalar Rhs>
inline std::strong_ordering CompareScalar(Lhs lhs, Rhs rhs) {
  if constexpr (std::same_as<Lhs, Rhs>) {
    if constexpr (std::floating_point<Lhs>) {
      return CompareFloat(lhs, rhs);
    } else {
      return lhs <=> rhs;
    }
  } else {
    if constexpr (std::floating_point<Lhs> || std::floating_point<Rhs>) {
      return CompareFloat<long double>(lhs, rhs);
    } else if constexpr (std::same_as<bool, Lhs> || std::same_as<bool, Rhs>) {
      return static_cast<bool>(lhs) <=> static_cast<bool>(rhs);
    } else if constexpr (std::is_signed_v<Lhs> == std::is_signed_v<Rhs>) {
      return lhs <=> rhs;
    } else if constexpr (std::is_signed_v<Lhs>) {
      if (lhs < 0 || rhs > std::numeric_limits<Lhs>::max()) {
        return std::strong_ordering::less;
      } else {
        using Common = std::common_type_t<Lhs, Rhs>;
        return static_cast<Common>(lhs) <=> static_cast<Common>(rhs);
      }
    } else {  // Rhs is signed
      if (rhs < 0 || lhs > std::numeric_limits<Rhs>::max()) {
        return std::strong_ordering::greater;
      } else {
        using Common = std::common_type_t<Lhs, Rhs>;
        return static_cast<Common>(lhs) <=> static_cast<Common>(rhs);
      }
    }
  }
}

// Compares two values that are scalar-numbers (including foat/double, excluding pointers and references).
template<IsArithmetic Lhs, IsArithmetic Rhs>
inline std::strong_ordering CompareArithmetic(Lhs lhs, Rhs rhs) {
  return CompareScalar(lhs, rhs);
}

// Compares two values that are integral-numbers (no float/double, no pointers, no references).
template<IsIntegral Lhs, IsIntegral Rhs>
inline std::strong_ordering CompareIntegral(Lhs lhs, Rhs rhs) {
  return CompareScalar(lhs, rhs);
}

inline std::strong_ordering WeakToStrong(std::weak_ordering order) {
  if (order == std::weak_ordering::less) {
    return std::strong_ordering::less;
  }
  if (order == std::weak_ordering::greater) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equivalent;
}

}  // namespace mbo::types

#endif  // MBO_TYPES_COMPARE_H
