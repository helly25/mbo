// Copyright M. Boerger (helly25.com)
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

#ifndef MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_H_
#define MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_H_

#include <algorithm>
#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <initializer_list>
#include <memory>
#include <new>  // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "mbo/container/internal/limited_ordered_config.h"
#include "mbo/container/limited_options.h"  // IWYU pragma: export
#include "mbo/types/compare.h"              // IWYU pragma: export
#include "mbo/types/traits.h"

#ifdef MBO_FORCE_INLINE
# undef MBO_FORCE_INLINE
#endif
#define MBO_FORCE_INLINE

// __attribute__((always_inline))

#ifdef MBO_ALWAYS_INLINE
# undef MBO_ALWAYS_INLINE
#endif
#define MBO_ALWAYS_INLINE __attribute__((always_inline))

namespace mbo::container::container_internal {

#ifdef LV_REQUIRE
# undef LV_REQUIRE
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LV_REQUIRE(severity, condition)                      \
  /* NOLINTNEXTLINE(bugprone-switch-missing-default-case) */ \
  ABSL_LOG_IF(severity, !(condition))

// NOLINTBEGIN(readability-identifier-naming)

template<typename Key, typename Mapped, typename Value>
concept LimitedOrderedValidImpl =
    std::same_as<std::remove_const_t<Key>, std::remove_const_t<Value>>
    || (mbo::types::IsPair<Value>
        && std::same_as<std::remove_const_t<Key>, std::remove_const_t<typename Value::first_type>>);

template<typename Key, typename Mapped, typename Value>
concept LimitedOrderedValid =  //
    std::move_constructible<std::remove_const_t<Key>> && std::move_constructible<Mapped>
    && LimitedOrderedValidImpl<Key, Mapped, Value>;

template<typename Key, typename Mapped, typename Value, auto options, typename Compare = std::less<Key>>
requires(LimitedOrderedValid<Key, Mapped, Value>)
class LimitedOrdered {
 protected:
  static constexpr bool kKeyOnly = std::same_as<Key, Value>;  // true = set, false = map (of pairs).

  // Size and Options management.
  static_assert(IsLimitedOptionsOrSize<decltype(options)>);
  using Options = decltype(MakeLimitedOptions<options>());
  static_assert(std::is_trivially_destructible_v<Value> || !Options::Has(LimitedOptionsFlag::kEmptyDestructor));
  static constexpr std::size_t Capacity = Options::kCapacity;

  static constexpr bool kOptimizeIndexOf = !Options::Has(LimitedOptionsFlag::kNoOptimizeIndexOf);
  static constexpr bool kCustomIndexOfBeyondUnroll = Options::Has(LimitedOptionsFlag::kCustomIndexOfBeyondUnroll);

  static constexpr std::size_t kUnrollMaxCapacityLimit = 32;                    // The maximum supported in code.
  static constexpr std::size_t kUnrollMaxCapacity = kUnrollMaxCapacityDefault;  // MUST MATCH `index_of`.
  static_assert(kUnrollMaxCapacity >= 4 && kUnrollMaxCapacity <= kUnrollMaxCapacityLimit);

  // Must declare each other as friends so that we can correctly move from other.
  template<typename OK, typename OM, typename OV, auto OtherN, typename Comp>
  requires(LimitedOrderedValid<OK, OM, OV>)
  friend class LimitedOrdered;

  struct None final {};

  union Data {
    constexpr Data() noexcept : none{} {}

    constexpr Data(const Data&) noexcept = default;
    constexpr Data& operator=(const Data&) noexcept = default;
    constexpr Data(Data&&) noexcept = default;
    constexpr Data& operator=(Data&&) noexcept = default;

    constexpr ~Data() noexcept
    requires(std::is_trivially_destructible_v<Value>)
    = default;
    ~Data() noexcept
    requires(!std::is_trivially_destructible_v<Value>)
    {};

    Value data;
    None none;
  };

 public:
  using key_type = Key;
  using value_type = Value;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;
  using reference = Value&;
  using const_reference = const Value&;
  using pointer = Value*;
  using const_pointer = const Value*;

  static constexpr size_type npos = static_cast<size_type>(-1);  // Result of `index_of` if not found.

  struct ValueCompare {
    MBO_ALWAYS_INLINE static constexpr const Key& GetKey(const Key& key) noexcept { return key; }

    MBO_ALWAYS_INLINE static constexpr const Key& GetKey(const value_type& val) noexcept
    requires(!kKeyOnly)
    {
      return val.first;
    }

