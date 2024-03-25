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

#ifndef MBO_CONTAINER_LIMITED_MAP_H_
#define MBO_CONTAINER_LIMITED_MAP_H_

#include <algorithm>
#include <compare>   // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <initializer_list>
#include <memory>  // IWYU pragma: keep
#include <new>     // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "mbo/container/internal/limited_ordered.h"
#include "mbo/types/compare.h"
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
template<typename Key, typename Value, auto CapacityOrOptions, typename KeyComp = std::less<Key>>
requires(std::move_constructible<Value>)
class LimitedMap final
    : public container_internal::LimitedOrdered<Key, Value, std::pair<const Key, Value>, CapacityOrOptions, KeyComp> {
  using KeyValueType = std::pair<const Key, Value>;
  static constexpr std::size_t kCapacity = MakeLimitedOptions<CapacityOrOptions>().kCapacity;
  using LimitedBase = container_internal::LimitedOrdered<Key, Value, KeyValueType, CapacityOrOptions, KeyComp>;
  static_assert(!LimitedBase::kKeyOnly);

 public:
  using mapped_type = Value;

  // Destructor and constructors from same type.

  using container_internal::LimitedOrdered<Key, Value, std::pair<const Key, Value>, CapacityOrOptions, KeyComp>::
      LimitedOrdered;

  constexpr ~LimitedMap() noexcept = default;

  constexpr LimitedMap() noexcept = default;

  constexpr explicit LimitedMap(const KeyComp& key_comp) noexcept : LimitedBase(key_comp){};

  constexpr LimitedMap(const LimitedMap& other) noexcept : LimitedBase(other) {}

  constexpr LimitedMap& operator=(const LimitedMap& other) noexcept {
    if (this != &other) {
      LimitedBase::operator=(other);
    }
    return *this;
  }

  constexpr LimitedMap(LimitedMap&& other) noexcept : LimitedBase(std::move(other)) {}

  constexpr LimitedMap& operator=(LimitedMap&& other) noexcept {
    if (this != &other) {
      LimitedBase::operator=(std::move(other));
    }
    return *this;
  }

  // Constructors and assignment from other LimitMap/value types.

  template<std::forward_iterator It>
  requires std::convertible_to<mbo::types::ForwardIteratorValueType<It>, KeyValueType>
  constexpr LimitedMap(It begin, It end, const KeyComp& key_comp = KeyComp()) noexcept
      : LimitedBase(begin, end, key_comp) {}

  constexpr LimitedMap(const std::initializer_list<KeyValueType>& list, const KeyComp& key_comp = KeyComp()) noexcept
      : LimitedBase(list, key_comp) {}

  template<typename U>
  requires(std::convertible_to<U, KeyValueType> && !std::same_as<U, KeyValueType>)
  constexpr LimitedMap(const std::initializer_list<U>& list, const KeyComp& key_comp = KeyComp()) noexcept
      : LimitedBase(list, key_comp) {}

  template<typename U, auto OtherN>
  requires(std::convertible_to<U, KeyValueType> && MakeLimitedOptions<OtherN>().kCapacity <= kCapacity)
  constexpr LimitedMap& operator=(const std::initializer_list<U>& list) noexcept {
    LimitedBase::operator=(list);
    return *this;
  }

  template<typename OK, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OV, Value>
      && MakeLimitedOptions<OtherN>().kCapacity <= kCapacity)
  constexpr explicit LimitedMap(const LimitedMap<OK, OV, OtherN, OtherCompare>& other) noexcept : LimitedBase(other) {}

  template<typename OK, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OV, Value>
      && MakeLimitedOptions<OtherN>().kCapacity <= kCapacity)
  constexpr LimitedMap& operator=(const LimitedMap<OK, OV, OtherN, OtherCompare>& other) noexcept {
    LimitedBase::operator=(other);
    return *this;
  }

  template<typename OK, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OV, Value>
      && MakeLimitedOptions<OtherN>().kCapacity <= kCapacity)
  constexpr explicit LimitedMap(LimitedMap<OK, OV, OtherN, OtherCompare>&& other) noexcept
      : LimitedBase(std::move(other)) {}

  template<typename OK, typename OV, auto OtherN, typename OtherCompare>
  requires(
      std::convertible_to<OK, Key> && std::convertible_to<OV, Value>
      && MakeLimitedOptions<OtherN>().kCapacity <= kCapacity)
  constexpr LimitedMap& operator=(LimitedMap<OK, OV, OtherN, OtherCompare>&& other) noexcept {
    LimitedBase::operator=(std::move(other));
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

  using LimitedBase::contains;
  using LimitedBase::contains_all;
  using LimitedBase::contains_any;
  using LimitedBase::count;
  using LimitedBase::equal_range;
  using LimitedBase::find;
  using LimitedBase::lower_bound;
  using LimitedBase::upper_bound;

  // Mofification: clear, swap, emplace, insert

  using LimitedBase::clear;
  using LimitedBase::emplace;
  using LimitedBase::erase;
  using LimitedBase::insert;
  using LimitedBase::swap;

  // Map-types only
  using LimitedBase::insert_or_assign;
  using LimitedBase::try_emplace;

  // Custom

  using LimitedBase::at_index;  // Index based access, throws `std::out_of_range` if not found.
  using LimitedBase::index_of;  // Return 0-based index of `key`, or `npos` if not found.
  using LimitedBase::npos;      // Return value for `index_of` if `key` is not found.

  // Read/write access

  using LimitedBase::capacity;
  using LimitedBase::empty;
  using LimitedBase::max_size;
  using LimitedBase::size;

  using LimitedBase::begin;
  using LimitedBase::cbegin;
  using LimitedBase::cend;
  using LimitedBase::crbegin;
  using LimitedBase::crend;
  using LimitedBase::data;
  using LimitedBase::end;
  using LimitedBase::rbegin;
  using LimitedBase::rend;

  // Observers

  using LimitedBase::key_comp;
  using LimitedBase::value_comp;
};

