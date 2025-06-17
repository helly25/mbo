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

#ifndef MBO_TYPES_OPTIONAL_DATA_OR_REF_H_
#define MBO_TYPES_OPTIONAL_DATA_OR_REF_H_

#include <compare>
#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>

#include "absl/hash/hash.h"
#include "absl/strings/str_format.h"
#include "mbo/types/optional_ref.h"

namespace mbo::types {

template<typename T, typename RefT = T>
requires(
    !std::is_reference_v<T> && !std::is_reference_v<RefT>
    && std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<RefT>>)
class OptionalDataOrRef {
 private:
  using OptionalRefT = OptionalRef<RefT>;

 public:
  using value_type = T;                 // NOLINT(*-identifier-naming)
  using pointer = T*;                   // NOLINT(*-identifier-naming)
  using const_pointer = const T*;       // NOLINT(*-identifier-naming)
  using reference = RefT&;              // NOLINT(*-identifier-naming)
  using const_reference = const RefT&;  // NOLINT(*-identifier-naming)

  constexpr ~OptionalDataOrRef() noexcept = default;

  constexpr OptionalDataOrRef() noexcept = default;

  constexpr OptionalDataOrRef(std::nullopt_t /* unused */) : v_(std::nullopt) {}  // NOLINT(*-explicit-*)

  template<typename U = RefT>
  requires(!std::is_rvalue_reference_v<U>)
  constexpr OptionalDataOrRef(U& v) {  // NOLINT(*-explicit-*)
    set_ref(v);
  }

  template<typename U = T>
  requires(std::is_rvalue_reference_v<U>)
  constexpr OptionalDataOrRef(U&& v) {  // NOLINT(*-explicit-*)
    emplace(std::forward<U>(v));
  }

  constexpr OptionalDataOrRef(const OptionalDataOrRef&) noexcept = default;
  constexpr OptionalDataOrRef& operator=(const OptionalDataOrRef&) = delete;
  constexpr OptionalDataOrRef(OptionalDataOrRef&&) noexcept = default;
  constexpr OptionalDataOrRef& operator=(OptionalDataOrRef&&) = delete;

  constexpr OptionalDataOrRef& operator=(std::nullopt_t /* unused */) { reset(); }

  constexpr OptionalDataOrRef& operator=(value_type&& v) {
    emplace(std::move(v));
    return *this;
  }

  template<typename U>
  requires(AssignableTo<U, T&> && std::constructible_from<T, U>)
  constexpr OptionalDataOrRef& operator=(U&& v) noexcept {
    if constexpr (std::is_rvalue_reference_v<decltype(v)>) {
      emplace(std::forward<U>(v));
    } else if (has_value()) {
      value() = std::forward<U>(v);
    } else {
      as_data() = std::forward<U>(v);
    }
    return *this;
  }

  constexpr OptionalDataOrRef& reset() noexcept {  // NOLINT(*-identifier-naming)
    v_.template emplace<OptionalRefT>().reset();
    return *this;
  }

  constexpr OptionalDataOrRef& set_ref(reference v) noexcept {  // NOLINT(*-identifier-naming)
    v_.template emplace<OptionalRefT>().set_ref(v);
    return *this;
  }

  template<typename... Args>
  constexpr OptionalDataOrRef& emplace(Args&&... args) noexcept {  // NOLINT(*-identifier-naming)
    v_.template emplace<T>(std::forward<Args>(args)...);
    return *this;
  }

  constexpr explicit operator bool() const noexcept { return has_value(); }

  constexpr bool has_value() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<T>(v_) || std::get<OptionalRefT>(v_).has_value();
  }

