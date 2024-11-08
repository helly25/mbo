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

#ifndef MBO_TYPES_REQUIRED_H_
#define MBO_TYPES_REQUIRED_H_

#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <utility>

namespace mbo::types {

// Template class `Required<T>` is a wrapper around a type `T`. The value can be
// replaced without using assign or move-assign operators. Instead the wrapper
// will call the destructor and then in-place construct the wrapper value using
// the type's move constructor.
//
// In other words the reson for this type is he rare case where a type `T` that
// follows the rule of 0/3/5 and of the 4 ctor/assignments only allows for
// move-construction all while (say a test) needs to be implemented with assign.
//
// The type is similar to `std::optional` only it cannot be `std::nullopt`.
// The type is however closer to `RefWrap`.
//
// The wrapper is not implemented itself as a wrapper or specialization of STL
// type `std::optional` due to its overhead.
template<typename T>
class Required {
 public:
  using type = T;

  Required() = default;

  constexpr explicit Required(T v) : value_(std::move(v)) {}

  template<typename U>
  requires(std::constructible_from<T, U> && !std::same_as<T, U>)
  constexpr explicit Required(U&& v) : value_(std::forward<U>(v)) {}

  template<typename... Args>
  requires(std::constructible_from<T, Args...>)
  constexpr explicit Required(std::in_place_t, Args&&... args) : value_(std::forward<Args>(args)...) {}

  constexpr Required& emplace(T v) {  // NOLINT(readability-identifier-naming)
    value_.~T();
    new (&value_) T(std::move(v));
    return *this;
  }

  template<typename... Args>
  requires(std::constructible_from<T, Args...>)
  constexpr Required& emplace(Args&&... args) {  // NOLINT(readability-identifier-naming)
    value_.~T();
    new (&value_) T(std::forward<Args>(args)...);
    return *this;
  }

  constexpr T& get() noexcept { return value_; }  // NOLINT(*-identifier-naming)

  constexpr const T& get() const noexcept { return value_; }  // NOLINT(*-identifier-naming)

  constexpr T* operator->() noexcept __attribute__((returns_nonnull)) { return &value_; }

  constexpr const T* operator->() const noexcept __attribute__((returns_nonnull)) { return &value_; }

  constexpr T& operator*() noexcept { return value_; }

  constexpr const T& operator*() const noexcept { return value_; }

  constexpr operator T&() noexcept { return value_; }  // NOLINT(*-explicit-*)

  constexpr operator const T &() const noexcept { return value_; }  // NOLINT(*-explicit-*)

  Required& operator++() = delete;
  Required& operator--() = delete;
  Required operator++(int) = delete;  // NOLINT(cert-dcl21-cpp)
  Required operator--(int) = delete;  // NOLINT(cert-dcl21-cpp)
  Required& operator+=(std::ptrdiff_t) = delete;
  Required& operator-=(std::ptrdiff_t) = delete;
  void operator[](std::ptrdiff_t) const = delete;

  template<std::three_way_comparable_with<T> U>
  constexpr auto operator<=>(const Required<U>& other) const noexcept {
    if (value_ == other) {
      return decltype(value_ <=> other)::equivalent;
    }
    return value_ <=> other.value_;
  }

  template<std::three_way_comparable_with<T> U>
  constexpr auto operator<=>(const U& other) const noexcept {
    if (value_ == other) {
      return decltype(value_ <=> other)::equivalent;
    }
    return value_ <=> other;
  }

 private:
  T value_;
};

}  // namespace mbo::types

#endif  // MBO_TYPES_REQUIRED_H_
