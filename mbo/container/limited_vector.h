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

#ifndef MBO_CONTAINER_LIMITED_VECTOR_H_
#define MBO_CONTAINER_LIMITED_VECTOR_H_

#include <array>
#include <concepts>
#include <exception>
#include <initializer_list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "absl/log/absl_check.h"

// NOLINTBEGIN(readability-identifier-naming)

namespace mbo::container {
namespace internal {

template<typename>
constexpr inline bool is_initializer_list = false;
template<typename T>
constexpr inline bool is_initializer_list<std::initializer_list<T>> = true;

template<typename T>
concept NotInitializerList = !is_initializer_list<T>;

}  // namespace internal

// Implements a `std::vector` like container that only uses inlined memory. So if used as a local
// variable with a types that does not perform memory allocation, then this type does not perform
// any memory allocation.
//
// Unlike `std::array` this type can vary in size.
//
// The type is fully `constexpr` compliant and can be used as a `static constexpr`.
//
// Can be constructed with helpers `MakeSmallVec`.
template<class T, std::size_t Capacity>
class LimitedVector final {
 private:
  struct Storage {
    struct None {};

    union Data {
      None none;
      T data;

      constexpr Data() noexcept : none{} {}
    };

    std::array<Data, Capacity == 0 ? 1 : Capacity> values;
  };

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = T*;
  using const_iterator = const T*;

  LimitedVector() noexcept = default;

  template<std::forward_iterator It>
  constexpr LimitedVector(It begin, It end) noexcept {
    while (begin < end) {
      emplace_back(*begin++);
    }
  }

  constexpr LimitedVector(std::initializer_list<T> list) noexcept {
    auto it = list.begin();
    while (it < list.end()) {
      emplace_back(std::move(*it++));
    }
  }

  constexpr LimitedVector(const LimitedVector& other) noexcept : size_(other.size_), storage_(other.storage_) {}

  constexpr LimitedVector& operator=(const LimitedVector& other) noexcept {
    if (this != &other) {
      clear();
      size_ = other.size_;
      storage_ = other.storage_;
    }
    return *this;
  }

  constexpr LimitedVector(LimitedVector&& other) noexcept : size_(other.size_), storage_(std::move(other.storage_)) {
    other.size_ = 0;
  }

  constexpr LimitedVector& operator=(LimitedVector&& other) noexcept {
    clear();
    size_ = other.size_;
    storage_ = std::move(other.storage_);
    other.size_ = 0;
    return *this;
  }

  constexpr ~LimitedVector() noexcept { clear(); }

  constexpr void assign(std::size_t num, const T& value) noexcept {
    clear();
    for (; num > 0; --num) {
      push_back(value);
    }
  }

  template<std::forward_iterator It>
  constexpr void assign(It begin, It end) noexcept {
    clear();
    while (begin < end) {
      emplace_back(*begin);
    }
  }

  constexpr void assign(std::initializer_list<T> list) noexcept {
    clear();
    auto it = list.begin();
    while (it < list.end()) {
      emplace_back(std::move(*it));
    }
  }

  constexpr LimitedVector& operator=(std::initializer_list<T> list) noexcept {
    assign(std::move(list));
    return *this;
  }

  template<size_t LN, size_t RN>
  friend auto operator<=>(const LimitedVector<T, LN>& lhs, const LimitedVector<T, RN>& rhs) noexcept;

  constexpr std::size_t size() const noexcept { return size_; }

  constexpr std::size_t max_size() const noexcept { return Capacity; }

  constexpr std::size_t capacity() const noexcept { return Capacity; }

  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr reference front() noexcept { return storage_[0].data; }

  constexpr const_reference front() const noexcept { return storage_[0].data; }

  constexpr reference back() noexcept { return storage_[size_ - 1].data; }

  constexpr const_reference back() const noexcept { return storage_[size_ - 1].data; }

  constexpr iterator begin() noexcept { return &storage_.values[0].data; }

  constexpr const_iterator begin() const noexcept { return &storage_.values[0].data; }

  constexpr const_iterator cbegin() const noexcept { return &storage_.values[0].data; }

  constexpr iterator end() noexcept { return &storage_.values[size_].data; }

