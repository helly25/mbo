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

#ifndef MBO_TYPES_INTERNAL_DECOMPOSE_COUNT_H_
#define MBO_TYPES_INTERNAL_DECOMPOSE_COUNT_H_

// IWYU pragma private, include "mbo/types/traits.h"

#include <limits>
#include <string>       // IWYU pragma: keep
#include <string_view>  // IWYU pragma: keep
#include <tuple>
#include <type_traits>
#include <utility>  // IWYU pragma: keep

#include "mbo/types/internal/binary_search.h"  // IWYU pragma: keep
#include "mbo/types/internal/cases.h"          // IWYU pragma: keep
#include "mbo/types/internal/is_braces_constructible.h"

namespace mbo::types::types_internal {

// NOLINTBEGIN(*-magic-numbers)

template<typename T>
concept IsAggregate = std::is_aggregate_v<std::remove_cvref_t<T>>;

struct NotDecomposableImpl : std::integral_constant<std::size_t, std::numeric_limits<std::size_t>::max()> {};

inline constexpr std::size_t kMaxSupportedFieldCount = 50;

template<typename T, std::size_t MaxArgs, typename... Args>
constexpr std::size_t StructCtorArgCountMaxFunc() {
  if constexpr (sizeof...(Args) > MaxArgs) {  // NOLINT(bugprone-branch-clone)
    return 0;
  } else if constexpr (constexpr std::size_t kCount = StructCtorArgCountMaxFunc<T, MaxArgs, Args..., AnyType>();
                       kCount) {
    return kCount;
  } else if constexpr (IsBracesConstructibleImplT<T, Args...>::value) {
    return sizeof...(Args);
  } else {
    return 0;
  }
}

template<typename T>
struct StructCtorArgCountMaxImpl
    : std::integral_constant<
          std::size_t,
          std::is_aggregate_v<T> ? StructCtorArgCountMaxFunc<T, kMaxSupportedFieldCount>() : 0> {};

template<typename T>
struct StructCtorArgCountMaxT : StructCtorArgCountMaxImpl<std::remove_cvref_t<T>> {};

template<typename T>
inline constexpr std::size_t StructCtorArgCountMaxV = StructCtorArgCountMaxT<T>::value;

template<typename T, bool kRequireNonEmpty>
struct AggregateHasBaseRaw
    : std::bool_constant<
          !std::is_empty_v<T> && std::is_aggregate_v<T>
              ? StructCtorArgCountMaxFunc<T, kMaxSupportedFieldCount, AnyTypeIf<T, true, kRequireNonEmpty>>() != 0
              : false> {};

template<typename T>
struct AggregateHasNonEmptyBaseRaw : AggregateHasBaseRaw<T, true> {};

template<typename T>
struct AggregateHasNonEmptyBaseImpl : AggregateHasNonEmptyBaseRaw<std::remove_cvref_t<T>> {};

// Crude aproximation of whether a `T` has a non empty base struct/class.
//
// NOTE: The behavior is undefined when using this on non aggregate types.
template<typename T>
concept AggregateHasNonEmptyBase = AggregateHasNonEmptyBaseImpl<T>::value;

#if defined(__clang__) && __clang_major__ < 17
// ----------------------------------------------------

template<typename T, size_t N, typename = void>
struct DecomposeCount final : std::false_type {};

template<typename T>
struct DecomposeCount<T, 1, std::void_t<decltype([]() { const auto& [a1] = T(); })>> final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 2, std::void_t<decltype([]() { const auto& [a1, a2] = T(); })>> final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 3, std::void_t<decltype([]() { const auto& [a1, a2, a3] = T(); })>> final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 4, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4] = T(); })>> final
    : std::true_type {};

template<typename T>
struct DecomposeCount<T, 5, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4, a5] = T(); })>> final
    : std::true_type {};

template<typename T>
struct DecomposeCount<T, 6, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4, a5, a6] = T(); })>> final
    : std::true_type {};

template<typename T>
struct DecomposeCount<T, 7, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4, a5, a6, a7] = T(); })>> final
    : std::true_type {};

template<typename T>
struct DecomposeCount<T, 8, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4, a5, a6, a7, a8] = T(); })>> final
    : std::true_type {};

