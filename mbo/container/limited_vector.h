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

#ifndef MBO_CONTAINER_LIMITED_VECTOR_H_
#define MBO_CONTAINER_LIMITED_VECTOR_H_

#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <initializer_list>
#include <memory>
#include <new>  // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "mbo/container/limited_options.h"  // IWYU pragma: export
#include "mbo/types/traits.h"

namespace mbo::container {

#ifdef LV_REQUIRE
#undef LV_REQUIRE
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LV_REQUIRE(severity, condition)                      \
  /* NOLINTNEXTLINE(bugprone-switch-missing-default-case) */ \
  ABSL_LOG_IF(severity, !(condition))

// NOLINTBEGIN(readability-identifier-naming)

template<typename T>
concept LimitedVectorValid = std::move_constructible<std::remove_const_t<T>>;

// Implements a `std::vector` like container that only uses inlined memory. So if used as a local
// variable with a types that does not perform memory allocation, then this type does not perform
// any memory allocation.
//
// Unlike `std::array` this type can vary in size.
//
// The type is fully `constexpr` compliant and can be used as a `static constexpr`.
//
// Can be constructed with helpers `MakeLimitedVector` or `ToLimitedVector`.
//
// Example:
//
// ```c++
// using mbo::container::LimitedVector;
//
// constexpr auto kMyData = MakeLimitedVector(1, 2, 3, 4);
// ```
//
// The above example infers the value_type to be `int` as it is the common type of the arguments.
// The resulting `LimitedVector` has a capacity of 4 and the elements {1, 2, 3, 4}.
template<typename T, auto CapacityOrOptions>
requires(LimitedVectorValid<T>)
class LimitedVector final {
 private:
  struct None final {};

  static_assert(IsLimitedOptionsOrSize<decltype(CapacityOrOptions)>);
  using Options = decltype(MakeLimitedOptions<CapacityOrOptions>());
  // static_assert(std::is_trivially_destructible_v<T> || Options::Has(LimitedOptionsFlag::kEmptyDestructor));
  static constexpr std::size_t Capacity = Options::kCapacity;

  union Data {
    constexpr Data() noexcept : none{} {}

    constexpr Data(const Data&) noexcept = default;
    constexpr Data& operator=(const Data&) noexcept = default;
    constexpr Data(Data&&) noexcept = default;
    constexpr Data& operator=(Data&&) noexcept = default;

    constexpr ~Data() noexcept {}  // NON DEFAULT

    None none;
    T data;
  };

  // Must declare each other as friends so that we can correctly move from other.
  template<typename U, auto OtherCapacityOrOptions>
  requires(LimitedVectorValid<U>)
  friend class LimitedVector;

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
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Destructor and constructors from same type.
  constexpr ~LimitedVector() noexcept
  requires(Options::Has(LimitedOptionsFlag::kEmptyDestructor))
  = default;

  constexpr ~LimitedVector() noexcept
  requires(!Options::Has(LimitedOptionsFlag::kEmptyDestructor) && std::is_trivially_destructible_v<T>)
  {
    clear();
  }

  ~LimitedVector() noexcept
  requires(!Options::Has(LimitedOptionsFlag::kEmptyDestructor) && !std::is_trivially_destructible_v<T>)
  {
    clear();
  }

  constexpr LimitedVector() noexcept = default;

