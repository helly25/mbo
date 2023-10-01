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

#ifndef MBO_CONTAINER_LIMITED_SET_H_
#define MBO_CONTAINER_LIMITED_SET_H_

#include <algorithm>
#include <compare>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "mbo/types/traits.h"

namespace mbo::container {

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LV_REQUIRE(severity, condition) ABSL_LOG_IF(severity, !(condition))

// NOLINTBEGIN(readability-identifier-naming)

// Implements a `std::set` like container that only uses inlined memory. So if used as a local
// variable with a types that does not perform memory allocation, then this type does not perform
// any memory allocation.
//
// Unlike `std::array` this type can vary in size.
//
// The type is fully `constexpr` compliant and can be used as a `static constexpr`.
//
// Can be constructed with helpers `MakeLimitedSet` or `ToLimitedSet`.
//
// Example:
//
// ```c++
// using mbo::container::LimitedSet;
//
// constexpr auto kMyData = MakeLimitedSet(1, 3, 2, 4);
// ```
//
// The above example infers the value_type to be `int` as it is the common type of the arguments.
// The resulting `LimitedSet` has a capacity of 4 and the elements {1, 2, 3, 4}.
//
// Internally a C-array is used and elements are moved as needed. That means that element
// addresses are not stable.
//
// Note that construction from char-arrays results in a container of `std::string_view` as opposed to `const char*`.
template<typename Key, std::size_t Capacity, typename Compare = std::less<Key>>
requires(std::move_constructible<Key>)
class LimitedSet final {
 private:
  struct None final {};

  union Data {
    constexpr Data() noexcept : none{} {}

    constexpr Data(const Data&) noexcept = default;
    constexpr Data& operator=(const Data&) noexcept = default;
    constexpr Data(Data&&) noexcept = default;
    constexpr Data& operator=(Data&&) noexcept = default;

    constexpr ~Data() noexcept {}  // NON DEFAULT

    Key data;
    None none;
  };

  // Must declare each other as friends so that we can correctly move from other.
  template<typename U, std::size_t OtherN, typename Comp>
  requires(std::move_constructible<U>)
  friend class LimitedSet;

 public:
  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;
  using value_compare = Compare;
  using reference = Key&;
  using const_reference = const Key&;
  using pointer = Key*;
  using const_pointer = const Key*;

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = LimitedSet::difference_type;
    using value_type = LimitedSet::value_type;
    using pointer = LimitedSet::const_pointer;
    using reference = LimitedSet::const_reference;

    constexpr const_iterator() noexcept : pos_(nullptr) {}  // Needed for STL

    constexpr explicit const_iterator(const Data* pos) noexcept : pos_(pos) {}

    constexpr explicit operator reference() const noexcept { return pos_->data; }

    constexpr reference operator*() const noexcept { return pos_->data; }

    constexpr pointer operator->() const noexcept { return &pos_->data; }

    constexpr const_iterator& operator++() noexcept {
      ++pos_;
      return *this;
    }

    constexpr const_iterator& operator--() noexcept {
      --pos_;
      return *this;
    }

    constexpr const_iterator operator++(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      const auto it = *this;
      ++pos_;
      return it;
    }

    constexpr const_iterator operator--(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      const auto it = *this;
      --pos_;
      return it;
    }

    constexpr const_iterator& operator+=(size_type ofs) noexcept {
      pos_ += ofs;
      return *this;
    }

    constexpr const_iterator& operator-=(size_type ofs) noexcept {
      pos_ -= ofs;
      return *this;
    }

    friend constexpr size_type operator-(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ - rhs.pos_;
    }

    friend constexpr size_type operator+(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ + rhs.pos_;
    }

    friend constexpr const_iterator operator-(const_iterator lhs, size_type rhs) noexcept {
      return const_iterator(lhs.pos_ - rhs);
    }

    friend constexpr const_iterator operator+(const_iterator lhs, size_type rhs) noexcept {
      return const_iterator(lhs.pos_ + rhs);
    }

    friend constexpr auto operator<=>(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ <=> rhs.pos_; }

    friend constexpr bool operator<(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ < rhs.pos_; }

    friend constexpr bool operator==(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ == rhs.pos_; }