template<typename T>
struct DecomposeCount<T, 9, std::void_t<decltype([]() { const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = T(); })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 10, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 11, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 12, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 13, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 14, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 15, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 16, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 17, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<T, 18, std::void_t<decltype([]() {
                        const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] =
                            T();
                      })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    19,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    20,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    21,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    22,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    23,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    24,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    25,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    26,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    27,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    28,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    29,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    30,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    31,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    32,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    33,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    34,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    35,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    36,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    37,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    38,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    39,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCount<
    T,
    40,
    std::void_t<decltype([]() {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          T();
    })>>
    final : std::true_type {};

template<typename T>
struct DecomposeCountnImpl
    : std::integral_constant<
          std::size_t,
          CaseIndexImpl<
              DecomposeCount<T, 1>,
              DecomposeCount<T, 2>,
              DecomposeCount<T, 3>,
              DecomposeCount<T, 4>,
              DecomposeCount<T, 5>,
              DecomposeCount<T, 6>,
              DecomposeCount<T, 7>,
              DecomposeCount<T, 8>,
              DecomposeCount<T, 9>,
              DecomposeCount<T, 10>,
              DecomposeCount<T, 11>,
              DecomposeCount<T, 12>,
              DecomposeCount<T, 13>,
              DecomposeCount<T, 14>,
              DecomposeCount<T, 15>,
              DecomposeCount<T, 16>,
              DecomposeCount<T, 17>,
              DecomposeCount<T, 18>,
              DecomposeCount<T, 19>,
              DecomposeCount<T, 20>,
              DecomposeCount<T, 21>,
              DecomposeCount<T, 22>,
              DecomposeCount<T, 23>,
              DecomposeCount<T, 24>,
              DecomposeCount<T, 25>,
              DecomposeCount<T, 26>,
              DecomposeCount<T, 27>,
              DecomposeCount<T, 28>,
              DecomposeCount<T, 29>,
              DecomposeCount<T, 30>,
              DecomposeCount<T, 31>,
              DecomposeCount<T, 32>,
              DecomposeCount<T, 33>,
              DecomposeCount<T, 34>,
              DecomposeCount<T, 35>,
              DecomposeCount<T, 36>,
              DecomposeCount<T, 37>,
              DecomposeCount<T, 38>,
              DecomposeCount<T, 39>,
              DecomposeCount<T, 40>>::index> {};

template<typename T>
concept DecomposeConditionRaw = std::is_aggregate_v<T> && (std::is_empty_v<T> || DecomposeCountnImpl<T>::value != 0);

template<typename T>
concept DecomposeCondition = DecomposeConditionRaw<std::remove_cvref_t<T>>;

template<typename T>
struct DecomposeCountraw : std::conditional_t<DecomposeCondition<T>, DecomposeCountnImpl<T>, NotDecomposableImpl> {};

template<typename T>
struct DecomposeCountImpl : DecomposeCountraw<std::remove_cvref_t<T>> {};

#else  // defined(__clang__) && __clang_major__ < 17
// ----------------------------------------------------

// From
// https://github.com/helly25/mbo/commit/03789fed711e9603170dda767b1ebe50be6df282
//
// Unlike Clang 16, GCC does not understand `decltype` of a lmbda performing
// structured-bindings, e.g.: `decltype([]() { const auto& [a0] = T(); }`.
//
// Thus GCC needs to count initializers and fields and base classes.
// The core idea of identifying the actual number of values a struct
// decomposes into for a structured-binding is extremely well discussed in
// https://towardsdev.com/counting-the-number-of-fields-in-an-aggregate-in-c-20-part-2-d3103dec734f
//
// However, that solution still does not respect base classes, which is why
// I originally gave up on that approach independently and developed the
// Clang solution instead. Finding this documentation was an excellent
// refresher on all the basic field handling. With that, I went on to
// identifying base classes and all conditions that structured-bindings
// apply while upgrading the core field/initializer handling.
//
// Some experiments were made here: https://godbolt.org/z/se7PcjqGz

// First determine how many initializer values can be used to initialize `T`.

template<typename T, typename... Args>
concept IsAggregateInitializable = IsAggregate<T> && requires { T{std::declval<Args>()...}; };

template<IsAggregate T, typename Indices>
struct IsAggregateInitializableFromIndices;

