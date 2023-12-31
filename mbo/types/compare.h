// Copyright 2023 M. Boerger (helly25.com)
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

#include <algorithm>
#include <compare>// IWYU pragma: keep
#include <type_traits>

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
  constexpr auto Compare(const value_type& lhs, other_type&& rhs) const noexcept {
    return lhs <=> rhs;
  }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr auto Compare(other_type&& lhs, value_type&& rhs) const noexcept {
    return lhs <=> rhs;
  }

  constexpr bool operator()(const value_type& lhs, const value_type& rhs) const noexcept { return (lhs <=> rhs) < 0; }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr bool operator()(const value_type& lhs, other_type&& rhs) const noexcept {
    return (lhs <=> rhs) < 0;
  }

  template<std::three_way_comparable_with<value_type> other_type>
  requires(!std::same_as<std::remove_cvref_t<value_type>, std::remove_cvref_t<other_type>>)
  constexpr bool operator()(other_type&& lhs, const value_type& rhs) const noexcept {
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

}  // namespace mbo::types

#endif  // MBO_TYPES_COMPARE_H
