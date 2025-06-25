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
#include <concepts>  // IWYU pragma: keep
#include <memory>
#include <optional>
#include <type_traits>

#include "absl/hash/hash.h"
#include "absl/strings/str_format.h"
#include "mbo/config/require.h"
#include "mbo/log/demangle.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep

namespace mbo::types {

// NOLINTBEGIN(*-identifier-naming)

template<typename T, typename RefT = T>
requires(
    !std::is_reference_v<T> && !std::is_reference_v<RefT>
    && std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<RefT>>)
class OptionalDataOrRef {
 public:
  using value_type = T;
  using pointer = RefT*;
  using const_pointer = const RefT*;
  using reference = RefT&;
  using const_reference = const RefT&;

  constexpr ~OptionalDataOrRef() noexcept { Destruct(); }

  constexpr OptionalDataOrRef() noexcept {}  // NOLINT(hicpp-use-equals-default): Not the same

  constexpr OptionalDataOrRef(std::nullopt_t /* unused */) {}  // NOLINT(*-explicit-*)

  template<typename U = T>
  requires(!std::same_as<U, OptionalDataOrRef>)
  constexpr OptionalDataOrRef(T&& v) {  // NOLINT(*-explicit-*)
    emplace(std::move(v));
  }

  constexpr OptionalDataOrRef(RefT& v)  // NOLINT(*-explicit-*)
  requires(!std::same_as<T, RefT>)
  {
    set_ref(v);
  }

  template<typename U = RefT>
  requires(
      !std::is_rvalue_reference_v<U> && !std::same_as<U, OptionalDataOrRef>
      && (!std::is_const_v<U> || std::is_const_v<RefT>))
  constexpr OptionalDataOrRef(U& v) {  // NOLINT(*-explicit-*)
    set_ref(v);
  }

  constexpr OptionalDataOrRef(const OptionalDataOrRef& other) noexcept
  requires std::copy_constructible<T>
  {
    if (other.is_val_) {
      emplace(other.union_.val);
    } else {
      Destruct(other.union_.ptr);
    }
  }

  constexpr OptionalDataOrRef& operator=(const OptionalDataOrRef& other)
  requires std::copy_constructible<T>
  {
    if (this == &other) {
      return *this;
    }
    if (other.is_val_) {
      emplace(other.union_.val);
    } else {
      Destruct(other.union_.ptr);
    }
    return *this;
  }

  constexpr OptionalDataOrRef(OptionalDataOrRef&& other) noexcept
  requires std::move_constructible<T>
  {
    if (other.is_val_) {
      emplace(std::move(other.union_.val));
      other.is_val_ = false;
      other.union_.ptr = nullptr;
    } else {
      Destruct(other.union_.ptr);
    }
  }

  constexpr OptionalDataOrRef& operator=(OptionalDataOrRef&& other)
  requires std::move_constructible<T>
  {
    if (other.is_val_) {
      emplace(std::move(other.union_.val));
      other.is_val_ = false;
      other.union_.ptr = nullptr;
    } else {
      Destruct(other.union_.ptr);
    }
    return *this;
  }

  constexpr OptionalDataOrRef& operator=(std::nullopt_t /* unused */) { reset(); }

  constexpr OptionalDataOrRef& operator=(value_type&& v) {
    emplace(std::move(v));
    return *this;
  }

  template<typename U>
  requires(std::assignable_from<T&, U> && std::constructible_from<T, U> && !std::same_as<U, T>)
  constexpr OptionalDataOrRef& operator=(U&& v) noexcept {
    if constexpr (std::is_rvalue_reference_v<decltype(v)>) {  // NOLINT(*-branch-clone)
      emplace(std::forward<U>(v));
    } else if (has_value()) {
      value() = std::forward<U>(v);
    } else {
      emplace(std::forward<U>(v));
    }
    return *this;
  }

  constexpr OptionalDataOrRef& reset() noexcept {
    Destruct();
    return *this;
  }

  constexpr OptionalDataOrRef& set_ref(reference v) noexcept {
    Destruct(&v);
    return *this;
  }

  template<typename... Args, typename = std::enable_if_t<ConstructibleFrom<T, Args...>>>
  constexpr OptionalDataOrRef& emplace(Args&&... args) noexcept {
    if (is_val_) {
      std::destroy_at(&union_.val);
    }
    std::construct_at(&union_.val, std::forward<Args>(args)...);
    is_val_ = true;
    return *this;
  }

  constexpr explicit operator bool() const noexcept { return has_value(); }

  constexpr bool has_value() const noexcept { return is_val_ || union_.ptr != nullptr; }

  constexpr bool HoldsData() const noexcept { return is_val_; }

  constexpr bool HoldsNullopt() const noexcept { return !is_val_ && union_.ptr == nullptr; }

  constexpr bool HoldsReference() const noexcept { return !is_val_ && union_.ptr != nullptr; }