   private:
    const Data* pos_;
  };

  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = LimitedSet::difference_type;
    using value_type = LimitedSet::value_type;
    using pointer = LimitedSet::pointer;
    using reference = LimitedSet::reference;

    constexpr iterator() noexcept : pos_(nullptr) {}  // Needed for STL

    constexpr explicit iterator(Data* pos) noexcept : pos_(pos) {}

    constexpr explicit operator reference() const noexcept { return pos_->data; }

    constexpr reference operator*() const noexcept { return pos_->data; }

    constexpr pointer operator->() const noexcept { return &pos_->data; }

    constexpr explicit operator const_iterator() const noexcept { return const_iterator(pos_); }

    constexpr iterator& operator++() noexcept {
      ++pos_;
      return *this;
    }

    constexpr iterator& operator--() noexcept {
      --pos_;
      return *this;
    }

    constexpr iterator operator++(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      const auto it = *this;
      ++pos_;
      return it;
    }

    constexpr iterator operator--(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      const auto it = *this;
      --pos_;
      return it;
    }

    constexpr iterator& operator+=(size_type ofs) noexcept {
      pos_ += ofs;
      return *this;
    }

    constexpr iterator& operator-=(size_type ofs) noexcept {
      pos_ -= ofs;
      return *this;
    }

    friend constexpr size_type operator-(iterator lhs, iterator rhs) noexcept { return lhs.pos_ - rhs.pos_; }

    friend constexpr size_type operator+(iterator lhs, iterator rhs) noexcept { return lhs.pos_ + rhs.pos_; }

    friend constexpr iterator operator-(iterator lhs, size_type rhs) noexcept { return iterator(lhs.pos_ - rhs); }

    friend constexpr iterator operator+(iterator lhs, size_type rhs) noexcept { return iterator(lhs.pos_ + rhs); }

    friend constexpr auto operator<=>(iterator lhs, iterator rhs) noexcept { return lhs.pos_ <=> rhs.pos_; }

    friend constexpr bool operator<(iterator lhs, iterator rhs) noexcept { return lhs.pos_ < rhs.pos_; }

    friend constexpr bool operator==(iterator lhs, iterator rhs) noexcept { return lhs.pos_ == rhs.pos_; }

    friend std::ostream& operator<<(std::ostream& os, const iterator& pos) {
      return os << "[" << pos.pos_ << " = " << pos.pos_->data << "]";
    }

   private:
    Data* pos_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  template<typename T>
  struct IsIterator
      : std::bool_constant<
            std::same_as<std::remove_cvref_t<T>, iterator> || std::same_as<std::remove_cvref_t<T>, const_iterator>> {};

 public:
  // Destructor and constructors from same type.

  constexpr ~LimitedSet() noexcept { clear(); }

  constexpr LimitedSet() noexcept = default;
  constexpr explicit LimitedSet(const Compare& key_comp) noexcept : key_comp_(key_comp){};

