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

#ifndef MBO_CONTAINER_LIMITED_MAP_H_
#define MBO_CONTAINER_LIMITED_MAP_H_

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

// Implements a `std::map` like container that only uses inlined memory. So if used as a local
// variable with a types that does not perform memory allocation, then this type does not perform
// any memory allocation.
//
// Unlike `std::array` this type can vary in size.
//
// The type is fully `constexpr` compliant and can be used as a `static constexpr`.
//
// Can be constructed with helpers `MakeLimitedMap` or `ToLimitedMap`.
//
// Example:
//
// ```c++
// using mbo::container::LimitedMap;
//
// constexpr auto kMyData = MakeLimitedMap({1, "1"}, {3, "3"}, {2, "4"});
// ```
//
// The above example infers the value_type to be `std::pair<int, std::string>`
// as it is the common type of the arguments. The resulting `LimitedMap` has a
// capacity of 4 and the elements {{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}}.
//
// Internally a C-array is used and elements are moved as needed. That means
// that element addresses are not stable.
template<
    typename Key,
    typename Value,
    std::size_t Capacity,
    typename KeyComp = std::less<Key>,
    typename ValueComp = std::less<Value>>
requires(std::move_constructible<Key> && std::move_constructible<Value>)
class LimitedMap final {
 public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<const Key, Value>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  struct KeyCompare {
    constexpr ~KeyCompare() noexcept = default;

    constexpr KeyCompare() noexcept = default;

    constexpr explicit KeyCompare(const KeyComp& key_comp) noexcept : key_comp_(key_comp) {}

    constexpr KeyCompare(const KeyCompare&) noexcept = default;
    constexpr KeyCompare& operator=(const KeyCompare&) noexcept = default;
    constexpr KeyCompare(KeyCompare&&) noexcept = default;
    constexpr KeyCompare& operator=(KeyCompare&&) noexcept = default;

    template<typename K>
    constexpr bool operator()(const K& lhs, const value_type& rhs) const noexcept {
      return key_comp_(lhs, rhs.first);
    }

    template<typename K>
    constexpr bool operator()(const value_type& lhs, const K& rhs) const noexcept {
      return key_comp_(lhs.first, rhs);
    }

    template<typename K>
    constexpr bool operator()(const value_type& lhs, const value_type& rhs) const noexcept {
      return key_comp_(lhs.first, rhs.first);
    }

    const KeyComp key_comp_;
  };

  using key_compare = KeyCompare;
  using value_compare = ValueComp;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

 private:
  struct None final {};

  union Data {
    constexpr Data() noexcept : none{} {}

    constexpr Data(const Data&) noexcept = default;
    constexpr Data& operator=(const Data&) noexcept = default;
    constexpr Data(Data&&) noexcept = default;
    constexpr Data& operator=(Data&&) noexcept = default;

    constexpr ~Data() noexcept {}  // NON DEFAULT

    value_type data;
    None none;
  };

  // Must declare each other as friends so that we can correctly move from other.
  template<typename K, typename V, std::size_t OtherN, typename KComp, typename VComp>
  requires(std::move_constructible<K> && std::move_constructible<V>)
  friend class LimitedMap;

 public:
  class const_iterator {
   public:
    using iterator_category = std::contiguous_iterator_tag;
    using difference_type = LimitedMap::difference_type;
    using value_type = LimitedMap::value_type;
    using pointer = LimitedMap::const_pointer;
    using reference = LimitedMap::const_reference;

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

    constexpr const_iterator& operator+=(difference_type ofs) noexcept {
      pos_ += ofs;
      return *this;
    }

    constexpr const_iterator& operator-=(difference_type ofs) noexcept {
      pos_ -= ofs;
      return *this;
    }

    constexpr reference operator[](difference_type ofs) const noexcept {
      return pos_[ofs].data;  // NOLINT(*-pointer-arithmetic)
    }

    friend constexpr difference_type operator-(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ - rhs.pos_;
    }

    friend constexpr difference_type operator+(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ + rhs.pos_;
    }

    friend constexpr const_iterator operator-(const_iterator lhs, difference_type rhs) noexcept {
      return const_iterator(lhs.pos_ - rhs);
    }

    friend constexpr const_iterator operator+(const_iterator lhs, difference_type rhs) noexcept {
      return const_iterator(lhs.pos_ + rhs);
    }

    friend constexpr const_iterator operator-(difference_type lhs, const_iterator rhs) noexcept {
      return const_iterator(lhs - rhs.pos_);
    }