template<IsAggregate T, std::size_t... Indices>
struct IsAggregateInitializableFromIndices<T, std::index_sequence<Indices...>>
    : std::bool_constant<IsAggregateInitializable<T, AnyTypeN<Indices>...>> {};

template<typename T, std::size_t N>
concept IsAggregateInitializableWithNumArgs =
    IsAggregate<T> && IsAggregateInitializableFromIndices<T, std::make_index_sequence<N>>::value;

// Now determine how many fields that translates to.
// For that we try to replace initializers with braced initializers.

template<IsAggregate T>
struct AggregateInitializeTest {
  template<std::size_t N>
  struct IsInitializable : std::bool_constant<IsAggregateInitializableWithNumArgs<T, N>> {};
};

// Upper bound of how many initializers there actually may be.
// On a single byte there could be 8 bit fields, so we need to multiply with 8.
constexpr std::size_t kFieldBlowupFactor = 8;

template<IsAggregate T>
struct AggregateInitializerCount
    : BinarySearch<AggregateInitializeTest<T>::template IsInitializable, 0, kFieldBlowupFactor * sizeof(T) + 1> {};

template<IsAggregate T, typename Indices, typename FieldIndices>
struct AggregateInitializeableWith;

template<IsAggregate T, std::size_t... kIndices, std::size_t... kFieldIndices>
    struct AggregateInitializeableWith<T, std::index_sequence<kIndices...>, std::index_sequence<kFieldIndices...>>
    : std::bool_constant < requires {
  T{std::declval<AnyTypeN<kIndices>>()..., {std::declval<AnyNonBaseTypeN<kFieldIndices, T>>()...}};
} > {};

template<typename T, std::size_t N, std::size_t M>
concept AggregateFieldAtInitializeableWithImpl =
    IsAggregate<T> && AggregateInitializeableWith<T, std::make_index_sequence<N>, std::make_index_sequence<M>>::value;

template<IsAggregate T, std::size_t N>
struct AggregateFieldInitTest {
  template<std::size_t M>
  struct IsInitializable : std::bool_constant<AggregateFieldAtInitializeableWithImpl<T, N, M>> {};
};

template<IsAggregate T, std::size_t N>
struct AggregateFieldInitializerCount
    : BinarySearch<AggregateFieldInitTest<T, N>::template IsInitializable, 0, kFieldBlowupFactor * sizeof(T) + 1> {};

template<std::size_t kFieldInitCount, std::size_t kInitializerCount>
struct DetectSpecial
    : std::integral_constant<std::size_t, (kFieldInitCount > kInitializerCount ? 1 : kFieldInitCount)> {};

template<
    IsAggregate T,
    std::size_t kInitializerCount,
    std::size_t kFieldIndex,
    std::size_t kFieldCount,
    bool = (kFieldIndex < kInitializerCount && kFieldCount < kInitializerCount)>
struct AggregateFieldCounterImpl
    : AggregateFieldCounterImpl<
          T,
          kInitializerCount,
          kFieldIndex + DetectSpecial<AggregateFieldInitializerCount<T, kFieldIndex>::value, kInitializerCount>::value,
          kFieldCount + 1> {};

template<IsAggregate T, std::size_t kInitializerCount, std::size_t kFieldIndex, std::size_t kFieldCount>
struct AggregateFieldCounterImpl<T, kInitializerCount, kFieldIndex, kFieldCount, false>
    : std::integral_constant<std::size_t, kFieldCount> {};

template<IsAggregate T, std::size_t kInitializerCount>
struct AggregateFieldCounter : AggregateFieldCounterImpl<T, kInitializerCount, 0, 0> {};

template<IsAggregate T>
struct AggregateFieldCounter<T, 0> : NotDecomposableImpl {};

template<IsAggregate T>
struct AggregateFieldCountRawImpl : AggregateFieldCounter<T, AggregateInitializerCount<T>::value> {};

// Only 1 base may have fields.
// If any base has fields, then T may not have fields itself. In other words
// there can only be one non empty base.

// a) Test whether `T` can be initialized from bases, all but one `field` empty.
//    If this works for one case, then there is exactly one non empty base.
template<IsAggregate T, std::size_t kNonEmptyIndex, typename BaseIndices>
struct OneNonEmptyBaseAtImpl;