  constexpr LimitedVector(const LimitedVector& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, other.values_[size_].data);
    }
  }

  constexpr LimitedVector& operator=(const LimitedVector& other) noexcept {
    if (this != &other) {
      clear();
      size_ = other.size_;
      values_ = other.values_;
    }
    return *this;
  }

  constexpr LimitedVector(LimitedVector&& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
  }

  constexpr LimitedVector& operator=(LimitedVector&& other) noexcept {
    clear();
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
    return *this;
  }

  // Constructors and assignment from other LimitVector/value types.

  template<std::forward_iterator It>
  requires std::convertible_to<mbo::types::ForwardIteratorValueType<It>, T>
  constexpr LimitedVector(It begin, It end) noexcept {
    while (begin < end) {
      emplace_back(*begin++);
    }
  }

  constexpr LimitedVector(const std::initializer_list<T>& list) noexcept {
    auto it = list.begin();
    while (it < list.end()) {
      emplace_back(*it++);
    }
  }

  template<typename U>
  requires std::convertible_to<U, T>
  constexpr LimitedVector(const std::initializer_list<U>& list) noexcept {
    auto it = list.begin();
    while (it < list.end()) {
      emplace_back(*it++);
    }
  }

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, T> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr LimitedVector& operator=(const std::initializer_list<U>& list) noexcept {
    assign(list);
    return *this;
  }

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, T> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr explicit LimitedVector(const LimitedVector<U, OtherN>& other) noexcept {
    for (; size_ < other.size(); ++size_) {
      std::construct_at(&values_[size_].data, other.at(size_));
    }
  }

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, T> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr LimitedVector& operator=(const LimitedVector<U, OtherN>& other) noexcept {
    clear();
    for (; size_ < other.size(); ++size_) {
      std::construct_at(&values_[size_].data, other.at(size_));
    }
    return *this;
  }

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, T> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr explicit LimitedVector(LimitedVector<U, OtherN>&& other) noexcept {
    for (; size_ < other.size(); ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.at(size_)));
    }
    other.size_ = 0;
  }

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, T> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr LimitedVector& operator=(LimitedVector<U, OtherN>&& other) noexcept {
    clear();
    for (; size_ < other.size(); ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.at(size_)));
    }
    other.size_ = 0;
    return *this;
  }

  // Mofification: clear, resize, reserve, explace_back, push_back, pop_back, assign

  constexpr void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
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
    LV_REQUIRE(FATAL, size <= Capacity) << "Cannot reserve beyond capacity.";
  }

  constexpr void shrink_to_fit() noexcept {
    // Nothing to do. The contract says there is no requirement to reduce capacity.
  }

  constexpr void swap(LimitedVector& other) noexcept {
    std::size_t pos = 0;
    for (; pos < size_ && pos < other.size(); ++pos) {
      std::swap(values_[pos].data, other.at(pos));
    }
    std::size_t other_size = other.size_;
    std::size_t this_size = size_;
    for (; pos < size_; ++pos) {
      other.emplace_back(std::move(values_[pos].data));
    }
    for (; pos < other.size(); ++pos) {
      emplace_back(std::move(other.at(pos)));
    }
    size_ = other_size;
    other.size_ = this_size;
  }

  template<typename... Args>
  constexpr iterator emplace(const_iterator pos, Args&&... args) noexcept {
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace` at capacity.";
    LV_REQUIRE(FATAL, &values_[0].data <= pos && pos <= &values_[size_].data) << "Invalid `pos`";
    iterator dst = end();
    for (; dst > pos; --dst) {
      *dst = std::move(*std::prev(dst));
    }
    std::construct_at(dst, std::forward<Args>(args)...);
    ++size_;
    return dst;
  }

  constexpr iterator erase(const_iterator pos) noexcept {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    LV_REQUIRE(FATAL, &values_[0].data <= pos && pos < &values_[size_].data) << "Invalid `pos`";
    auto dst = const_cast<iterator>(pos);
    --size_;
    std::destroy_at(dst);
    for (; dst < end(); ++dst) {
      *dst = std::move(*std::next(dst));
    }
    return pos > end() ? end() : const_cast<iterator>(pos);
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
  }

  constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    LV_REQUIRE(FATAL, &values_[0].data <= first && first <= last && last <= &values_[size_].data)
        << "Invalid `first` or `last`";
    std::size_t deleted = 0;
    for (const_iterator it = first; it < last; ++it) {
      std::destroy_at(it);
      ++deleted;
    }
    auto dst = const_cast<iterator>(first);
    auto src = const_cast<iterator>(last);
    for (; src < end(); ++src, ++dst) {
      *dst = std::move(*src);
    }
    size_ -= deleted;
    return const_cast<iterator>(first);
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
  }

  template<typename... Args>
  constexpr reference emplace_back(Args&&... args) noexcept {
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace_back` at capacity.";
    auto& data_ref{values_[size_++]};
    std::construct_at(&data_ref.data, std::forward<Args>(args)...);
    return data_ref.data;
  }

  constexpr reference push_back(T&& val) noexcept {
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `push_back` at capacity.";
    auto& data_ref{values_[size_++]};
    std::construct_at(&data_ref.data, std::forward<T>(val));
    return data_ref.data;
  }

  constexpr reference push_back(const T& val) noexcept {
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `push_back` at capacity.";
    auto& data_ref{values_[size_++]};
    std::construct_at(&data_ref.data, val);
    return data_ref.data;
  }

  constexpr void pop_back() noexcept {
    LV_REQUIRE(FATAL, size_ > 0) << "No element to pop.";
    std::destroy_at(&values_[--size_].data);
  }

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
      emplace_back(std::forward<mbo::types::ForwardIteratorValueType<It>>(*begin));
    }
  }

  constexpr void assign(const std::initializer_list<T>& list) noexcept {
    clear();
    auto it = list.begin();
    while (it < list.end()) {
      emplace_back(std::forward<T>(*it));
    }
  }

  // Read/write access

  constexpr std::size_t size() const noexcept { return size_; }

  constexpr std::size_t max_size() const noexcept { return Capacity; }

  constexpr std::size_t capacity() const noexcept { return Capacity; }

  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr reference front() noexcept { return values_[0].data; }

  constexpr const_reference front() const noexcept { return values_[0].data; }

  constexpr reference back() noexcept { return values_[size_ > 0 ? size_ - 1 : 0].data; }

  constexpr const_reference back() const noexcept { return values_[size_ > 0 ? size_ - 1 : 0].data; }

  constexpr iterator begin() noexcept { return &values_[0].data; }

  constexpr const_iterator begin() const noexcept { return &values_[0].data; }

  constexpr const_iterator cbegin() const noexcept { return &values_[0].data; }

  constexpr iterator end() noexcept { return &values_[size_].data; }

  constexpr const_iterator end() const noexcept { return &values_[size_].data; }

  constexpr const_iterator cend() const noexcept { return &values_[size_].data; }

  constexpr reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(end()); }

  constexpr reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }

  constexpr const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }

  constexpr const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator(cbegin()); }

  constexpr reference operator[](std::size_t index) noexcept {
    LV_REQUIRE(FATAL, index < size_) << "Access past size.";
    return values_[index].data;
  }

  constexpr reference at(std::size_t index) noexcept {
    LV_REQUIRE(FATAL, index < size_) << "Access past size.";
    return values_[index].data;
  }

  constexpr const_reference operator[](std::size_t index) const noexcept {
    LV_REQUIRE(FATAL, index < size_) << "Access past size.";
    return values_[index].data;
  }

  constexpr const_reference at(std::size_t index) const noexcept {
    LV_REQUIRE(FATAL, index < size_) << "Access past size.";
    return values_[index].data;
  }

  constexpr const_pointer data() const noexcept { return &values_[0].data; }

 private:
  std::size_t size_{0};
  // Array would be better but that does not work with ASAN builds.
  // std::array<Data, Capacity == 0 ? 1 : Capacity> values_;
  // Add an unused sentinal, so that `end` and other functions do not cause memory issues.
  Data values_[Capacity + 1];  // NOLINT(*-avoid-c-arrays)
};