  constexpr const_iterator end() const noexcept { return &storage_.values[size_].data; }

  constexpr const_iterator cend() const noexcept { return &storage_.values[size_].data; }

  constexpr reference operator[](std::size_t index) noexcept {
    ABSL_CHECK_LT(index, size_) << "Access past size.";
    return storage_.value[index].data;
  }

  constexpr reference at(std::size_t index) noexcept {
    ABSL_CHECK_LT(index, size_) << "Access past size.";
    return storage_.value[index].data;
  }

  constexpr const_reference operator[](std::size_t index) const noexcept {
    ABSL_CHECK_LT(index, size_) << "Access past size.";
    return storage_.value[index].data;
  }

  constexpr const_reference at(std::size_t index) const noexcept {
    ABSL_CHECK_LT(index, size_) << "Access past size.";
    return storage_.value[index].data;
  }

  template<typename... Args>
  constexpr reference emplace_back(Args&&... args) noexcept {
    ABSL_CHECK_LE(size_, Capacity) << "Called `emplace_back` at capacity.";
    auto& data_ref{storage_.values[size_]};
    std::construct_at(&data_ref.data, std::forward<Args>(args)...);
    ++size_;
    return data_ref.data;
  }

  constexpr reference push_back(T&& val) noexcept {
    ABSL_CHECK_LE(size_, Capacity) << "Called `push_back` at capacity.";
    auto& data_ref{storage_.values[size_]};
    std::construct_at(&data_ref.data, std::forward<T>(val));
    ++size_;
    return data_ref.data;
  }

  constexpr reference push_back(const T& val) noexcept {
    ABSL_CHECK_LE(size_, Capacity) << "Called `push_back` at capacity.";
    auto& data_ref{storage_.values[size_]};
    std::construct_at(&data_ref.data, val);
    ++size_;
    return data_ref.data;
  }

  constexpr void pop_back() noexcept {
    ABSL_CHECK_GT(size_, 0) << "No element to pop.";
    std::destroy_at(&storage_.values[--size_].data);
  }

  constexpr void resize(std::size_t new_size) noexcept {
    while (new_size < size()) {
      pop_back();
    }
    while (new_size > size()) {
      push_back({});
    }
  }

  constexpr void reserve(std::size_t size) noexcept {
    ABSL_CHECK_LE(size, Capacity) << "Cannot reserve beyond capacity.";
  }

  constexpr void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

 private:
  std::size_t size_{0};
  Storage storage_;
};

template<size_t LN, size_t RN, typename T>
inline auto operator<=>(const LimitedVector<T, LN>& lhs, const LimitedVector<T, RN>& rhs) noexcept {
  std::size_t minsize = std::min(LN, RN);
  for (std::size_t index = 0; index < minsize; ++index) {
    const auto comp = lhs[index] <=> rhs[index];
    if (comp != 0) {
      return comp;
    }
  }
  return LN <=> RN;
}

template<typename T>
inline constexpr auto MakeLimitedVector() noexcept {
  return LimitedVector<T, 0>();
}

template<std::size_t N, std::forward_iterator It>
inline constexpr auto MakeLimitedVector(It&& begin, It&& end) noexcept {
  return LimitedVector<typename It::value_type, N>(std::forward<It>(begin), std::forward<It>(end));
}

template<std::size_t N, typename T>
inline constexpr auto MakeLimitedVector(std::initializer_list<T>&& data) {
  ABSL_CHECK_LE(data.size(), N) << "Too many initlizer values.";
  return LimitedVector<T, N>(std::forward<std::initializer_list<T>>(data));
}

template<std::size_t N, typename T>
inline constexpr auto MakeLimitedVector(T&& value) noexcept {
  static_assert(N > 0);
  auto result = LimitedVector<T, N>();
  result.assign(N, value);
  return result;
}

template<internal::NotInitializerList... Args>
inline constexpr auto MakeLimitedVector(Args&&... args) noexcept {
  return LimitedVector<std::common_type_t<Args...>, sizeof...(Args)>({args...});
}

}  // namespace mbo::container

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_CONTAINER_LIMITED_VECTOR_H_