    friend constexpr const_iterator operator+(difference_type lhs, const_iterator rhs) noexcept {
      return const_iterator(lhs + rhs.pos_);
    }

    friend constexpr auto operator<=>(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ <=> rhs.pos_; }

    friend constexpr bool operator<(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ < rhs.pos_; }

    friend constexpr bool operator==(const_iterator lhs, const_iterator rhs) noexcept { return lhs.pos_ == rhs.pos_; }

   private:
    const Data* pos_;
  };

  class iterator {
   public:
    using iterator_category = std::contiguous_iterator_tag;
    using difference_type = LimitedMap::difference_type;
    using value_type = LimitedMap::value_type;
    using pointer = LimitedMap::pointer;
    using reference = LimitedMap::reference;

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

    constexpr iterator& operator+=(difference_type ofs) noexcept {
      pos_ += ofs;
      return *this;
    }

    constexpr iterator& operator-=(difference_type ofs) noexcept {
      pos_ -= ofs;
      return *this;
    }

    constexpr reference operator[](difference_type ofs) const noexcept {
      return pos_[ofs].data;  // NOLINT(*-pointer-arithmetic)
    }

    friend constexpr difference_type operator-(iterator lhs, iterator rhs) noexcept { return lhs.pos_ - rhs.pos_; }

    friend constexpr difference_type operator+(iterator lhs, iterator rhs) noexcept { return lhs.pos_ + rhs.pos_; }

    friend constexpr iterator operator-(iterator lhs, difference_type rhs) noexcept { return iterator(lhs.pos_ - rhs); }

    friend constexpr iterator operator+(iterator lhs, difference_type rhs) noexcept { return iterator(lhs.pos_ + rhs); }

    friend constexpr iterator operator-(difference_type lhs, iterator rhs) noexcept { return iterator(lhs - rhs.pos_); }

    friend constexpr iterator operator+(difference_type lhs, iterator rhs) noexcept { return iterator(lhs + rhs.pos_); }

    friend constexpr auto operator<=>(iterator lhs, iterator rhs) noexcept { return lhs.pos_ <=> rhs.pos_; }

    friend constexpr bool operator<(iterator lhs, iterator rhs) noexcept { return lhs.pos_ < rhs.pos_; }

    friend constexpr bool operator==(iterator lhs, iterator rhs) noexcept { return lhs.pos_ == rhs.pos_; }

    friend std::ostream& operator<<(std::ostream& os, const iterator& pos) {
      return os << "[" << pos.pos_ << " = {" << pos.pos_->data.first << ", " << pos.pos_->data.second << "}]";
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

  constexpr ~LimitedMap() noexcept { clear(); }

  constexpr LimitedMap() noexcept = default;
  constexpr explicit LimitedMap(const KeyComp& key_comp) noexcept : key_comp_(key_comp){};

  constexpr LimitedMap(const LimitedMap& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, other.values_[size_].data);
    }
  }

  constexpr LimitedMap& operator=(const LimitedMap& other) noexcept {
    if (this != &other) {
      clear();
      size_ = other.size_;
      values_ = other.values_;
    }
    return *this;
  }

  constexpr LimitedMap(LimitedMap&& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
  }

  constexpr LimitedMap& operator=(LimitedMap&& other) noexcept {
    clear();
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
    return *this;
  }

  // Constructors and assignment from other LimitVector/value types.

  template<std::forward_iterator It>
  requires std::convertible_to<mbo::types::ForwardIteratorValueType<It>, value_type>
  constexpr LimitedMap(It begin, It end, const KeyComp& key_comp = KeyComp()) noexcept : key_comp_(key_comp) {
    while (begin < end) {
      emplace(*begin);
      ++begin;
    }
  }

  constexpr LimitedMap(const std::initializer_list<value_type>& list, const KeyComp& key_comp = KeyComp()) noexcept
      : key_comp_(key_comp) {
    auto it = list.begin();
    while (it < list.end()) {
      emplace(*it);  // emplace(std::move(*it));
      ++it;
    }
  }

  template<typename U>
  requires(std::convertible_to<U, value_type> && !std::same_as<U, value_type>)
  constexpr LimitedMap(const std::initializer_list<U>& list, const KeyComp& key_comp = KeyComp()) noexcept
      : key_comp_(key_comp) {
    auto it = list.begin();
    while (it < list.end()) {
      emplace(*it);
      ++it;
    }
  }