  constexpr bool HoldsData() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<T>(v_);
  }

  constexpr bool HoldsNullopt() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<OptionalRefT>(v_) && !std::get<OptionalRefT>(v_).has_value();
  }

  constexpr bool HoldsReference() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<OptionalRefT>(v_) && std::get<OptionalRefT>(v_).has_value();
  }

  constexpr reference value() noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<T>(v_) ? std::get<T>(v_) : std::get<OptionalRefT>(v_).value();
  }

  constexpr const_reference value() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<T>(v_) ? std::get<T>(v_) : std::get<OptionalRefT>(v_).value();
  }

  // Returns a reference to existing data or created data. If the object:
  // * is `std::nullopt`, then a default value will be emplace and is reference returned.
  // * contains a value, then its reference will be returned.
  // * contains a reference, then that reference is emplace and then its reference returned.
  constexpr value_type& as_data() noexcept {  // NOLINT(*-identifier-naming)
    if (!std::holds_alternative<T>(v_)) {
      const OptionalRefT& ref = std::get<OptionalRefT>(v_);
      if (ref.has_value()) {
        emplace(ref.value());
      } else {
        emplace();
      }
    }
    return std::get<T>(v_);
  }

  constexpr reference operator*() noexcept {
    return std::holds_alternative<T>(v_) ? std::get<T>(v_) : std::get<OptionalRefT>(v_).value();
  }

  constexpr const_reference operator*() const noexcept {
    return std::holds_alternative<T>(v_) ? std::get<T>(v_) : std::get<OptionalRefT>(v_).value();
  }

  constexpr pointer operator->() noexcept {
    return std::holds_alternative<T>(v_) ? &std::get<T>(v_) : &std::get<OptionalRefT>(v_).value();
  }

  constexpr const_pointer operator->() const noexcept {
    return std::holds_alternative<T>(v_) ? &std::get<T>(v_) : &std::get<OptionalRefT>(v_).value();
  }

  template<std::equality_comparable_with<T> U = T>
  constexpr bool operator==(const OptionalDataOrRef<U>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return false;
    }
    if (!has_value()) {
      return true;
    }
    return value() == rhs.value();
  }

  template<std::totally_ordered_with<T> U = T>
  constexpr bool operator<(const OptionalDataOrRef<U>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return !has_value();
    }
    if (!has_value()) {
      return false;
    }
    return value() < rhs.value();
  }

  template<std::three_way_comparable_with<T> U = T>
  constexpr auto operator<=>(const OptionalDataOrRef<U>& rhs) const {
    using Result = decltype(std::declval<T>() <=> std::declval<T>());
    if (Result result = has_value() <=> rhs.has_value(); result != Result::equal) {
      return result;
    }
    if (!has_value()) {
      return Result::equal;
    }
    return value() <=> rhs.value();
  }

  constexpr bool operator==(std::nullopt_t /*unused*/) const noexcept { return !has_value(); }

  constexpr bool operator<(std::nullopt_t /*unused*/) const noexcept { return false; }

  constexpr std::strong_ordering operator<=>(std::nullopt_t /*unused*/) const noexcept {
    if (!has_value()) {
      return std::strong_ordering::equal;
    }
    return std::strong_ordering::greater;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalDataOrRef<T, RefT>> && std::equality_comparable_with<T, U>)
  constexpr bool operator==(const U& other) const noexcept {
    if (!has_value()) {
      return false;
    }
    return value() == other;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalDataOrRef<T, RefT>> && std::totally_ordered_with<T, U>)
  constexpr bool operator<(const U& other) const noexcept {
    if (!has_value()) {
      return true;
    }
    return value() < other;
  }

  template<typename U = T>
  requires(!std::same_as<U, OptionalDataOrRef<T, RefT>> && std::three_way_comparable_with<T, U>)
  constexpr auto operator<=>(const U& other) const noexcept {
    using Result = decltype(*v_ <=> other);
    if (!has_value()) {
      return Result::less;
    }
    return value() <=> other;
  }

  template<typename H>
  friend constexpr H AbslHashValue(H hash, const OptionalDataOrRef<T>& v) {
    if (v.has_value()) {
      return H::combine(std::move(hash), absl::HashOf<>(v.value()));
    } else {
      return H::combine(std::move(hash), absl::HashOf<>(std::nullopt));
    }
  }

  template<typename Sink>
  friend constexpr void AbslStringify(Sink& sink, const OptionalDataOrRef<T>& v) {
    if (v.has_value()) {
      absl::Format(&sink, "%v", v.value());
    } else {
      absl::Format(&sink, "std::nullopt");
    }
  }

 private:
  std::variant<OptionalRefT, T> v_ = OptionalRefT(std::nullopt);
};

template<typename T>
using OptionalDataOrConstRef = OptionalDataOrRef<T, const T>;

}  // namespace mbo::types

#endif  // MBO_TYPES_OPTIONAL_DATA_OR_REF_H_