template<IsAggregate T, std::size_t kNonEmptyIndex, std::size_t... kBaseIndex>
struct OneNonEmptyBaseAtImpl<T, kNonEmptyIndex, std::index_sequence<kBaseIndex...>>
    : IsBracesConstructibleImpl<T, AnyBaseMaybeEmptyN<kBaseIndex, T, kNonEmptyIndex != kBaseIndex>...> {};

template<IsAggregate T, std::size_t kFieldCount, std::size_t kNonEmptyIndex>
struct OneNonEmptyBaseAt : OneNonEmptyBaseAtImpl<T, kNonEmptyIndex, std::make_index_sequence<kFieldCount>> {};

template<IsAggregate T, std::size_t kFieldCount, typename Indices>
struct OneNonEmptyBaseIndicesImpl;

template<IsAggregate T, std::size_t kFieldCount, std::size_t... kIndex>
struct OneNonEmptyBaseIndicesImpl<T, kFieldCount, std::index_sequence<kIndex...>>
    : std::bool_constant<(... || OneNonEmptyBaseAt<T, kFieldCount, kIndex>::value)> {};

template<typename T, std::size_t kFieldCount>
concept OneNonEmptyBaseImpl = OneNonEmptyBaseIndicesImpl<T, kFieldCount, std::make_index_sequence<kFieldCount>>::value;

template<typename T>
concept OneNonEmptyBase = OneNonEmptyBaseImpl<T, AggregateFieldCountRawImpl<T>::value>;

// b) Test whether `T` can be initialized from at least one non empty base and
// at least one field.
//    In (a) only one non empty base was allowed, here we require at least one
//    non empty base.
template<IsAggregate T, std::size_t kNonEmptyIndex, typename BaseIndices, typename FieldIndices>
struct OneNonEmptyBasePlusFieldsAtImpl;

template<IsAggregate T, std::size_t kNonEmptyIndex, std::size_t... kBaseIndex, std::size_t... kFieldIndex>
struct OneNonEmptyBasePlusFieldsAtImpl<
    T,
    kNonEmptyIndex,
    std::index_sequence<kBaseIndex...>,
    std::index_sequence<kFieldIndex...>>
    : IsBracesConstructibleImpl<
          T,
          AnyBaseMaybeEmptyN<kBaseIndex, T, kNonEmptyIndex != kBaseIndex, true>...,
          AnyNonBaseTypeN<kFieldIndex, T>...> {};

template<IsAggregate T, std::size_t kFieldCount, std::size_t kBaseCount, std::size_t kNonEmptyIndex>
struct OneNonEmptyBasePlusFieldsAt
    : OneNonEmptyBasePlusFieldsAtImpl<
          T,
          kNonEmptyIndex,
          std::make_index_sequence<kBaseCount>,
          std::make_index_sequence<kFieldCount - kBaseCount>> {};

template<IsAggregate T, std::size_t kFieldCount, std::size_t kBaseCount, typename NonEmptyIndices>
struct OneNonEmptyBasePlusFieldsIndicesImpl;

template<IsAggregate T, std::size_t kFieldCount, std::size_t kBaseCount, std::size_t... kNonEmptyIndex>
struct OneNonEmptyBasePlusFieldsIndicesImpl<T, kFieldCount, kBaseCount, std::index_sequence<kNonEmptyIndex...>>
    : std::bool_constant<(... || OneNonEmptyBasePlusFieldsAt<T, kFieldCount, kBaseCount, kNonEmptyIndex>::value)> {};

template<typename T, std::size_t kFieldCount, std::size_t kBaseCount>
concept OneNonEmptyBasePlusFieldsIndices =
    OneNonEmptyBasePlusFieldsIndicesImpl<T, kFieldCount, kBaseCount, std::make_index_sequence<kBaseCount>>::value;

template<typename T, std::size_t kFieldCount, typename BaseCount>
struct OneNonEmptyBasePlusFieldsBaseCountImpl;

template<typename T, std::size_t kFieldCount, std::size_t... kBaseCount>
struct OneNonEmptyBasePlusFieldsBaseCountImpl<T, kFieldCount, std::index_sequence<kBaseCount...>>
    : std::bool_constant<(... || (OneNonEmptyBasePlusFieldsIndices<T, kFieldCount, kBaseCount>))> {};

