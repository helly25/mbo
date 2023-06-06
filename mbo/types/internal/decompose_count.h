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

#ifndef MBO_TYPES_INTERNAL_DECOMPOSECOUNTH_
#define MBO_TYPES_INTERNAL_DECOMPOSECOUNTH_

// IWYU pragma private, include "mbo/types/traits.h"

#include <limits>
#include <tuple>
#include <type_traits>

#include "mbo/types/internal/cases.h"
#include "mbo/types/internal/is_braces_constructible.h"

namespace mbo::types::types_internal {
inline constexpr size_t kMaxSupportedFieldCount = 50;

template<typename T, size_t MaxArgs, typename... Args>
constexpr size_t StructCtorArgCountMaxFunc() {
  if constexpr (sizeof...(Args) > MaxArgs) {  // NOLINT(bugprone-branch-clone)
    return 0;
  } else if constexpr (constexpr size_t kCount = StructCtorArgCountMaxFunc<
                           T, MaxArgs, Args..., AnyType>();
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
          size_t,
          std::is_aggregate_v<T>
              ? StructCtorArgCountMaxFunc<T, kMaxSupportedFieldCount>()
              : 0> {};

template<typename T>
struct StructCtorArgCountMaxT
    : StructCtorArgCountMaxImpl<std::remove_cvref_t<T>> {};

template<typename T>
inline constexpr size_t StructCtorArgCountMaxV =
    StructCtorArgCountMaxT<T>::value;

template<typename T>
struct AggregateHasNonEmptyBaseRaw
    : std::bool_constant<
          !std::is_empty_v<T> && std::is_aggregate_v<T>
              ? StructCtorArgCountMaxFunc<
                    T,
                    kMaxSupportedFieldCount,
                    AnyTypeIf<T, true, true>>()
                    != 0
              : false> {};

template<typename T>
struct AggregateHasNonEmptyBaseImpl
    : AggregateHasNonEmptyBaseRaw<std::remove_cvref_t<T>> {};

// Crude aproximation of whether a `T` has a non empty base struct/class.
//
// NOTE: The behavior is undefined when using this on non aggregate types.
template<typename T>
concept AggregateHasNonEmptyBase = AggregateHasNonEmptyBaseImpl<T>::value;

template<typename T, bool = std::is_aggregate_v<T>&& std::is_empty_v<T>>
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
    std::void_t<decltype([]() {
      const auto& [a0, a1, a2, a3, a4, a5, a6] = T();
    })>>
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
    std::void_t<decltype([]() {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = T();
    })>>
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
    std::void_t<decltype([]() {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = T();
    })>>
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
struct DecomposeCount10<
    T,
    std::void_t<decltype([]() {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = T();
    })>>
    final : std::true_type {
  struct type {
    template<typename U>
    static constexpr auto ToTuple(U&& v) {
      const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = v;
      return std::make_tuple(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
  };
};

template<typename T>
struct DecomposeCountnImpl
    : std::integral_constant<
          size_t,
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
              DecomposeCount10<T>>::index> {};

template<typename T>
concept DecomposeConditionRaw =
    std::is_aggregate_v<T>
    && (std::is_empty_v<T> || DecomposeCountnImpl<T>::value != 0);

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
          DecomposeCount10<T>>::type {};

template<typename T>
struct DecomposeHelperRaw<T, false> {};

template<typename T>
requires DecomposeCondition<T>
struct DecomposeHelper : DecomposeHelperRaw<std::remove_cvref_t<T>>::type {};

struct NotDecomposableImpl
    : std::integral_constant<size_t, std::numeric_limits<size_t>::max()> {};

template<typename T>
struct DecomposeCountraw
    : std::conditional_t<
          DecomposeCondition<T>,
          DecomposeCountnImpl<T>,
          NotDecomposableImpl> {};

template<typename T>
struct DecomposeCountImpl : DecomposeCountraw<std::remove_cvref_t<T>> {};

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_DECOMPOSECOUNTH_