  constexpr reference value() noexcept {
    MBO_CONFIG_REQUIRE(has_value(), "No value set for: ") << mbo::log::DemangleV(*this);
    return is_val_ ? union_.val : *union_.ptr;
  }

  constexpr const_reference value() const noexcept {
    MBO_CONFIG_REQUIRE(has_value(), "No value set for: ") << mbo::log::DemangleV(*this);
    return is_val_ ? union_.val : *union_.ptr;
  }

#if __cplusplus >= 202'302L
  // Returns `value()` if `holds_value()` is true, a reference to static defaults otherwise.
  constexpr const_reference get() const noexcept
  requires std::is_default_constructible_v<T>
  {
    static constexpr T kDefaults{};
    return has_value() ? value() : kDefaults;
  }
#endif  // __cplusplus >= 202302L

  // Returns `value()` if `holds_value()` is true, a reference `defaults`.
  // BEWARE of dangling references.
  constexpr const_reference get(const T& defaults) const noexcept { return has_value() ? value() : defaults; }

  // Returns a reference to existing data or created data. If the object:
  // * is `std::nullopt`, then a default value will be emplace and is reference returned.
  // * contains a value, then its reference will be returned.
  // * contains a reference, then that reference is emplace and then its reference returned.
  template<typename... Args, typename = std::enable_if_t<ConstructibleFrom<T, Args...>>>
  constexpr value_type& as_data(Args&&... args) noexcept {
    if (!is_val_) {
      if (union_.ptr != nullptr) {
        emplace(*union_.ptr);
      } else {
        emplace(std::forward<Args>(args)...);
      }
    }
    return union_.val;
  }

  constexpr reference operator*() noexcept { return value(); }

  constexpr const_reference operator*() const noexcept { return value(); }

  constexpr pointer operator->() noexcept { return &value(); }

  constexpr const_pointer operator->() const noexcept { return &value(); }

  template<std::equality_comparable_with<T> U = T, typename RefU = U>
  constexpr bool operator==(const OptionalDataOrRef<U, RefU>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return false;
    }
    if (!has_value()) {
      return true;
    }
    return value() == rhs.value();
  }

  template<std::totally_ordered_with<T> U = T, typename RefU = U>
  constexpr bool operator<(const OptionalDataOrRef<U, RefU>& rhs) const noexcept {
    if (has_value() != rhs.has_value()) {
      return !has_value();
    }
    if (!has_value()) {
      return false;
    }
    return value() < rhs.value();
  }

  template<std::three_way_comparable_with<T> U = T, typename RefU = U>
  constexpr auto operator<=>(const OptionalDataOrRef<U, RefU>& rhs) const {
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
    using Result = decltype(union_.val <=> other);
    if (!has_value()) {
      return Result::less;
    }
    return value() <=> other;
  }

  template<typename H>
  friend constexpr H AbslHashValue(H hash, const OptionalDataOrRef<T, RefT>& v) {
    if (v.has_value()) {
      return H::combine(std::move(hash), absl::HashOf<>(v.value()));
    } else {
      return H::combine(std::move(hash), absl::HashOf<>(std::nullopt));
    }
  }

  template<typename Sink>
  friend constexpr void AbslStringify(Sink& sink, const OptionalDataOrRef<T, RefT>& v) {
    if (v.has_value()) {
      absl::Format(&sink, "%v", v.value());
    } else {
      absl::Format(&sink, "std::nullopt");
    }
  }

 private:
  // In C++20 some implementtions of `variant` are not (fully) `constexpr` compliant.
  // std::variant<OptionalRefT, T> v_ = OptionalRefT(std::nullopt);
  // So instead we go with a manual implementation.

  constexpr void DestructOnly() noexcept {
    if (is_val_) {
      std::destroy_at(&union_.val);
    }
  }

  constexpr void Destruct(RefT* ptr = nullptr) noexcept {
    DestructOnly();
    union_.ptr = ptr;
    is_val_ = false;
  }

  union Data {
    constexpr ~Data() noexcept { /* Nothing to do: Owner deletes */ }

    constexpr Data() noexcept : ptr(nullptr) {}

    constexpr Data(const Data& /*unused*/) noexcept {}

    constexpr Data& operator=(const Data& /*unused*/) noexcept { return *this; }  // NOLINT(cert-oop54-cpp,*unhandled*)

    constexpr Data(Data&& /*unused*/) noexcept {}

    constexpr Data& operator=(Data&& /*unused*/) noexcept { return *this; }

    RefT* ptr;
    T val;
  };

  Data union_;
  bool is_val_ = false;
};

template<typename T>
using OptionalDataOrConstRef = OptionalDataOrRef<T, const T>;

template<typename T>
concept IsOptionalDataOrRef = requires {
  typename T::value_type;
  typename T::reference;
  requires std::same_as<T, OptionalDataOrRef<typename T::value_type, std::remove_reference_t<typename T::reference>>>;
};

// NOLINTEND(*-identifier-naming)

}  // namespace mbo::types

#endif  // MBO_TYPES_OPTIONAL_DATA_OR_REF_H_