template<typename T, std::size_t kFieldCount>
concept OneNonEmptyBasePlusFieldsBaseCount =
    kFieldCount > 0  // At least one field (non base)
    && OneNonEmptyBasePlusFieldsBaseCountImpl<T, kFieldCount, std::make_index_sequence<kFieldCount - 1>>::value;

template<typename T>
concept OneNonEmptyBasePlusFields = OneNonEmptyBasePlusFieldsBaseCount<T, AggregateFieldCountRawImpl<T>::value>;

// c) Empty bases do not participate in structured binding/decomposition. So
// count them.
//    We actually count bases twice: 1) Only empty bases, b) any bases.

template<IsAggregate T, bool kRequireEmpty, typename BaseIndices, typename FieldIndices>
struct CountBasesTestImpl;

template<IsAggregate T, bool kRequireEmpty, std::size_t... kBaseIndex, std::size_t... kFieldIndex>
struct CountBasesTestImpl<T, kRequireEmpty, std::index_sequence<kBaseIndex...>, std::index_sequence<kFieldIndex...>>
    : std::conditional_t<
          IsBracesConstructibleImpl<
              T,
              AnyBaseMaybeEmptyN<kBaseIndex, T, kRequireEmpty>...,
              AnyNonBaseTypeN<kFieldIndex, T>...>::value,
          std::integral_constant<std::size_t, sizeof...(kBaseIndex)>,
          std::integral_constant<std::size_t, 0>> {};

template<IsAggregate T, bool kRequireEmpty, std::size_t kNumBases, std::size_t kNumFields>
struct CountBasesTest
    : CountBasesTestImpl<T, kRequireEmpty, std::make_index_sequence<kNumBases>, std::make_index_sequence<kNumFields>> {
};

template<IsAggregate T, bool kRequireEmpty, std::size_t kNumFields, typename NumBases>
struct CountBasesIndicesImpl;

template<IsAggregate T, bool kRequireEmpty, std::size_t kNumFields, std::size_t... kNumBases>
struct CountBasesIndicesImpl<T, kRequireEmpty, kNumFields, std::index_sequence<kNumBases...>>
    : std::integral_constant<
          std::size_t,
          (... + CountBasesTest<T, kRequireEmpty, kNumBases, kNumFields - kNumBases>::value)> {};

template<IsAggregate T, bool kRequireEmpty, std::size_t kNumFields>
struct CountBasesImpl : CountBasesIndicesImpl<T, kRequireEmpty, kNumFields, std::make_index_sequence<kNumFields>> {};

template<IsAggregate T, bool kRequireEmpty>
struct CountBases : CountBasesImpl<T, kRequireEmpty, AggregateFieldCountRawImpl<T>::value> {};

// Put all into once struct.
template<typename T, bool = std::is_aggregate_v<std::remove_cvref_t<T>> && !std::is_empty_v<std::remove_cvref_t<T>>>
struct DecomposeInfo final {
  using Type = std::remove_cvref_t<T>;
  static constexpr bool kIsAggregate = std::is_aggregate_v<Type>;
  static constexpr bool kIsEmpty = std::is_empty_v<Type>;
  static constexpr std::size_t kInitializerCount = 0;
  static constexpr std::size_t kFieldCount = 0;
  static constexpr bool kBadFieldCount = 0;
  static constexpr bool kOneNonEmptyBase = 0;
  static constexpr bool kOneNonEmptyBasePlusFields = 0;
  static constexpr std::size_t kCountBases = 0;
  static constexpr std::size_t kCountEmptyBases = 0;
  static constexpr bool kOnlyEmptyBases = true;
  static constexpr bool kDecomposable = kIsAggregate || kIsEmpty;
  static constexpr std::size_t kDecomposeCount = kDecomposable ? 0 : NotDecomposableImpl::value;

