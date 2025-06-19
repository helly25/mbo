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

#ifndef MBO_TYPES_OPTIONAL_REF_H_
#define MBO_TYPES_OPTIONAL_REF_H_

#include <compare>
#include <concepts>
#include <optional>
#include <type_traits>

#include "absl/hash/hash.h"
#include "absl/strings/str_format.h"
#include "mbo/config/require.h"

namespace mbo::types {

template<typename Rhs, typename Lhs>
concept AssignableTo = std::assignable_from<Lhs, Rhs>;

template<typename T>
requires(!std::is_reference_v<T>)
class OptionalRef {
 public:
  using value_type = T&;             // NOLINT(*-identifier-naming)
  using reference = T&;              // NOLINT(*-identifier-naming)
  using const_reference = const T&;  // NOLINT(*-identifier-naming)
  using pointer = T*;                // NOLINT(*-identifier-naming)
  using const_pointer = const T*;    // NOLINT(*-identifier-naming)

  constexpr ~OptionalRef() noexcept = default;

  constexpr OptionalRef() noexcept = default;

  constexpr OptionalRef(reference v) : v_(&v) {}  // NOLINT(*-explicit-*)

  constexpr OptionalRef(std::nullopt_t /*unused */) : v_(nullptr) {}  // NOLINT(*-explicit-*)

  constexpr OptionalRef(const OptionalRef&) noexcept = default;
  constexpr OptionalRef& operator=(const OptionalRef&) = delete;
  constexpr OptionalRef(OptionalRef&&) noexcept = default;
  constexpr OptionalRef& operator=(OptionalRef&&) = delete;

  constexpr OptionalRef& reset() noexcept {  // NOLINT(*-identifier-naming)
    v_ = nullptr;
    return *this;
  }

  constexpr OptionalRef& set_ref(T& v) noexcept {  // NOLINT(*-identifier-naming)
    v_ = &v;
    return *this;
  }

  constexpr bool has_value() const noexcept { return v_ != nullptr; }  // NOLINT(*-identifier-naming)

  constexpr explicit operator bool() const noexcept { return has_value(); }

  constexpr reference value() noexcept {  // NOLINT(*-identifier-naming)
    MBO_CONFIG_REQUIRE(has_value(), "Value is not set.");
    return *v_;
  }

  constexpr const_reference value() const noexcept {  // NOLINT(*-identifier-naming)
    MBO_CONFIG_REQUIRE(has_value(), "Value is not set.");
    return *v_;
  }

  constexpr reference operator*() noexcept { return value(); }

  constexpr const_reference operator*() const noexcept { return value(); }

  constexpr pointer operator->() noexcept { return &value(); }

  constexpr const_pointer operator->() const noexcept { return &value(); }

  template<AssignableTo<T&> U = T>
  constexpr OptionalRef& operator=(U&& v) noexcept {
    value() = std::forward<U>(v);
    return *this;
  }

  template<AssignableTo<T&> U = T>
  constexpr OptionalRef& operator=(const U& v) noexcept {
    value() = v;
    return *this;
  }

  template<std::equality_comparable_with<T> U = T>
  constexpr bool operator==(const OptionalRef<U>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return false;
    }
    if (!has_value()) {
      return true;
    }
    return value() == rhs.value();
  }

  template<std::totally_ordered_with<T> U = T>
  constexpr bool operator<(const OptionalRef<U>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return !has_value();
    }
    if (!has_value()) {
      return false;
    }
    return value() < rhs.value();
  }

  template<std::three_way_comparable_with<T> U = T>
  constexpr auto operator<=>(const OptionalRef<U>& rhs) const {
    using Result = decltype(std::declval<T>() <=> std::declval<T>());
    if (Result result = has_value() <=> rhs.has_value(); result != Result::equal) {
      return result;
    }
    if (!has_value()) {
      return Result::equal;
    }
    return value() <=> rhs.value();
  }

  constexpr bool operator==(std::nullopt_t /*unused*/) const noexcept { return v_ == nullptr; }

  constexpr bool operator<(std::nullopt_t /*unused*/) const noexcept { return v_ < nullptr; }

  constexpr std::strong_ordering operator<=>(std::nullopt_t /*unused*/) const noexcept {
    if (!has_value()) {
      return std::strong_ordering::equal;
    }
    return std::strong_ordering::greater;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalRef<T>> && std::equality_comparable_with<T, U>)
  constexpr bool operator==(const U& other) const noexcept {
    if (!has_value()) {
      return false;
    }
    return value() == other;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalRef<T>> && std::totally_ordered_with<T, U>)
  constexpr bool operator<(const U& other) const noexcept {
    if (!has_value()) {
      return true;
    }
    return value() < other;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalRef<T>> && std::three_way_comparable_with<T, U>)
  constexpr auto operator<=>(const U& other) const noexcept {
    using Result = decltype(*v_ <=> other);
    if (!has_value()) {
      return Result::less;
    }
    return value() <=> other;
  }

  template<typename H>
  friend constexpr H AbslHashValue(H hash, const OptionalRef<T>& v) {
    if (v.has_value()) {
      return H::combine(std::move(hash), absl::HashOf<>(v.value()));
    } else {
      return H::combine(std::move(hash), absl::HashOf<>(std::nullopt));
    }
  }

  template<typename Sink>
  friend constexpr void AbslStringify(Sink& sink, const OptionalRef<T>& v) {
    if (v.has_value()) {
      absl::Format(&sink, "%v", v.value());
    } else {
      absl::Format(&sink, "std::nullopt");
    }
  }

 private:
  T* v_ = nullptr;
};

}  // namespace mbo::types

#endif  // MBO_TYPES_OPTIONAL_REF_H_