    MBO_ALWAYS_INLINE static constexpr const Key& GetKey(const Data& data) noexcept {
      if constexpr (kKeyOnly) {
        return data.data;
      } else {
        return data.data.first;
      }
    }

    constexpr ~ValueCompare() noexcept = default;

    constexpr explicit ValueCompare() noexcept = default;

    constexpr explicit ValueCompare(const key_compare& comp) noexcept : key_comp(comp) {}

    constexpr ValueCompare(const ValueCompare&) noexcept = default;
    constexpr ValueCompare& operator=(const ValueCompare&) noexcept = default;
    constexpr ValueCompare(ValueCompare&&) noexcept = default;
    constexpr ValueCompare& operator=(ValueCompare&&) noexcept = default;

    template<typename L, typename R>
    requires(  // clang-format off
      (std::same_as<std::remove_cvref_t<L>, std::remove_cvref_t<Key>> ||
       std::same_as<std::remove_cvref_t<L>, Value> ||
       std::same_as<std::remove_cvref_t<L>, Data>
      ) &&
      (std::same_as<std::remove_cvref_t<R>, std::remove_cvref_t<Key>> ||
       std::same_as<std::remove_cvref_t<R>, Value> ||
       std::same_as<std::remove_cvref_t<R>, Data>)
    )  // clang-format on
    MBO_FORCE_INLINE constexpr bool operator()(const L& lhs, const R& rhs) const noexcept {
      return key_comp(GetKey(lhs), GetKey(rhs));
    }

    const key_compare key_comp;
  };

  // NOTE: The name is misleading but must adhere to the STL: The comparator only
  // compares the `first` part (the key) of mapped values.
  using value_compare = std::conditional_t<kKeyOnly, Compare, ValueCompare>;

  class const_iterator {
   public:
    using iterator_category = std::contiguous_iterator_tag;
    using difference_type = LimitedOrdered::difference_type;
    using value_type = LimitedOrdered::value_type;
    using pointer = LimitedOrdered::const_pointer;
    using reference = LimitedOrdered::const_reference;
    using element_type = LimitedOrdered::value_type;

    constexpr const_iterator() noexcept : pos_(nullptr) {}  // Needed for STL

    MBO_ALWAYS_INLINE constexpr explicit const_iterator(const Data* pos) noexcept : pos_(pos) {}

    MBO_ALWAYS_INLINE constexpr explicit operator reference() const noexcept { return pos_->data; }

    MBO_ALWAYS_INLINE constexpr reference operator*() const noexcept { return pos_->data; }

    MBO_ALWAYS_INLINE constexpr pointer operator->() const noexcept { return &pos_->data; }

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

    MBO_ALWAYS_INLINE friend constexpr auto operator<=>(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ <=> rhs.pos_;
    }

    MBO_ALWAYS_INLINE friend constexpr bool operator<(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ < rhs.pos_;
    }

    MBO_ALWAYS_INLINE friend constexpr bool operator==(const_iterator lhs, const_iterator rhs) noexcept {
      return lhs.pos_ == rhs.pos_;
    }

   private:
    const Data* pos_;
  };

  class iterator {
   public:
    using iterator_category = std::contiguous_iterator_tag;
    using difference_type = LimitedOrdered::difference_type;
    using value_type = LimitedOrdered::value_type;
    using pointer = LimitedOrdered::pointer;
    using reference = LimitedOrdered::reference;
    using element_type = LimitedOrdered::value_type;

    constexpr iterator() noexcept : pos_(nullptr) {}  // Needed for STL

    MBO_ALWAYS_INLINE constexpr explicit iterator(Data* pos) noexcept : pos_(pos) {}

    MBO_ALWAYS_INLINE constexpr explicit operator reference() const noexcept { return pos_->data; }

    MBO_ALWAYS_INLINE constexpr reference operator*() const noexcept { return pos_->data; }

    MBO_ALWAYS_INLINE constexpr pointer operator->() const noexcept { return &pos_->data; }

    MBO_ALWAYS_INLINE constexpr explicit operator const_iterator() const noexcept { return const_iterator(pos_); }

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

    MBO_ALWAYS_INLINE friend constexpr auto operator<=>(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ <=> rhs.pos_;
    }

    MBO_ALWAYS_INLINE friend constexpr bool operator<(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ < rhs.pos_;
    }