  static std::string Debug() {
    std::string str;
    std::string_view sep;
# define DEBUG_ADD(f)                                                         \
   str += std::string(sep) + #f + ": " + std::to_string(DecomposeInfo<T>::f); \
   sep = ", "
    DEBUG_ADD(kIsAggregate);
    DEBUG_ADD(kIsEmpty);
    // DEBUG_ADD(kInitializerCount);
    // DEBUG_ADD(kFieldCount);
    // DEBUG_ADD(kOneNonEmptyBase);
    // DEBUG_ADD(kOneNonEmptyBasePlusFields);
    // DEBUG_ADD(kOnlyEmptyBases);
    DEBUG_ADD(kDecomposable);
    // DEBUG_ADD(kCountBases);
    // DEBUG_ADD(kCountEmptyBases);
    DEBUG_ADD(kDecomposeCount);
# undef DEBUG_ADD
    return str;
  }
};

template<typename T>
struct DecomposeInfo<T, true> final {
  using Type = std::remove_cvref_t<T>;
  static constexpr bool kIsAggregate = std::is_aggregate_v<Type>;
  static constexpr bool kIsEmpty = std::is_empty_v<Type>;
  static constexpr std::size_t kInitializerCount = AggregateInitializerCount<Type>::value;
  static constexpr std::size_t kFieldCount = AggregateFieldCountRawImpl<Type>::value;
  static constexpr bool kBadFieldCount =
      !kIsAggregate || kIsEmpty || kFieldCount == 0 || kFieldCount == NotDecomposableImpl::value;
  static constexpr bool kOneNonEmptyBase = OneNonEmptyBase<Type>;
  static constexpr bool kOneNonEmptyBasePlusFields = OneNonEmptyBasePlusFields<Type>;
  static constexpr std::size_t kCountBases = kBadFieldCount ? 0 : CountBases<Type, false>::value;
  static constexpr std::size_t kCountEmptyBases = kBadFieldCount ? 0 : CountBases<Type, true>::value;
  static constexpr bool kOnlyEmptyBases = kCountBases <= kCountEmptyBases;
  static constexpr bool kDecomposable =
      kIsAggregate && (kIsEmpty || ((kOneNonEmptyBase || kOnlyEmptyBases) && !kOneNonEmptyBasePlusFields));
  static constexpr std::size_t kDecomposeCount =
      kDecomposable ? kFieldCount - kCountEmptyBases : NotDecomposableImpl::value;

  static std::string Debug() {
    std::string str;
    std::string_view sep;
# define DEBUG_ADD(f)                                                         \
   str += std::string(sep) + #f + ": " + std::to_string(DecomposeInfo<T>::f); \
   sep = ", "
    DEBUG_ADD(kIsAggregate);
    DEBUG_ADD(kIsEmpty);
    DEBUG_ADD(kInitializerCount);
    DEBUG_ADD(kFieldCount);
    DEBUG_ADD(kOneNonEmptyBase);
    DEBUG_ADD(kOneNonEmptyBasePlusFields);
    DEBUG_ADD(kOnlyEmptyBases);
    DEBUG_ADD(kDecomposable);
    DEBUG_ADD(kCountBases);
    DEBUG_ADD(kCountEmptyBases);
    DEBUG_ADD(kDecomposeCount);
# undef DEBUG_ADD
    return str;
  }
};

// What we actually need

template<typename T>
concept DecomposeCondition = DecomposeInfo<T>::kDecomposable;

template<typename T>
struct DecomposeCountImpl : std::integral_constant<std::size_t, DecomposeInfo<T>::kDecomposeCount> {};

#endif  // defined(__clang__) && __clang_major__ < 17
// ----------------------------------------------------

// NOLINTBEGIN(readability-function-cognitive-complexity)

template<typename T>
struct DecomposeHelper final {
  DecomposeHelper() = delete;