  template<typename U, std::size_t OtherN>
  requires(std::convertible_to<U, value_type> && OtherN <= Capacity)
  constexpr LimitedMap& operator=(const std::initializer_list<U>& list) noexcept {
    assign(list);
    return *this;
  }

  template<typename K, typename V, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<K, Key> && std::convertible_to<V, Value> && OtherN <= Capacity)
  constexpr explicit LimitedMap(const LimitedMap<K, V, OtherN, OtherCompare>& other) noexcept {
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(it->first, it->second);
    }
  }

  template<typename K, typename V, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<K, Key> && std::convertible_to<V, Value> && OtherN <= Capacity)
  constexpr LimitedMap& operator=(const LimitedMap<K, V, OtherN, OtherCompare>& other) noexcept {
    clear();
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(it->first, it->second);
    }
    return *this;
  }

  template<typename K, typename V, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<K, Key> && std::convertible_to<V, Value> && OtherN <= Capacity)
  constexpr explicit LimitedMap(LimitedMap<K, V, OtherN, OtherCompare>&& other) noexcept {
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(std::move(*it));
    }
    other.size_ = 0;
  }

  template<typename K, typename V, std::size_t OtherN, typename OtherCompare>
  requires(std::convertible_to<K, Key> && std::convertible_to<V, Value> && OtherN <= Capacity)
  constexpr LimitedMap& operator=(LimitedMap<K, V, OtherN, OtherCompare>&& other) noexcept {
    clear();
    for (auto it = other.begin(); it < other.end(); ++it) {
      emplace(std::move(*it));
    }
    other.size_ = 0;
    return *this;
  }

  // Find and search: at, [], lower_bound, upper_bound, equal_range, find, contains, count

  constexpr Value& at(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Out of rage");
    }
    return it->second;
  }

  constexpr const Value& at(const Key& key) const {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Out of rage");
    }
    return it->second;
  }

  constexpr Value& operator[](const Key& key) { return try_emplace(key, mapped_type{}).first->second; }

  constexpr Value& operator[](Key&& key) { return try_emplace(std::move(key), mapped_type{}).first->second; }

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
  template<types::ContainerIsForwardIteratable Other>
  requires(std::equality_comparable_with<typename Other::value_type, Key>)
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
  template<types::ContainerIsForwardIteratable Other>
  requires(std::equality_comparable_with<typename Other::value_type, Key>)
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

  constexpr void swap(LimitedMap& other) noexcept {
    std::size_t pos = 0;
    for (; pos < size_ && pos < other.size(); ++pos) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      std::swap(const_cast<Key&>(values_[pos].data.first), const_cast<Key&>(other.values_[pos].data.first));
      std::swap(values_[pos].data.second, other.values_[pos].data.second);
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
    value_type new_kv(args...);
    const iterator dst = lower_bound(new_kv.first);
    if (dst != end() && !key_comp_(*dst, new_kv.first) && !key_comp_(new_kv.first, *dst)) {
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(&*dst, std::move(new_kv));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<typename... Args>
  constexpr std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(*dst, key) && !key_comp_(key, *dst)) {
      dst->second = Value(args...);
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `try_emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(
        &*dst, std::piecewise_construct, std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<Args>(args)...));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<typename... Args>
  constexpr std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(*dst, key) && !key_comp_(key, *dst)) {
      dst->second = Value(args...);
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `try_emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    // Should possibly use: std::piecewise_construct, std::move(key), std::forward_as_tuple(std::forward<Args>(args)...)
    // But that creates issues with conversion. However, we not the two types, so we do not need piecewise.
    std::construct_at(&*dst, std::move(key), mapped_type(std::forward<Args>(args)...));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<class V>
  constexpr std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(*dst, key) && !key_comp_(key, *dst)) {
      dst->second = std::forward<V>(value);
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `insert_or_assign` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(&*dst, key, std::forward<V>(value));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<class V>
  constexpr std::pair<iterator, bool> insert_or_assign(Key&& key, V&& value) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(*dst, key) && !key_comp_(key, *dst)) {
      dst->second = std::forward<V>(value);
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(&*dst, std::move(key), std::forward<V>(value));
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
      std::construct_at(&*dst, *std::next(dst));
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
      std::construct_at(&*dst, std::move(*src));
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

  template<class P>
  requires(std::constructible_from<value_type, P>)
  constexpr std::pair<iterator, bool> insert(P&& value) {
    return emplace(std::forward<P>(value));
  }

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

  constexpr const_pointer data() const noexcept { return &values_[0].data; }

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
  const KeyCompare key_comp_ = {};
};

template<
    size_t LN,
    size_t RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp,
    typename LVComp,
    typename RVComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline auto operator<=>(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp, LVComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp, RVComp>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = std::tie(lhs_it->first, lhs_it->second) <=> std::tie(rhs_it->first, rhs_it->second);
    if (comp != 0) {
      return comp;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() <=> rhs.size();
}

template<
    size_t LN,
    size_t RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp,
    typename LVComp,
    typename RVComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline bool operator==(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp, LVComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp, RVComp>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = std::tie(lhs_it->first, lhs_it->second) <=> std::tie(rhs_it->first, rhs_it->second);
    if (comp != 0) {
      return false;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() == rhs.size();
}

template<
    size_t LN,
    size_t RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp,
    typename LVComp,
    typename RVComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline bool operator<(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp, LVComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp, RVComp>& rhs) noexcept {
  auto lhs_it = lhs.begin();
  auto rhs_it = rhs.begin();
  while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
    const auto comp = std::tie(lhs_it->first, lhs_it->second) <=> std::tie(rhs_it->first, rhs_it->second);
    if (comp != 0) {
      return comp < 0;
    }
    ++lhs_it;
    ++rhs_it;
  }
  return lhs.size() < rhs.size();
}

template<typename K, typename V, std::size_t N = 0, typename KComp = std::less<K>, typename VComp = std::less<V>>
inline constexpr auto MakeLimitedMap() noexcept {  // Parameter `key_comp` would create a conflict.
  return LimitedMap<K, V, N, KComp, VComp>();
}

template<
    std::size_t N,
    std::forward_iterator It,
    typename KComp = std::less<typename mbo::types::ForwardIteratorValueType<It>::first_type>>
requires(types::IsPair<typename mbo::types::ForwardIteratorValueType<It>>)
inline constexpr auto MakeLimitedMap(It begin, It end, const KComp& key_comp = KComp()) noexcept {
  using KV = mbo::types::ForwardIteratorValueType<It>;
  return LimitedMap<typename KV::first_type, typename KV::second_type, N, KComp>(begin, end, key_comp);
}

template<
    std::size_t N,
    types::IsPair KV,
    typename KComp = std::less<typename KV::first_type>,
    typename VComp = std::less<typename KV::second_type>>
inline constexpr auto MakeLimitedMap(const std::initializer_list<KV>& data, const KComp& key_comp = KComp()) {
  return LimitedMap<typename KV::first_type, typename KV::second_type, N, KComp, VComp>(data, key_comp);
}

template<types::IsPair... Args>
requires((types::NotInitializerList<Args> && !std::forward_iterator<Args>) && ...)
inline constexpr auto MakeLimitedMap(Args&&... args) noexcept {
  using KV = std::common_type_t<Args...>;
  auto result = LimitedMap<typename KV::first_type, typename KV::second_type, sizeof...(Args)>();
  (result.emplace(std::forward<Args>(args)), ...);
  return result;
}

// NOLINTBEGIN(*-avoid-c-arrays)
template<types::IsPair KV, std::size_t N, typename KComp = std::less<typename std::remove_cvref_t<KV>::first_type>>
constexpr auto ToLimitedMap(KV (&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<typename std::remove_cvref_t<KV>::first_type, typename std::remove_cvref_t<KV>::second_type, N, KComp>
      result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<types::IsPair KV, std::size_t N, typename KComp = std::less<typename std::remove_cvref_t<KV>::first_type>>
constexpr auto ToLimitedMap(KV (&&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<typename std::remove_cvref_t<KV>::first_type, typename std::remove_cvref_t<KV>::second_type, N, KComp>
      result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(std::move(array[idx]));
  }
  return result;
}

template<typename K, typename V, std::size_t N, typename KComp = std::less<K>>
requires(!types::IsPair<K>)
constexpr auto ToLimitedMap(std::pair<K, V> (&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<K, V, N, KComp> result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<typename K, typename V, std::size_t N, typename KComp = std::less<K>>
requires(!types::IsPair<K>)
constexpr auto ToLimitedMap(std::pair<K, V> (&&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<K, V, N, KComp> result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(std::move(array[idx]));
  }
  return result;
}

// NOLINTEND(*-avoid-c-arrays)

// NOLINTEND(readability-identifier-naming)

#undef LV_REQUIRE

}  // namespace mbo::container

#endif  // MBO_CONTAINER_LIMITED_MAP_H_