  constexpr LimitedSet(const LimitedSet& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, other.values_[size_].data);
    }
  }

  constexpr LimitedSet& operator=(const LimitedSet& other) noexcept {
    if (this != &other) {
      clear();
      size_ = other.size_;
      values_ = other.values_;
    }
    return *this;
  }

  constexpr LimitedSet(LimitedSet&& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
  }

  constexpr LimitedSet& operator=(LimitedSet&& other) noexcept {
    clear();
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
    return *this;
  }

  // Constructors and assignment from other LimitVector/value types.

  template<std::forward_iterator It>
  requires std::convertible_to<mbo::types::ForwardIteratorValueType<It>, Key>
  constexpr LimitedSet(It begin, It end, const Compare& key_comp = Compare()) noexcept : key_comp_(key_comp) {
    while (begin < end) {
      emplace(*begin);
      ++begin;
    }
  }

  constexpr LimitedSet(const std::initializer_list<Key>& list, const Compare& key_comp = Compare()) noexcept
      : key_comp_(key_comp) {
    auto it = list.begin();
    while (it < list.end()) {
      emplace(*it);//emplace(std::move(*it));
      ++it;
    }
  }

  template<typename U>
  requires(std::convertible_to<U, Key> && !std::same_as<Key, U>)
  constexpr LimitedSet(const std::initializer_list<U>& list, const Compare& key_comp = Compare()) noexcept
      : key_comp_(key_comp) {
    auto it = list.begin();
    while (it < list.end()) {
      emplace(*it);
      ++it;
    }
  }

  template<typename U, std::size_t OtherN>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr LimitedSet& operator=(const std::initializer_list<U>& list) noexcept {
    assign(list);
    return *this;
  }

  template<typename U, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr explicit LimitedSet(const LimitedSet<U, OtherN, OtherCompare>& other) noexcept {
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(*it);
    }
  }

  template<typename U, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr LimitedSet& operator=(const LimitedSet<U, OtherN, OtherCompare>& other) noexcept {
    clear();
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(*it);
    }
    return *this;
  }

  template<typename U, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr explicit LimitedSet(LimitedSet<U, OtherN, OtherCompare>&& other) noexcept {
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(std::move(*it));
    }
    other.size_ = 0;
  }

  template<typename U, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr LimitedSet& operator=(LimitedSet<U, OtherN, OtherCompare>&& other) noexcept {
    clear();
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(std::move(*it));
    }
    other.size_ = 0;
    return *this;
  }

  // Find and search: lower_bound, upper_bound, equal_range, find, contains, count

  constexpr iterator lower_bound(const Key& key) { return std::lower_bound(begin(), end(), key, key_comp_); }

  constexpr const_iterator lower_bound(const Key& key) const {
    return std::lower_bound(begin(), end(), key, key_comp_);
  }

  constexpr iterator upper_bound(const Key& key) { return std::upper_bound(begin(), end(), key, key_comp_); }

  constexpr const_iterator upper_bound(const Key& key) const {
    return std::upper_bound(begin(), end(), key, key_comp_);
  }

  constexpr iterator find(const Key& key) {
    iterator it = lower_bound(key);
    return it == end() || key_comp_(*it, key) || key_comp_(key, *it) ? end() : it;
  }

  constexpr const_iterator find(const Key& key) const {
    const_iterator it = lower_bound(key);
    return it == end() || key_comp_(*it, key) || key_comp_(key, *it) ? end() : it;
  }

  constexpr bool contains(const Key& key) const { return std::binary_search(begin(), end(), key, key_comp_); }

  // Performs contains-all-of functionality (not part of STL).
  template<typename Other>
  requires(types::ContainerIsForwardIteratable<Other> && std::equality_comparable_with<typename Other::value_type, Key>)
  constexpr bool contains_all(const Other& other) const {
    for (auto it = other.begin(); it != other.end(); ++it) {
      if (!contains(*it)) {
        return false;
      }
    }
    return true;
  }

  // Performs contains-all-of functionality (not part of STL).
  template<typename U>
  requires(std::equality_comparable_with<Key, U>)
  constexpr bool contains_all(const std::initializer_list<U>& other) const {
    for (auto it = other.begin(); it != other.end(); ++it) {
      if (!contains(*it)) {
        return false;
      }
    }
    return true;
  }

  // Performs contains-any-of functionality (not part of STL).
  template<typename Other = std::initializer_list<Key>>
  requires(types::ContainerIsForwardIteratable<Other> && std::equality_comparable_with<typename Other::value_type, Key>)
  constexpr bool contains_any(const Other& other) const {
    for (auto it = other.begin(); it != other.end(); ++it) {
      if (contains(*it)) {
        return true;
      }
    }
    return false;
  }

  // Performs contains-any-of functionality (not part of STL).
  template<typename U>
  requires(std::equality_comparable_with<Key, U>)
  constexpr bool contains_any(const std::initializer_list<U>& other) const {
    for (auto it = other.begin(); it != other.end(); ++it) {
      if (contains(*it)) {
        return true;
      }
    }
    return false;
  }

  constexpr std::pair<iterator, iterator> equal_range(const Key& key) {
    return std::equal_range(begin(), end(), key, key_comp_);
  }

  constexpr std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
    return std::equal_range(begin(), end(), key, key_comp_);
  }

  constexpr std::size_t count(const Key& key) const {
    const auto [first, last] = equal_range(key);
    return last - false;
  }

  // Mofification: clear, swap, emplace, insert

  constexpr void clear() noexcept {
    while (size_ > 0) {
      std::destroy_at(&values_[--size_].data);
    }
  }

  constexpr void swap(LimitedSet& other) noexcept {
    std::size_t pos = 0;
    for (; pos < size_ && pos < other.size(); ++pos) {
      std::swap(values_[pos].data, other.values_[pos].data);
    }
    std::size_t other_size = other.size_;
    std::size_t this_size = size_;
    for (; pos < size_; ++pos) {
      other.emplace(std::move(values_[pos].data));
    }
    for (; pos < other.size(); ++pos) {
      emplace(std::move(other.values_[pos].data));
    }
    size_ = other_size;
    other.size_ = this_size;
  }

  template<typename... Args>
  constexpr std::pair<iterator, bool> emplace(Args&&... args) noexcept {
    Key new_key(args...);
    const iterator dst = lower_bound(new_key);
    if (dst != end() && !key_comp_(*dst, new_key) && !key_comp_(new_key, *dst)) {
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(&*dst, std::move(new_key));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<typename It>
  requires IsIterator<It>::value
  constexpr iterator erase(It pos) noexcept {
    LV_REQUIRE(FATAL, begin() <= pos && pos < end()) << "Invalid `pos`";
    auto dst = to_iterator(pos);
    --size_;
    std::destroy_at(&*dst);
    for (; dst < end(); ++dst) {
      *dst = std::move(*std::next(dst));
    }
    return pos > end() ? end() : to_iterator(pos);
  }

  constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
    LV_REQUIRE(FATAL, begin() <= first && first <= last && last <= end()) << "Invalid `first` or `last`";
    std::size_t deleted = 0;
    for (const_iterator it = first; it < last; ++it) {
      std::destroy_at(it);
      ++deleted;
    }
    auto dst = to_iterator(first);
    auto src = to_iterator(last);
    for (; src < end(); ++src, ++dst) {
      *dst = std::move(*src);
    }
    size_ -= deleted;
    return to_iterator(first);
  }

  constexpr size_type erase(const Key& key) {
    size_type count = 0;
    while (true) {
      auto pos = find(key);
      if (pos == end()) {
        break;
      }
      erase(pos);
      ++count;
    }
    return count;
  }

  template<typename K>
  requires(!IsIterator<K>::value)
  constexpr size_type erase(K&& key) {
    size_type count = 0;
    while (true) {
      auto pos = find(key);
      if (pos == end()) {
        break;
      }
      erase(pos);
      ++count;
    }
    return count;
  }

  constexpr std::pair<iterator, bool> insert(const value_type& value) { return emplace(value); }

  constexpr std::pair<iterator, bool> insert(value_type&& value) { return emplace(std::move(value)); }

  template<typename InputIt>
  constexpr void insert(InputIt first, InputIt last) {
    while (first < last) {
      emplace(*first);
      ++first;
    }
  }

  // Read/write access

  constexpr std::size_t size() const noexcept { return size_; }

  constexpr std::size_t max_size() const noexcept { return Capacity; }

  constexpr std::size_t capacity() const noexcept { return Capacity; }

  constexpr bool empty() const noexcept { return size_ == 0; }

  // NOLINTBEGIN(readability-container-data-pointer)

  constexpr iterator begin() noexcept { return iterator(&values_[0]); }

  constexpr const_iterator begin() const noexcept { return const_iterator(&values_[0]); }

  constexpr const_iterator cbegin() const noexcept { return const_iterator(&values_[0]); }

  constexpr iterator end() noexcept { return iterator(&values_[size_]); }

  constexpr const_iterator end() const noexcept { return const_iterator(&values_[size_]); }

  constexpr const_iterator cend() const noexcept { return const_iterator(&values_[size_]); }

  // NOLINTEND(readability-container-data-pointer)

  constexpr reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(end()); }

  constexpr reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }

  constexpr const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }

  constexpr const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator(cbegin()); }

  // Observers

  constexpr key_compare key_comp() const { return key_comp; }

  constexpr value_compare value_comp() const { return key_comp; }

 private:
  static constexpr iterator to_iterator(iterator pos) noexcept { return pos; }

  static constexpr iterator to_iterator(const const_iterator& pos) noexcept { return iterator(pos.pos); }

  std::size_t size_{0};
  // Array would be better but that does not work with ASAN builds.
  // std::array<Data, Capacity == 0 ? 1 : Capacity> values_;
  Data values_[Capacity == 0 ? 1 : Capacity];  // NOLINT(*-avoid-c-arrays)
  const key_compare key_comp_ = {};
};

