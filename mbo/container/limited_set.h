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
#include <array>
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
// constexpr auto kMyData = MakeLimitedSet(1, 2, 3, 4);
// ```
//
// The above example infers the value_type to be `int` as it is the common type of the arguments.
// The resulting `LimitedSet` has a capacity of 4 and the elements {1, 2, 3, 4}.
template<class Key, std::size_t Capacity, class Compare = std::less<Key>>
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
  template<typename U, std::size_t OtherN, class Comp>
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

  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = LimitedSet::difference_type;
    using value_type        = LimitedSet::value_type;
    using pointer           = LimitedSet::pointer;
    using reference         = LimitedSet::reference;

    iterator() noexcept : pos_(nullptr) {}
    constexpr explicit iterator(Data* pos) noexcept : pos_(pos) {}

    constexpr explicit operator reference () const noexcept { return pos_->data; }
    constexpr reference operator*() const noexcept { return pos_->data; }
    constexpr pointer operator->() const noexcept { return &pos_->data; }

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
    friend constexpr size_type operator-(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ - rhs.pos_;
    }
    friend constexpr size_type operator+(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ + rhs.pos_;
    }
    friend constexpr iterator operator-(iterator lhs, size_type rhs) noexcept {
      return iterator(lhs.pos_ - rhs);
    }
    friend constexpr iterator operator+(iterator lhs, size_type rhs) noexcept {
      return iterator(lhs.pos_ + rhs);
    }
    friend constexpr auto operator<=>(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ <=> rhs.pos_;
    }
    friend constexpr bool operator<(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ < rhs.pos_;
    }
    friend constexpr bool operator==(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ == rhs.pos_;
    }
    friend std::ostream& operator<<(std::ostream& os, const iterator& pos) {
      return os << "[" << pos.pos_ << " = " << pos.pos_->data << "]";
    }
  
  private:
    Data* pos_;
  };

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = LimitedSet::difference_type;
    using value_type        = LimitedSet::value_type;
    using pointer           = LimitedSet::const_pointer;
    using reference         = LimitedSet::const_reference;

    const_iterator() noexcept : pos_(nullptr) {}
    constexpr explicit const_iterator(const Data* pos) noexcept : pos_(pos) {}

    constexpr explicit operator reference () const noexcept { return pos_->data; }
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
    friend constexpr auto operator<=>(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ <=> rhs.pos_;
    }
    friend constexpr bool operator<(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ < rhs.pos_;
    }
    friend constexpr bool operator==(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ == rhs.pos_;
    }

  private:
    const Data* pos_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Destructor and constructors from same type.

  constexpr ~LimitedSet() noexcept { clear(); }

  LimitedSet() noexcept = default;

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
  constexpr LimitedSet(It begin, It end) noexcept {
    while (begin < end) {
      emplace(*begin);
      ++begin;
    }
  }

  template<typename U>
  requires std::convertible_to<U, Key>
  constexpr LimitedSet(std::initializer_list<U>&& list) noexcept {
    auto it = list.begin();
    while (it < list.end()) {
      emplace(std::move(*it));
      ++it;
    }
  }

  template<typename U, std::size_t OtherN>
  requires(std::convertible_to<U, Key> && OtherN <= Capacity)
  constexpr LimitedSet& operator=(std::initializer_list<U>&& list) noexcept {
    assign(std::move(list));
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

  constexpr iterator lower_bound(const Key& key) {
    return std::lower_bound(begin(), end(), key, key_comp_);
  }

  constexpr const_iterator lower_bound(const Key& key) const {
    return std::lower_bound(begin(), end(), key, key_comp_);
  }

  constexpr iterator upper_bound(const Key& key) {
    return std::upper_bound(begin(), end(), key, key_comp_);
  }

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

  constexpr bool contains(const Key& key) const {
    return std::binary_search(begin(), end(), key, key_comp_);
  }

  constexpr std::pair<iterator,iterator> equal_range(const Key& key) {
    return std::equal_range(begin(), end(), key, key_comp_);
  }

  constexpr std::pair<const_iterator,const_iterator> equal_range(const Key& key) const {
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
      std::swap(values_[pos].data, other.values_.at(pos).data);
    }
    std::size_t other_size = other.size_;
    std::size_t this_size = size_;
    for (; pos < size_; ++pos) {
      other.emplace(std::move(values_[pos].data));
    }
    for (; pos < other.size(); ++pos) {
      emplace(std::move(other.values_.at(pos).data));
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

  constexpr std::pair<iterator, bool> insert(const value_type& value) {
    return emplace(value);
  }

  constexpr std::pair<iterator, bool> insert(value_type&& value) {
    return emplace(std::move(value));
  }

  template<class InputIt>
  void insert(InputIt first, InputIt last) {
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

 private:
  std::size_t size_{0};
  std::array<Data, Capacity == 0 ? 1 : Capacity> values_;
  const key_compare key_comp_ = {};
};

template<size_t LN, size_t RN, typename LHS, typename RHS, typename LCompare, typename RCompare>
requires std::three_way_comparable_with<LHS, RHS>
constexpr inline auto operator<=>(const LimitedSet<LHS, LN, LCompare>& lhs, const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
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
constexpr inline bool operator==(const LimitedSet<LHS, LN, LCompare>& lhs, const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
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
constexpr inline bool operator<(const LimitedSet<LHS, LN, LCompare>& lhs, const LimitedSet<RHS, RN, RCompare>& rhs) noexcept {
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

template<typename T, std::size_t N = 0>
inline constexpr auto MakeLimitedSet() noexcept {
  return LimitedSet<T, N>();
}

template<std::size_t N, std::forward_iterator It>
inline constexpr auto MakeLimitedSet(It begin, It end) noexcept {
  return LimitedSet<mbo::types::ForwardIteratorValueType<It>, N>(begin, end);
}

template<std::size_t N, typename T>
inline constexpr auto MakeLimitedSet(std::initializer_list<T>&& data) {
  LV_REQUIRE(FATAL, data.size() <= N) << "Too many initlizer values.";
  return LimitedSet<T, N>(std::forward<std::initializer_list<T>>(data));
}

template<typename... Args>
requires((types::NotInitializerList<Args> && !std::forward_iterator<Args> && !types::IsCharArray<Args>)&& ...)
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

template<types::NotIsCharArray T, int&..., types::IsCharArray... Args>
inline constexpr auto MakeLimitedSet(Args... args) noexcept {
  auto result = LimitedSet<T, sizeof...(Args)>();
  (result.emplace(T(args)), ...);
  return result;
}

// NOLINTBEGIN(*-avoid-c-arrays)
template<typename T, std::size_t N>
constexpr LimitedSet<std::remove_cvref_t<T>, N> ToLimitedSet(T (&array)[N]) {
  LimitedSet<std::remove_cvref_t<T>, N> result;
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<typename T, std::size_t N>
constexpr LimitedSet<std::remove_cvref_t<T>, N> ToLimitedSet(T (&&array)[N]) {
  LimitedSet<std::remove_cvref_t<T>, N> result;
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