    MBO_ALWAYS_INLINE friend constexpr bool operator==(iterator lhs, iterator rhs) noexcept {
      return lhs.pos_ == rhs.pos_;
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

  constexpr ~LimitedOrdered() noexcept
  requires(Options::Has(LimitedOptionsFlag::kEmptyDestructor))
  = default;

  constexpr ~LimitedOrdered() noexcept
  requires(!Options::Has(LimitedOptionsFlag::kEmptyDestructor) && std::is_trivially_destructible_v<Value>)
  {
    clear();
  }

  ~LimitedOrdered() noexcept
  requires(!Options::Has(LimitedOptionsFlag::kEmptyDestructor) && !std::is_trivially_destructible_v<Value>)
  {
    clear();
  }

  constexpr LimitedOrdered() noexcept = default;
  constexpr explicit LimitedOrdered(const Compare& key_comp) noexcept : key_comp_(key_comp){};

  constexpr LimitedOrdered(const LimitedOrdered& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, other.values_[size_].data);
    }
  }

  constexpr LimitedOrdered& operator=(const LimitedOrdered& other) noexcept {
    if (this != &other) {
      clear();
      size_ = other.size_;
      values_ = other.values_;
    }
    return *this;
  }

  constexpr LimitedOrdered(LimitedOrdered&& other) noexcept {
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
  }

  constexpr LimitedOrdered& operator=(LimitedOrdered&& other) noexcept {
    clear();
    for (; size_ < other.size_; ++size_) {
      std::construct_at(&values_[size_].data, std::move(other.values_[size_].data));
    }
    other.size_ = 0;
    return *this;
  }

  // Constructors and assignment from other LimitVector/value types.

  template<std::forward_iterator It>
  requires std::convertible_to<mbo::types::ForwardIteratorValueType<It>, Value>
  constexpr LimitedOrdered(It first, It last, const Compare& key_comp = Compare()) noexcept : key_comp_(key_comp) {
    if constexpr (Options::Has(LimitedOptionsFlag::kRequireSortedInput)) {
      LV_REQUIRE(FATAL, std::is_sorted(first, last, key_comp_));
    }
    while (first < last) {
      if constexpr (Options::Has(LimitedOptionsFlag::kRequireSortedInput)) {
        std::construct_at(&values_[size_++].data, *first);
      } else {
        emplace(*first);
      }
      ++first;
    }
  }

  constexpr LimitedOrdered(const std::initializer_list<value_type>& list, const Compare& key_comp = Compare()) noexcept
      : LimitedOrdered(list.begin(), list.end(), key_comp) {}

  template<typename U>
  requires(std::convertible_to<U, value_type> && !std::same_as<U, value_type>)
  constexpr LimitedOrdered(const std::initializer_list<U>& list, const Compare& key_comp = Compare()) noexcept
      : LimitedOrdered(list.begin(), list.end(), key_comp) {}

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, value_type> && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr LimitedOrdered& operator=(const std::initializer_list<U>& list) noexcept {
    assign(list);
    return *this;
  }

  template<typename OK, typename OM, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OM, Mapped>
      && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr explicit LimitedOrdered(const LimitedOrdered<OK, OM, OV, OtherN, OtherCompare>& other) noexcept {
    for (auto it = other.begin(); it < other.end(); ++it) {
      if constexpr (kKeyOnly) {
        emplace(*it);
      } else {
        emplace(it->first, it->second);
      }
    }
  }

  template<typename OK, typename OM, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OM, Mapped>
      && MakeLimitedOptions<OtherN>().kCapacity <= Capacity)
  constexpr LimitedOrdered& operator=(const LimitedOrdered<OK, OM, OV, OtherN, OtherCompare>& other) noexcept {
    clear();
    for (auto it = other.begin(); it < other.end(); ++it) {
      if constexpr (kKeyOnly) {
        emplace(*it);
      } else {
        emplace(it->first, it->second);
      }
    }
    return *this;
  }

  // Find and search: lower_bound, upper_bound, equal_range, find, contains, count

  MBO_FORCE_INLINE constexpr iterator lower_bound(const Key& key) {
    return std::lower_bound(begin(), end(), key, val_comp_);
  }

  MBO_FORCE_INLINE constexpr const_iterator lower_bound(const Key& key) const {
    return std::lower_bound(begin(), end(), key, val_comp_);
  }

  MBO_FORCE_INLINE constexpr iterator upper_bound(const Key& key) {
    return std::upper_bound(begin(), end(), key, val_comp_);
  }