template<types::NotIsCharArray... T>
LimitedSet(T&&... data) -> LimitedSet<std::common_type_t<T...>, sizeof...(T)>;

template<types::IsCharArray... T>
LimitedSet(T&&... data) -> LimitedSet<std::string_view, sizeof...(T)>;

template<size_t LN, size_t RN, typename LHS, typename RHS, typename LCompare, typename RCompare>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline auto operator<=>(
    const LimitedSet<LHS, LN, LCompare>& lhs,
    const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = *lhs_it <=> *rhs_it;
    if (comp != 0) {
      return comp;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() <=> rhs.size();
}

template<size_t LN, size_t RN, typename LHS, typename RHS, typename LCompare, typename RCompare>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline bool operator==(
    const LimitedSet<LHS, LN, LCompare>& lhs,
    const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = *lhs_it <=> *rhs_it;
    if (comp != 0) {
      return false;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() == rhs.size();
}

template<size_t LN, size_t RN, typename LHS, typename RHS, typename LCompare, typename RCompare>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline bool operator<(
    const LimitedSet<LHS, LN, LCompare>& lhs,
    const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = *lhs_it <=> *rhs_it;
    if (comp != 0) {
      return comp < 0;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() < rhs.size();
}

template<typename Key, std::size_t N = 0, typename Compare = std::less<Key>>
inline constexpr auto MakeLimitedSet() noexcept {  // Parameter `key_comp` would create a conflict.
  return LimitedSet<Key, N, Compare>();
}

template<
    std::size_t N,
    std::forward_iterator It,
    typename Compare = std::less<mbo::types::ForwardIteratorValueType<It>>>
inline constexpr auto MakeLimitedSet(It begin, It end, const Compare& key_comp = Compare()) noexcept {
  return LimitedSet<mbo::types::ForwardIteratorValueType<It>, N, Compare>(begin, end, key_comp);
}

template<std::size_t N, typename Key, typename Compare = std::less<Key>>
inline constexpr auto MakeLimitedSet(const std::initializer_list<Key>& data, const Compare& key_comp = Compare()) {
  return LimitedSet<Key, N, Compare>(data, key_comp);
}

template<typename... Args>
requires((types::NotInitializerList<Args> && !std::forward_iterator<Args> && !types::IsCharArray<Args>) && ...)
inline constexpr auto MakeLimitedSet(Args&&... args) noexcept {
  using T = std::common_type_t<Args...>;
  auto result = LimitedSet<T, sizeof...(Args)>();
  (result.emplace(std::forward<T>(args)), ...);
  return result;
}

// This specialization takes `const char*` and `const char(&)[N]` arguments and creates an appropriately sized container
// of type `LimitedSet<std::string_view>`.
template<int&..., types::IsCharArray... Args>
inline constexpr auto MakeLimitedSet(Args... args) noexcept {
  auto result = LimitedSet<std::string_view, sizeof...(Args)>();
  (result.emplace(std::string_view(args)), ...);
  return result;
}

template<types::NotIsCharArray Key, int&..., types::IsCharArray... Args>
inline constexpr auto MakeLimitedSet(Args... args) noexcept {
  auto result = LimitedSet<Key, sizeof...(Args)>();
  (result.emplace(Key(args)), ...);
  return result;
}

// NOLINTBEGIN(*-avoid-c-arrays)
template<typename Key, std::size_t N, typename Compare = std::less<Key>>
constexpr LimitedSet<std::remove_cvref_t<Key>, N, Compare> ToLimitedSet(
    Key (&array)[N],
    const Compare& key_comp = Compare()) {
  LimitedSet<std::remove_cvref_t<Key>, N, Compare> result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<typename Key, std::size_t N, typename Compare = std::less<Key>>
constexpr LimitedSet<std::remove_cvref_t<Key>, N, Compare> ToLimitedSet(
    Key (&&array)[N],
    const Compare& key_comp = Compare()) {
  LimitedSet<std::remove_cvref_t<Key>, N, Compare> result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(std::move(array[idx]));
  }
  return result;
}

// NOLINTEND(*-avoid-c-arrays)

// NOLINTEND(readability-identifier-naming)

#undef LV_REQUIRE

}  // namespace mbo::container

#endif  // MBO_CONTAINER_LIMITED_SET_H_