template<typename... T>
LimitedVector(T&&... v) -> LimitedVector<std::common_type_t<T...>, sizeof...(T)>;

template<size_t LN, size_t RN, typename LHS, typename RHS>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline auto operator<=>(const LimitedVector<LHS, LN>& lhs, const LimitedVector<RHS, RN>& rhs) noexcept {
  std::size_t minsize = std::min(LN, RN);
  for (std::size_t index = 0; index < minsize; ++index) {
    const auto comp = lhs[index] <=> rhs[index];
    if (comp != 0) {
      return comp;
    }
  }
  return lhs.size() <=> rhs.size();
}

template<size_t LN, size_t RN, typename LHS, typename RHS>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline bool operator==(const LimitedVector<LHS, LN>& lhs, const LimitedVector<RHS, RN>& rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  std::size_t minsize = std::min(LN, RN);
  for (std::size_t index = 0; index < minsize; ++index) {
    const auto comp = lhs[index] <=> rhs[index];
    if (comp != 0) {
      return false;
    }
  }
  return true;
}

template<size_t LN, size_t RN, typename LHS, typename RHS>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline bool operator<(const LimitedVector<LHS, LN>& lhs, const LimitedVector<RHS, RN>& rhs) noexcept {
  std::size_t minsize = std::min(LN, RN);
  for (std::size_t index = 0; index < minsize; ++index) {
    const auto comp = lhs[index] <=> rhs[index];
    if (comp != 0) {
      return comp < 0;
    }
  }
  return lhs.size() < rhs.size();
}

