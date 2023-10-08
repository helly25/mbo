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

#ifndef MBO_CONTAINER_LIMITED_OPTIONS_H_
#define MBO_CONTAINER_LIMITED_OPTIONS_H_

#include <concepts>
#include <type_traits>

namespace mbo::container {

// Type used to control `LimitedSet` anf `LimitedMap`.
template <std::size_t Capacity, bool EmptyDestructor = false>
struct LimitedOptions {
  static constexpr std::size_t kCapacity = Capacity;

  // USE WITH CAUTION: By default the destructor will call `clear`. This allows to prevent that.
  // The only reason this is present is to make ASAN analysis of constexpr vars happy.
  static constexpr bool kEmptyDestructor = EmptyDestructor;
};

template <typename T>
concept IsLimitedOptions = requires(T val) {
  { val.kCapacity } -> std::convertible_to<std::size_t>;
  { val.kEmptyDestructor } -> std::convertible_to<bool>;
  requires std::same_as<std::remove_cvref_t<decltype(val.kCapacity)>, std::size_t>;
  requires std::same_as<std::remove_cvref_t<decltype(val.kEmptyDestructor)>, bool>;
  requires std::same_as<std::remove_cvref_t<T>, LimitedOptions<T::kCapacity, T::kEmptyDestructor>>;
};

template <typename T>
concept IsLimitedOptionsOrSize = std::convertible_to<T, std::size_t> || IsLimitedOptions<T>;

template<std::size_t Size, bool EmptyDestructor = false>
constexpr auto MakeLimitedOptions() {
  return LimitedOptions<Size, EmptyDestructor>{};
};

template<LimitedOptions Options>
constexpr auto MakeLimitedOptions() {
  return Options;
};

}  // namespace mbo::container

#endif  // MBO_CONTAINER_LIMITED_OPTIONS_H_