  template<typename U>
  static constexpr auto ToTuple(U&& data) noexcept {
    constexpr auto kNumFields = DecomposeCountImpl<U>::value;
    if constexpr (kNumFields == 0) {
      return std::make_tuple();
    } else if constexpr (kNumFields == 1) {
      auto& [a1] = data;
      return std::make_tuple(a1);
    } else if constexpr (kNumFields == 2) {
      auto& [a1, a2] = data;
      return std::make_tuple(a1, a2);
    } else if constexpr (kNumFields == 3) {
      auto& [a1, a2, a3] = data;
      return std::make_tuple(a1, a2, a3);
    } else if constexpr (kNumFields == 4) {
      auto& [a1, a2, a3, a4] = data;
      return std::make_tuple(a1, a2, a3, a4);
    } else if constexpr (kNumFields == 5) {
      auto& [a1, a2, a3, a4, a5] = data;
      return std::make_tuple(a1, a2, a3, a4, a5);
    } else if constexpr (kNumFields == 6) {
      auto& [a1, a2, a3, a4, a5, a6] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6);
    } else if constexpr (kNumFields == 7) {
      auto& [a1, a2, a3, a4, a5, a6, a7] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7);
    } else if constexpr (kNumFields == 8) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8);
    } else if constexpr (kNumFields == 9) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    } else if constexpr (kNumFields == 10) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    } else if constexpr (kNumFields == 11) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    } else if constexpr (kNumFields == 12) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    } else if constexpr (kNumFields == 13) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    } else if constexpr (kNumFields == 14) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    } else if constexpr (kNumFields == 15) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    } else if constexpr (kNumFields == 16) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);
    } else if constexpr (kNumFields == 17) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
    } else if constexpr (kNumFields == 18) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18);
    } else if constexpr (kNumFields == 19) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19);
    } else if constexpr (kNumFields == 20) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    } else if constexpr (kNumFields == 21) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21);
    } else if constexpr (kNumFields == 22) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22);
    } else if constexpr (kNumFields == 23) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
    } else if constexpr (kNumFields == 24) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23,
          a24);
    } else if constexpr (kNumFields == 25) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25);
    } else if constexpr (kNumFields == 26) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26);
    } else if constexpr (kNumFields == 27) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27);
    } else if constexpr (kNumFields == 28) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28);
    } else if constexpr (kNumFields == 29) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29);
    } else if constexpr (kNumFields == 30) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30);
    } else if constexpr (kNumFields == 31) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31);
    } else if constexpr (kNumFields == 32) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32);
    } else if constexpr (kNumFields == 33) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33);
    } else if constexpr (kNumFields == 34) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34);
    } else if constexpr (kNumFields == 35) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35);
    } else if constexpr (kNumFields == 36) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
    } else if constexpr (kNumFields == 37) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37);
    } else if constexpr (kNumFields == 38) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38);
    } else if constexpr (kNumFields == 39) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39);
    } else if constexpr (kNumFields == 40) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          data;
      return std::make_tuple(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40);
    }
  }

  template<typename U>
  static constexpr auto ToTuple(U& data) noexcept {
    constexpr auto kNumFields = DecomposeCountImpl<U>::value;
    if constexpr (kNumFields == 0) {
      return std::make_tuple();
    } else if constexpr (kNumFields == 1) {
      auto& [a1] = data;
      return std::tie(a1);
    } else if constexpr (kNumFields == 2) {
      auto& [a1, a2] = data;
      return std::tie(a1, a2);
    } else if constexpr (kNumFields == 3) {
      auto& [a1, a2, a3] = data;
      return std::tie(a1, a2, a3);
    } else if constexpr (kNumFields == 4) {
      auto& [a1, a2, a3, a4] = data;
      return std::tie(a1, a2, a3, a4);
    } else if constexpr (kNumFields == 5) {
      auto& [a1, a2, a3, a4, a5] = data;
      return std::tie(a1, a2, a3, a4, a5);
    } else if constexpr (kNumFields == 6) {
      auto& [a1, a2, a3, a4, a5, a6] = data;
      return std::tie(a1, a2, a3, a4, a5, a6);
    } else if constexpr (kNumFields == 7) {
      auto& [a1, a2, a3, a4, a5, a6, a7] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7);
    } else if constexpr (kNumFields == 8) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8);
    } else if constexpr (kNumFields == 9) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    } else if constexpr (kNumFields == 10) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    } else if constexpr (kNumFields == 11) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    } else if constexpr (kNumFields == 12) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    } else if constexpr (kNumFields == 13) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    } else if constexpr (kNumFields == 14) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    } else if constexpr (kNumFields == 15) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    } else if constexpr (kNumFields == 16) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);
    } else if constexpr (kNumFields == 17) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
    } else if constexpr (kNumFields == 18) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18);
    } else if constexpr (kNumFields == 19) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19);
    } else if constexpr (kNumFields == 20) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    } else if constexpr (kNumFields == 21) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21);
    } else if constexpr (kNumFields == 22) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22);
    } else if constexpr (kNumFields == 23) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
    } else if constexpr (kNumFields == 24) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23,
          a24);
    } else if constexpr (kNumFields == 25) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25);
    } else if constexpr (kNumFields == 26) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26);
    } else if constexpr (kNumFields == 27) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27);
    } else if constexpr (kNumFields == 28) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28);
    } else if constexpr (kNumFields == 29) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29);
    } else if constexpr (kNumFields == 30) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30);
    } else if constexpr (kNumFields == 31) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31);
    } else if constexpr (kNumFields == 32) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32);
    } else if constexpr (kNumFields == 33) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33);
    } else if constexpr (kNumFields == 34) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34);
    } else if constexpr (kNumFields == 35) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35);
    } else if constexpr (kNumFields == 36) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
    } else if constexpr (kNumFields == 37) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37);
    } else if constexpr (kNumFields == 38) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38);
    } else if constexpr (kNumFields == 39) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39);
    } else if constexpr (kNumFields == 40) {
      auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40);
    }
  }

  template<typename U>
  static constexpr auto ToTuple(const U& data) noexcept {
    constexpr auto kNumFields = DecomposeCountImpl<U>::value;
    if constexpr (kNumFields == 0) {
      return std::make_tuple();
    } else if constexpr (kNumFields == 1) {
      const auto& [a1] = data;
      return std::tie(a1);
    } else if constexpr (kNumFields == 2) {
      const auto& [a1, a2] = data;
      return std::tie(a1, a2);
    } else if constexpr (kNumFields == 3) {
      const auto& [a1, a2, a3] = data;
      return std::tie(a1, a2, a3);
    } else if constexpr (kNumFields == 4) {
      const auto& [a1, a2, a3, a4] = data;
      return std::tie(a1, a2, a3, a4);
    } else if constexpr (kNumFields == 5) {
      const auto& [a1, a2, a3, a4, a5] = data;
      return std::tie(a1, a2, a3, a4, a5);
    } else if constexpr (kNumFields == 6) {
      const auto& [a1, a2, a3, a4, a5, a6] = data;
      return std::tie(a1, a2, a3, a4, a5, a6);
    } else if constexpr (kNumFields == 7) {
      const auto& [a1, a2, a3, a4, a5, a6, a7] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7);
    } else if constexpr (kNumFields == 8) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8);
    } else if constexpr (kNumFields == 9) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    } else if constexpr (kNumFields == 10) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    } else if constexpr (kNumFields == 11) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    } else if constexpr (kNumFields == 12) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    } else if constexpr (kNumFields == 13) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    } else if constexpr (kNumFields == 14) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    } else if constexpr (kNumFields == 15) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    } else if constexpr (kNumFields == 16) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);
    } else if constexpr (kNumFields == 17) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
    } else if constexpr (kNumFields == 18) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18);
    } else if constexpr (kNumFields == 19) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19);
    } else if constexpr (kNumFields == 20) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    } else if constexpr (kNumFields == 21) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] =
          data;
      return std::tie(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21);
    } else if constexpr (kNumFields == 22) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22);
    } else if constexpr (kNumFields == 23) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
    } else if constexpr (kNumFields == 24) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23,
          a24);
    } else if constexpr (kNumFields == 25) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25);
    } else if constexpr (kNumFields == 26) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26);
    } else if constexpr (kNumFields == 27) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27);
    } else if constexpr (kNumFields == 28) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28);
    } else if constexpr (kNumFields == 29) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29);
    } else if constexpr (kNumFields == 30) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30);
    } else if constexpr (kNumFields == 31) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31);
    } else if constexpr (kNumFields == 32) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32);
    } else if constexpr (kNumFields == 33) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33);
    } else if constexpr (kNumFields == 34) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34);
    } else if constexpr (kNumFields == 35) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35);
    } else if constexpr (kNumFields == 36) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
    } else if constexpr (kNumFields == 37) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37);
    } else if constexpr (kNumFields == 38) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38);
    } else if constexpr (kNumFields == 39) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39);
    } else if constexpr (kNumFields == 40) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          data;
      return std::tie(
          a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
          a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40);
    }
  }
};

// NOLINTEND(readability-function-cognitive-complexity)
// NOLINTEND(*-magic-numbers)

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_DECOMPOSE_COUNT_H_