  MBO_FORCE_INLINE constexpr const_iterator upper_bound(const Key& key) const {
    return std::upper_bound(begin(), end(), key, val_comp_);
  }

  // NOLINTBEGIN(*-magic-numbers,*-macro-usage,*-function-size,readability-function-cognitive-complexity)
  MBO_ALWAYS_INLINE constexpr std::size_t index_of(const Key& key) const
  requires(kOptimizeIndexOf && Capacity <= kUnrollMaxCapacity)
  {
#define MBO_CASE_LIMITED_POS_COMP(POS)                                     \
  static_assert((POS) + 1 <= kUnrollMaxCapacityLimit);                     \
  case ((POS) + 1):                                                        \
    if constexpr ((POS) >= Capacity) {                                     \
      return npos;                                                         \
    } else if constexpr (mbo::types::IsCompareLess<Compare>) {             \
      const auto comp = key_comp_.Compare(key, GetKey(values_[POS].data)); \
      if (comp >= 0) [[unlikely]] {                                        \
        if (comp > 0) [[likely]] {                                         \
          return npos;                                                     \
        } else [[unlikely]] {                                              \
          return (POS);                                                    \
        }                                                                  \
      }                                                                    \
    } else {                                                               \
      if (!key_comp_(key, GetKey(values_[POS].data))) [[unlikely]] {       \
        if (key_comp_(GetKey(values_[POS].data), key)) [[likely]] {        \
          return npos;                                                     \
        } else [[unlikely]] {                                              \
          return POS;                                                      \
        }                                                                  \
      }                                                                    \
    }                                                                      \
    [[fallthrough]]
    switch (size_) {
      MBO_CASE_LIMITED_POS_COMP(31);
      MBO_CASE_LIMITED_POS_COMP(30);
      MBO_CASE_LIMITED_POS_COMP(29);
      MBO_CASE_LIMITED_POS_COMP(28);
      MBO_CASE_LIMITED_POS_COMP(27);
      MBO_CASE_LIMITED_POS_COMP(26);
      MBO_CASE_LIMITED_POS_COMP(25);
      MBO_CASE_LIMITED_POS_COMP(24);
      MBO_CASE_LIMITED_POS_COMP(23);
      MBO_CASE_LIMITED_POS_COMP(22);
      MBO_CASE_LIMITED_POS_COMP(21);
      MBO_CASE_LIMITED_POS_COMP(20);
      MBO_CASE_LIMITED_POS_COMP(19);
      MBO_CASE_LIMITED_POS_COMP(18);
      MBO_CASE_LIMITED_POS_COMP(17);
      MBO_CASE_LIMITED_POS_COMP(16);
      MBO_CASE_LIMITED_POS_COMP(15);
      MBO_CASE_LIMITED_POS_COMP(14);
      MBO_CASE_LIMITED_POS_COMP(13);
      MBO_CASE_LIMITED_POS_COMP(12);
      MBO_CASE_LIMITED_POS_COMP(11);
      MBO_CASE_LIMITED_POS_COMP(10);
      MBO_CASE_LIMITED_POS_COMP(9);
      MBO_CASE_LIMITED_POS_COMP(8);
      MBO_CASE_LIMITED_POS_COMP(7);
      MBO_CASE_LIMITED_POS_COMP(6);
      MBO_CASE_LIMITED_POS_COMP(5);
      MBO_CASE_LIMITED_POS_COMP(4);
      MBO_CASE_LIMITED_POS_COMP(3);
      MBO_CASE_LIMITED_POS_COMP(2);
      MBO_CASE_LIMITED_POS_COMP(1);
      MBO_CASE_LIMITED_POS_COMP(0);
      default: break;  // Handles `size_ == 0`.
    }
#undef MBO_CASE_LIMITED_POS_COMP
    return npos;
  }  // NOLINTEND(*-magic-numbers,*-macro-usage,*-function-size,readability-function-cognitive-complexity)

  MBO_ALWAYS_INLINE constexpr std::size_t index_of(const Key& key) const
  requires(
      kOptimizeIndexOf && kCustomIndexOfBeyondUnroll
      && mbo::types::IsCompareLess<Compare> && Capacity > kUnrollMaxCapacity)
  {
    std::size_t left = 0;
    std::size_t right = size_;
    while (left < right) [[likely]] {
      const std::size_t pos = left + ((right - left) >> 1U);
      auto cmp = key_comp_.Compare(key, GetKey(values_[pos]));
      if (cmp < 0) [[unlikely]] {
        right = pos;
      } else if (cmp > 0) {
        left = pos + 1;
      } else {
        return pos;  // == 0
      }
    }
    return npos;
  }

