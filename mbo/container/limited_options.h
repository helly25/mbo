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

#ifndef MBO_CONTAINER_LIMITED_OPTIONS_H_
#define MBO_CONTAINER_LIMITED_OPTIONS_H_

#include <algorithm>
#include <array>
#include <concepts>  // IWYU pragma: keep
#include <type_traits>

namespace mbo::container {

// Flags to be used with `LimitedOptions`. These control specific features of `LimitedSet` and `LimitedMap`.
enum class LimitedOptionsFlag {
  // Empty placeholder: Does not cause a change in behavior..
  kDefault = 0,

  // USE WITH CAUTION: By default the destructor will call `clear`. This allows to prevent that.
  // The only reason this is present is to make ASAN analysis of constexpr vars happy.
  kEmptyDestructor,

  // If true, then the input to constructors must be sorted (only applies to `iterator` and `std::initializer_list`
  // based constructors). This is an optimization for constexpr construction and allows the compiler to handle much
  // larger sets/maps, as the compiler does not need to execute the `emplace` (which requires sorting and shifting) and
  // can instead just place the elements.
  // Note: If `NDEBUG` is defined, then the requirement will not be checked.
  kRequireSortedInput,

  // If true, then do NOT use the optimized `index_of` implementation, and also do not use `index_of` in methods like
  // `find`.
  kNoOptimizeIndexOf,

  // Tf true, then a customized `index_of` implementation will be used beyond loop unrolling. That can be particularly
  // good for some systems in cases where the vast majority of calls to `contains`, `find` or `index_of` are non hits.
  kCustomIndexOfBeyondUnroll,
};

// Type used to control `LimitedSet` and `LimitedMap`.
template<std::size_t Capacity, LimitedOptionsFlag... Flags>
struct LimitedOptions {
  static constexpr std::size_t kCapacity = Capacity;

  static constexpr std::array<LimitedOptionsFlag, sizeof...(Flags)> kFlags{Flags...};

  static constexpr bool Has(LimitedOptionsFlag test_flag) noexcept { return ((test_flag == Flags) || ...); }
};

namespace container_internal {

template<std::size_t Size, std::size_t FlagCount, std::array<LimitedOptionsFlag, FlagCount> Flags, std::size_t... Idx>
constexpr auto MakeLimitedOptionsFromArrayIndices(std::index_sequence<Idx...> /*index_seq*/) {
  return LimitedOptions<Size, Flags.at(Idx)...>{};
}

template<std::size_t Size, std::size_t FlagCount, std::array<LimitedOptionsFlag, FlagCount> Flags>
constexpr auto MakeLimitedOptionsFromArray() {
  return MakeLimitedOptionsFromArrayIndices<Size, FlagCount, Flags>(std::make_index_sequence<FlagCount>());
}

}  // namespace container_internal

template<typename T>
concept IsLimitedOptions = requires(T val) {
  T::kCapacity;
  T::kFlags.size();
  requires std::same_as<std::remove_cvref_t<decltype(val.kCapacity)>, std::size_t>;
  requires std::same_as<std::remove_cvref_t<decltype(val.kFlags.size())>, std::size_t>;
  requires std::same_as<std::remove_cvref_t<decltype(val.kFlags)>, std::array<LimitedOptionsFlag, T::kFlags.size()>>;
  requires std::same_as<
      std::remove_cvref_t<T>,
      decltype(container_internal::MakeLimitedOptionsFromArray<T::kCapacity, T::kFlags.size(), T::kFlags>())>;
};

template<typename T>
concept IsLimitedOptionsOrSize = std::convertible_to<T, std::size_t> || IsLimitedOptions<T>;

template<std::size_t Size, LimitedOptionsFlag... Flags>
constexpr auto MakeLimitedOptions() {
  return LimitedOptions<Size, Flags...>{};
}

template<LimitedOptions Options>
constexpr auto MakeLimitedOptions() {
  return Options;
}

}  // namespace mbo::container

#endif  // MBO_CONTAINER_LIMITED_OPTIONS_H_
