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

#ifndef MBO_TYPES_INTERNAL_DECOMPOSECOUNT_H_
#define MBO_TYPES_INTERNAL_DECOMPOSECOUNT_H_

// IWYU pragma private, include "mbo/types/traits.h"

#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "mbo/types/internal/binary_search.h"
#include "mbo/types/internal/cases.h"  // IWYU pragma: keep
#include "mbo/types/internal/is_braces_constructible.h"

namespace mbo::types::types_internal {

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

#if defined(__clang__)

template<typename T, bool = IsAggregate<T>&& std::is_empty_v<T>>
struct DecomposeCount0 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount0<T, true> final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& /* v */) {
      return std::tuple<>{};
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount1 : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount1<  //
    T,
    std::void_t<decltype([]() { const auto& [a0] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0] = v;
      return std::make_tuple(a0);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount2 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount2<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1] = v;
      return std::make_tuple(a0, a1);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount3 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount3<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2] = v;
      return std::make_tuple(a0, a1, a2);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount4 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount4<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3] = v;
      return std::make_tuple(a0, a1, a2, a3);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount5 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount5<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4] = v;
      return std::make_tuple(a0, a1, a2, a3, a4);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount6 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount6<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4, a5] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount7 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount7<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4, a5, a6] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount8 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount8<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount9 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount9<  //
    T,
    std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount10 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount10<T, std::void_t<decltype([]() { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = T(); })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount11 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount11<T, std::void_t<decltype([]() {
                          const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = T();
                        })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount12 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount12<T, std::void_t<decltype([]() {
                          const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = T();
                        })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount13 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount13<T, std::void_t<decltype([]() {
                          const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = T();
                        })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount14 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount14<T, std::void_t<decltype([]() {
                          const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = T();
                        })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    }
  };
};

template<typename T, typename = void>
struct DecomposeCount15 final : IfFalseThenVoid {};

template<typename T>
struct DecomposeCount15<T, std::void_t<decltype([]() {
                          const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = T();
                        })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    }
  };
};

template<typename T>
struct DecomposeCountnImpl
    : std::integral_constant<
          std::size_t,
          CaseIndexImpl<
              DecomposeCount1<T>,
              DecomposeCount2<T>,
              DecomposeCount3<T>,
              DecomposeCount4<T>,
              DecomposeCount5<T>,
              DecomposeCount6<T>,
              DecomposeCount7<T>,
              DecomposeCount8<T>,
              DecomposeCount9<T>,
              DecomposeCount10<T>,
              DecomposeCount11<T>,
              DecomposeCount12<T>,
              DecomposeCount13<T>,
              DecomposeCount14<T>,
              DecomposeCount15<T>>::index> {};

template<typename T>
concept DecomposeConditionRaw = std::is_aggregate_v<T> && (std::is_empty_v<T> || DecomposeCountnImpl<T>::value != 0);

template<typename T>
concept DecomposeCondition = DecomposeConditionRaw<std::remove_cvref_t<T>>;

template<typename T, bool = DecomposeCondition<T>>
struct DecomposeHelperRaw
    : CasesImpl<
          DecomposeCount0<T>,
          DecomposeCount1<T>,
          DecomposeCount2<T>,
          DecomposeCount3<T>,
          DecomposeCount4<T>,
          DecomposeCount5<T>,
          DecomposeCount6<T>,
          DecomposeCount7<T>,
          DecomposeCount8<T>,
          DecomposeCount9<T>,
          DecomposeCount10<T>,
          DecomposeCount11<T>,
          DecomposeCount12<T>,
          DecomposeCount13<T>,
          DecomposeCount14<T>,
          DecomposeCount15<T>>::type {};

template<typename T>
struct DecomposeHelperRaw<T, false> {};

template<typename T>
requires DecomposeCondition<T>
struct DecomposeHelper : DecomposeHelperRaw<std::remove_cvref_t<T>>::type {};

template<typename T>
struct DecomposeCountraw : std::conditional_t<DecomposeCondition<T>, DecomposeCountnImpl<T>, NotDecomposableImpl> {};

template<typename T>
struct DecomposeCountImpl : DecomposeCountraw<std::remove_cvref_t<T>> {};

#else  // __clang__

// From https://github.com/helly25/mbo/commit/03789fed711e9603170dda767b1ebe50be6df282
//
// Unlike Clang, GCC does not understand `decltype` of a lmbda performing
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
// If any base has fields, then T may not have fields itself. In other words there can only be one non empty base.

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

// b) Test whether `T` can be initialized from at least one non empty base and at least one field.
//    In (a) only one non empty base was allowed, here we require at least one non empty base.
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

// c) Empty bases do not participate in structured binding/decomposition. So count them.
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
#define DEBUG_ADD(f)                                                         \
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
#undef DEBUG_ADD
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
#define DEBUG_ADD(f)                                                         \
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
#undef DEBUG_ADD
    return str;
  }
};

// What we actually need

template<typename T>
concept DecomposeCondition = DecomposeInfo<T>::kDecomposable;

template<typename T>
struct DecomposeCountImpl : std::integral_constant<std::size_t, DecomposeInfo<T>::kDecomposeCount> {};

template<typename T>
struct DecomposeHelper final {
  DecomposeHelper() = delete;

  template<typename U>
  static constexpr auto ToTuple(U&& data) noexcept {
    constexpr auto kNumFields = DecomposeCountImpl<U>::value;
    if constexpr (kNumFields == 0) {
      return std::make_tuple();
    } else if constexpr (kNumFields == 1) {
      const auto& [a1] = data;
      return std::make_tuple(a1);
    } else if constexpr (kNumFields == 2) {
      const auto& [a1, a2] = data;
      return std::make_tuple(a1, a2);
    } else if constexpr (kNumFields == 3) {
      const auto& [a1, a2, a3] = data;
      return std::make_tuple(a1, a2, a3);
    } else if constexpr (kNumFields == 4) {
      const auto& [a1, a2, a3, a4] = data;
      return std::make_tuple(a1, a2, a3, a4);
    } else if constexpr (kNumFields == 5) {
      const auto& [a1, a2, a3, a4, a5] = data;
      return std::make_tuple(a1, a2, a3, a4, a5);
    } else if constexpr (kNumFields == 6) {
      const auto& [a1, a2, a3, a4, a5, a6] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6);
    } else if constexpr (kNumFields == 7) {
      const auto& [a1, a2, a3, a4, a5, a6, a7] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7);
    } else if constexpr (kNumFields == 8) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8);
    } else if constexpr (kNumFields == 9) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    } else if constexpr (kNumFields == 10) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    } else if constexpr (kNumFields == 11) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    } else if constexpr (kNumFields == 12) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    } else if constexpr (kNumFields == 13) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    } else if constexpr (kNumFields == 14) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    } else if constexpr (kNumFields == 15) {
      const auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = data;
      return std::make_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    }
  }
};

#endif  // __clang__ / not __clang__

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_DECOMPOSECOUNT_H_