  MBO_ALWAYS_INLINE std::size_t index_of(const Key& key) const
  requires(
      kOptimizeIndexOf && kCustomIndexOfBeyondUnroll
      && !mbo::types::IsCompareLess<Compare> && Capacity > kUnrollMaxCapacity)
  {
    if (size_ == 0) {
      return npos;
    }
    std::size_t left = 0;
    std::size_t right = size_;
    while (true) {
      const std::size_t diff = (right - left) >> 1U;
      const std::size_t pos = left + diff;
      if (key_comp_(key, GetKey(values_[pos]))) [[likely]] {
        if (diff == 0) [[unlikely]] {
          return npos;
        }
        right = pos;
      } else {
        if (diff == 0) [[unlikely]] {
          if (key_comp_(GetKey(values_[left]), key)) [[likely]] {
            return npos;
          } else [[unlikely]] {
            return left;
          }
        }
        left = pos;
      }
    }
  }

  MBO_ALWAYS_INLINE constexpr std::size_t index_of(const Key& key) const
  requires(!kOptimizeIndexOf || (kOptimizeIndexOf && !kCustomIndexOfBeyondUnroll && Capacity > kUnrollMaxCapacity))
  {
    const_iterator it = lower_bound(key);
    return it == end() || key_comp_(key, GetKey(*it)) ? npos : it - begin();
  }

  MBO_FORCE_INLINE constexpr value_type& at_index(size_type pos) {
    if (pos >= size_) {
      throw std::out_of_range("Out of rage");
    }
    return values_[pos].data;
  }

  MBO_FORCE_INLINE constexpr const value_type& at_index(size_type pos) const {
    if (pos >= size_) {
      throw std::out_of_range("Out of rage");
    }
    return values_[pos].data;
  }

  MBO_FORCE_INLINE constexpr iterator find(const Key& key) {
    if constexpr (kOptimizeIndexOf) {
      std::size_t pos = index_of(key);
      return pos == npos ? end() : iterator(&values_[pos]);
    } else {  // Not kOptimizeIndexOf
      iterator it = lower_bound(key);
      return it == end() || key_comp_(key, GetKey(*it)) ? end() : it;
    }
  }

  MBO_FORCE_INLINE constexpr const_iterator find(const Key& key) const {
    if constexpr (kOptimizeIndexOf) {
      std::size_t pos = index_of(key);
      return pos == npos ? end() : const_iterator(&values_[pos]);
    } else {  // Not kOptimizeIndexOf
      const_iterator it = lower_bound(key);
      return it == end() || key_comp_(key, GetKey(*it)) ? end() : it;
    }
  }

