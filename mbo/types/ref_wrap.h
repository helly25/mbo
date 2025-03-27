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

#ifndef MBO_TYPES_REF_WRAP_H_
#define MBO_TYPES_REF_WRAP_H_

#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <cstddef>

namespace mbo::types {

// Template class `RefWrap<T>` is a reference wrapper for a type `T`. It has
// similar goals to `std::reference_wrapper` or GSL's `not_null`. However, the
// wrapper behaves like a pointer that cannot be a nullptr. In details:
// Unlike `std::reference_wrapper`:
//   * Can be used directly without calling `get`.
//   * The reference can be updated directly.
//
// Unlike GSL's `not_null`:
//   * Cannot be initialized with a nullptr. Instead it must be initialized by
//     an actual reference (note that technically you could still construct a
//     nullptr that way).
//   * Comparison is on the values not the pointer (since we mimic references).
template<typename T>
class RefWrap final {
 public:
  using type = T;

  constexpr ~RefWrap() noexcept = default;

  constexpr RefWrap() = delete;

  constexpr /* implicit */ RefWrap(T& ref) noexcept : ptr_(&ref) {}  // NOLINT(google-explicit-*,hicpp-explicit-*)

  constexpr RefWrap(const RefWrap&) noexcept = default;
  constexpr RefWrap& operator=(const RefWrap&) noexcept = default;
  constexpr RefWrap(RefWrap&&) noexcept = default;
  constexpr RefWrap& operator=(RefWrap&&) noexcept = default;

  constexpr RefWrap& operator=(T& ref) noexcept {
    ptr_ = &ref;
    return *this;
  }

  constexpr T& get() noexcept { return *ptr_; }  // NOLINT(*-identifier-naming)

  constexpr const T& get() const noexcept { return *ptr_; }  // NOLINT(*-identifier-naming)

  constexpr T* operator->() noexcept __attribute__((returns_nonnull)) { return ptr_; }

  constexpr const T* operator->() const noexcept __attribute__((returns_nonnull)) { return ptr_; }

  constexpr T& operator*() noexcept { return *ptr_; }

  constexpr const T& operator*() const noexcept { return *ptr_; }

  constexpr operator T&() noexcept { return *ptr_; }  // NOLINT(*-explicit-*)

  constexpr operator const T &() const noexcept { return *ptr_; }  // NOLINT(*-explicit-*)

  RefWrap& operator++() = delete;
  RefWrap& operator--() = delete;
  RefWrap operator++(int) = delete;  // NOLINT(cert-dcl21-cpp)
  RefWrap operator--(int) = delete;  // NOLINT(cert-dcl21-cpp)
  RefWrap& operator+=(std::ptrdiff_t) = delete;
  RefWrap& operator-=(std::ptrdiff_t) = delete;
  void operator[](std::ptrdiff_t) const = delete;

  template<std::three_way_comparable_with<T> U>
  constexpr auto operator<=>(const RefWrap<U>& other) const noexcept {
    if (ptr_ == other.ptr_) {
      return decltype(*ptr_ <=> other)::equivalent;
    }
    return *ptr_ <=> *other.ptr_;
  }

  template<std::three_way_comparable_with<T> U>
  constexpr auto operator<=>(const U& other) const noexcept {
    if (ptr_ == &other) {
      return decltype(*ptr_ <=> other)::equivalent;
    }
    return *ptr_ <=> other;
  }

 private:
  T* ptr_;
};

}  // namespace mbo::types

#endif  // MBO_TYPES_REF_WRAP_H_