template<
    auto LN,
    auto RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline auto operator<=>(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp>& rhs) noexcept {
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
    auto LN,
    auto RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline bool operator==(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp>& rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
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
  return true;
}

template<
    auto LN,
    auto RN,
    typename LHS_K,
    typename RHS_K,
    typename LHS_V,
    typename RHS_V,
    typename LKComp,
    typename RKComp>
requires(std::three_way_comparable_with<LHS_K, RHS_K> && std::three_way_comparable_with<LHS_V, RHS_V>)
constexpr inline bool operator<(
    const LimitedMap<LHS_K, LHS_V, LN, LKComp>& lhs,
    const LimitedMap<RHS_K, RHS_V, RN, RKComp>& rhs) noexcept {
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

template<typename K, typename V, auto N = 0, typename KComp = types::CompareLess<K>>
inline constexpr auto MakeLimitedMap() noexcept {  // Parameter `key_comp` would create a conflict.
  return LimitedMap<K, V, N, KComp>();
}

template<
    auto N,
    std::forward_iterator It,
    typename KComp = types::CompareLess<typename mbo::types::ForwardIteratorValueType<It>::first_type>>
requires(types::IsPair<typename mbo::types::ForwardIteratorValueType<It>>)
inline constexpr auto MakeLimitedMap(It begin, It end, const KComp& key_comp = KComp()) noexcept {
  using KV = mbo::types::ForwardIteratorValueType<It>;
  return LimitedMap<typename KV::first_type, typename KV::second_type, N, KComp>(begin, end, key_comp);
}

template<auto N, types::IsPair KV, typename KComp = types::CompareLess<typename KV::first_type>>
inline constexpr auto MakeLimitedMap(const std::initializer_list<KV>& data, const KComp& key_comp = KComp()) {
  return LimitedMap<typename KV::first_type, typename KV::second_type, N, KComp>(data, key_comp);
}

template<types::IsPair... Args>
requires(sizeof...(Args) > 0)
inline constexpr auto MakeLimitedMap(Args&&... args) noexcept {
  using KV = std::common_type_t<Args...>;
  auto result = LimitedMap<typename KV::first_type, typename KV::second_type, sizeof...(Args)>();
  (result.emplace(std::forward<Args>(args)), ...);
  return result;
}

template<types::IsPair KV>
inline constexpr auto MakeLimitedMap() noexcept {
  auto result = LimitedMap<typename KV::first_type, typename KV::second_type, 0>();
  return result;
}

// NOLINTBEGIN(*-avoid-c-arrays)
template<
    types::IsPair KV,
    std::size_t N,
    typename KComp = types::CompareLess<typename std::remove_cvref_t<KV>::first_type>>
constexpr auto ToLimitedMap(KV (&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<typename std::remove_cvref_t<KV>::first_type, typename std::remove_cvref_t<KV>::second_type, N, KComp>
      result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<
    types::IsPair KV,
    std::size_t N,
    typename KComp = types::CompareLess<typename std::remove_cvref_t<KV>::first_type>>
constexpr auto ToLimitedMap(KV (&&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<typename std::remove_cvref_t<KV>::first_type, typename std::remove_cvref_t<KV>::second_type, N, KComp>
      result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(std::move(array[idx]));
  }
  return result;
}

template<typename K, typename V, std::size_t N, typename KComp = types::CompareLess<K>>
requires(!types::IsPair<K>)
constexpr auto ToLimitedMap(std::pair<K, V> (&array)[N], const KComp& key_comp = KComp()) {
  LimitedMap<K, V, N, KComp> result(key_comp);
  for (std::size_t idx = 0; idx < N; ++idx) {
    result.emplace(array[idx]);
  }
  return result;
}

template<typename K, typename V, std::size_t N, typename KComp = types::CompareLess<K>>
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
