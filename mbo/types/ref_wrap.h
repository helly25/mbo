// Copyright 2024 M. Boerger (helly25.com)
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
  ~RefWrap() noexcept = default;

  RefWrap() = delete;

  explicit RefWrap(T& ref) noexcept : ptr_(&ref) {}

  RefWrap(const RefWrap&) noexcept = default;
  RefWrap& operator=(const RefWrap&) noexcept = default;
  RefWrap(RefWrap&&) noexcept = default;
  RefWrap& operator=(RefWrap&&) noexcept = default;

  RefWrap& operator=(T& ref) noexcept {
    ptr_ = &ref;
    return *this;
  }

  T* get() noexcept __attribute__((returns_nonnull)) { return ptr_; }  // NOLINT(*-identifier-naming)

  const T* get() const noexcept __attribute__((returns_nonnull)) { return ptr_; }  // NOLINT(*-identifier-naming)

  T* operator->() noexcept __attribute__((returns_nonnull)) { return ptr_; }

  const T* operator->() const noexcept __attribute__((returns_nonnull)) { return ptr_; }

  T& operator*() noexcept { return *ptr_; }

  const T& operator*() const noexcept { return *ptr_; }

  operator T&() noexcept { return *ptr_; }  // NOLINT(*-explicit-*)

  operator const T &() const noexcept { return *ptr_; }  // NOLINT(*-explicit-*)

  RefWrap& operator++() = delete;
  RefWrap& operator--() = delete;
  RefWrap operator++(int) = delete;  // NOLINT(cert-dcl21-cpp)
  RefWrap operator--(int) = delete;  // NOLINT(cert-dcl21-cpp)
  RefWrap& operator+=(std::ptrdiff_t) = delete;
  RefWrap& operator-=(std::ptrdiff_t) = delete;
  void operator[](std::ptrdiff_t) const = delete;

  auto operator<=>(const RefWrap& other) const noexcept {
    if (ptr_ == other.ptr_) {
      return decltype(*ptr_ <=> other)::equivalent;
    }
    return *ptr_ <=> *other.ptr_;
  }

  template<typename U>
  requires(std::three_way_comparable_with<T, U>)
  auto operator<=>(const U& other) const noexcept {
    if (ptr_ == &other) {
      return decltype(*ptr_ <=> other)::equivalent;
    }
    return *ptr_ <=> other;
  }

  friend auto operator<=>(const RefWrap& lhs, const RefWrap& rhs) noexcept = default;

 private:
  T* ptr_;
};

}  // namespace mbo::types

#endif  // MBO_TYPES_REF_WRAP_H_