template<typename T, std::size_t N = 0>
inline constexpr auto MakeLimitedVector() noexcept {
  return LimitedVector<T, N>();
}

template<std::size_t N, std::forward_iterator It>
inline constexpr auto MakeLimitedVector(It&& begin, It&& end) noexcept {
  return LimitedVector<mbo::types::ForwardIteratorValueType<It>, N>(std::forward<It>(begin), std::forward<It>(end));
}

template<std::size_t N, typename T>
inline constexpr auto MakeLimitedVector(const std::initializer_list<T>& data) {
  LV_REQUIRE(FATAL, data.size() <= N) << "Too many initlizer values.";
  return LimitedVector<T, N>(data);
}

template<std::size_t N, typename T>
requires(N > 0 && mbo::types::NotInitializerList<T>)
inline constexpr auto MakeLimitedVector(const T& value) noexcept {
  auto result = LimitedVector<T, N>();
  result.assign(N, value);
  return result;
}

template<typename... Args>
requires((types::NotInitializerList<Args> && !std::forward_iterator<Args> && !types::IsCharArray<Args>) && ...)
inline constexpr auto MakeLimitedVector(Args&&... args) noexcept {
  using T = std::common_type_t<Args...>;
  auto result = LimitedVector<T, sizeof...(Args)>();
  (result.emplace_back(std::forward<T>(args)), ...);
  return result;
}

// This specialization takes `const char*` and `const char(&)[N]` arguments and creates an appropriately sized container
// of type `LimitedVector<std::string_view>`.
template<int&..., types::IsCharArray... Args>
inline constexpr auto MakeLimitedVector(Args... args) noexcept {
  auto result = LimitedVector<std::string_view, sizeof...(Args)>();
  (result.emplace_back(std::string_view(args)), ...);
  return result;
}

template<types::NotIsCharArray T, int&..., types::IsCharArray... Args>
inline constexpr auto MakeLimitedVector(Args... args) noexcept {
  auto result = LimitedVector<T, sizeof...(Args)>();
  (result.emplace_back(T(args)), ...);
  return result;
}

// NOLINTBEGIN(*-avoid-c-arrays)
template<typename T, std::size_t N>
constexpr LimitedVector<std::remove_cvref_t<T>, N> ToLimitedVector(T (&array)[N]) {
  LimitedVector<std::remove_cvref_t<T>, N> result;
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace_back(array[idx]);
  }
  return result;
}

template<typename T, std::size_t N>
constexpr LimitedVector<std::remove_cvref_t<T>, N> ToLimitedVector(T (&&array)[N]) {
  LimitedVector<std::remove_cvref_t<T>, N> result;
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace_back(std::move(array[idx]));
  }
  return result;
}

// NOLINTEND(*-avoid-c-arrays)

// NOLINTEND(readability-identifier-naming)

#undef LV_REQUIRE

}  // namespace mbo::container

#endif  // MBO_CONTAINER_LIMITED_VECTOR_H_