  MBO_FORCE_INLINE constexpr bool contains(const Key& key) const {
    if constexpr (kOptimizeIndexOf) {
      return index_of(key) != npos;
    } else {
      return std::binary_search(begin(), end(), key, val_comp_);
    }
  }

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
    return std::equal_range(begin(), end(), key, val_comp_);
  }

  constexpr std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
    return std::equal_range(begin(), end(), key, val_comp_);
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

  template<typename OK, typename OM, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OM, Mapped>
      && MakeLimitedOptions<OtherN>().kCapacity == Capacity && std::same_as<OtherCompare, Compare>)
  constexpr void swap(LimitedOrdered<OK, OM, OV, OtherN, OtherCompare>& other) noexcept {
    std::size_t pos = 0;
    for (; pos < size_ && pos < other.size(); ++pos) {
      if constexpr (kKeyOnly) {
        std::swap(values_[pos].data, other.values_[pos].data);
      } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        std::swap(const_cast<Key&>(values_[pos].data.first), const_cast<Key&>(other.values_[pos].data.first));
        std::swap(values_[pos].data.second, other.values_[pos].data.second);
      }
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
    Value new_val(std::forward<Args>(args)...);
    const iterator dst = lower_bound(GetKey(new_val));
    if (dst != end() && !key_comp_(GetKey(*dst), GetKey(new_val)) && !key_comp_(GetKey(new_val), GetKey(*dst))) {
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    std::construct_at(&*dst, std::move(new_val));
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
      std::construct_at(&*dst, std::move(*std::next(dst)));
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
      auto pos = find(std::forward<K>(key));
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

  // Map-types only

  template<typename... Args>
  requires(!kKeyOnly)
  constexpr std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(dst->first, key) && !key_comp_(key, dst->first)) {
      dst->second = Mapped(args...);
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
  requires(!kKeyOnly)
  constexpr std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(dst->first, key) && !key_comp_(key, dst->first)) {
      dst->second = Mapped(args...);
      return std::make_pair(dst, false);
    }
    LV_REQUIRE(FATAL, size_ < Capacity) << "Called `try_emplace` at capacity.";
    for (iterator next = end(); next > dst; --next) {
      std::construct_at(&*next, std::move(*std::prev(next)));
    }
    // Should possibly use: std::piecewise_construct, std::move(key), std::forward_as_tuple(std::forward<Args>(args)...)
    // But that creates issues with conversion. However, we know the two types, so we do not need piecewise.
    std::construct_at(&*dst, std::move(key), Mapped(std::forward<Args>(args)...));
    ++size_;
    return std::make_pair(dst, true);
  }

  template<class V>
  requires(!kKeyOnly)
  constexpr std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(dst->first, key) && !key_comp_(key, dst->first)) {
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
  requires(!kKeyOnly)
  constexpr std::pair<iterator, bool> insert_or_assign(Key&& key, V&& value) {
    const iterator dst = lower_bound(key);
    if (dst != end() && !key_comp_(dst->first, key) && !key_comp_(key, dst->first)) {
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

  // Read/write access

  MBO_FORCE_INLINE constexpr std::size_t size() const noexcept { return size_; }

  constexpr std::size_t max_size() const noexcept { return Capacity; }

  constexpr std::size_t capacity() const noexcept { return Capacity; }

  constexpr bool empty() const noexcept { return size_ == 0; }

  // NOLINTBEGIN(readability-container-data-pointer)

  constexpr iterator begin() noexcept { return iterator(&values_[0]); }

  constexpr const_iterator begin() const noexcept { return const_iterator(&values_[0]); }

  constexpr const_iterator cbegin() const noexcept { return const_iterator(&values_[0]); }

  MBO_FORCE_INLINE constexpr iterator end() noexcept { return iterator(&values_[size_]); }

  MBO_FORCE_INLINE constexpr const_iterator end() const noexcept { return const_iterator(&values_[size_]); }

  MBO_FORCE_INLINE constexpr const_iterator cend() const noexcept { return const_iterator(&values_[size_]); }

  constexpr reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }

  constexpr const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(end()); }

  MBO_FORCE_INLINE constexpr reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }

  MBO_FORCE_INLINE constexpr const_reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(begin());
  }

  MBO_FORCE_INLINE constexpr const_reverse_iterator crend() const noexcept {
    return std::make_reverse_iterator(cbegin());
  }

  constexpr const_pointer data() const noexcept { return &values_[0].data; }

  // NOLINTEND(readability-container-data-pointer)

  // Observers

  constexpr key_compare key_comp() const noexcept { return key_comp; }

  constexpr value_compare value_comp() const noexcept { return val_comp_; }

 protected:
  static constexpr iterator to_iterator(iterator pos) noexcept { return pos; }

  static constexpr iterator to_iterator(const const_iterator& pos) noexcept { return iterator(pos.pos); }

  static constexpr const Key& GetKey(const Key& key) noexcept { return key; }

  static constexpr const Key& GetKey(const Value& val) noexcept
  requires(!kKeyOnly)
  {
    return val.first;
  }

  MBO_ALWAYS_INLINE static constexpr const Key& GetKey(const Data& data) noexcept {
    if constexpr (kKeyOnly) {
      return data.data;
    } else {
      return data.data.first;
    }
  }

 private:
  std::size_t size_{0};

  // Array would be better but that does not work with ASAN builds.
  // std::array<Data, Capacity == 0 ? 1 : Capacity> values_;
  Data values_[Capacity == 0 ? 1 : Capacity];  // NOLINT(*-avoid-c-arrays)

  const key_compare key_comp_ = {};
  const value_compare val_comp_ = value_compare(key_comp_);
};

// NOLINTEND(readability-identifier-naming)

#undef LV_REQUIRE

}  // namespace mbo::container::container_internal

#ifdef MBO_FORCE_INLINE
# undef MBO_FORCE_INLINE
#endif

#ifdef MBO_ALWAYS_INLINE
# undef MBO_ALWAYS_INLINE
#endif

#endif  // MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_H_
