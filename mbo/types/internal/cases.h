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

#ifndef MBO_TYPES_INTERNAL_CASES_H_
#define MBO_TYPES_INTERNAL_CASES_H_

// IWYU pragma private, include "mbo/types/cases.h"

#include <cstddef>  // IWYU pragma: keep
#include <type_traits>
#include <utility>

namespace mbo::types::types_internal {

// Requirement for the `if_then` cases of the `cases` template.
template<typename T>
concept IsIfThen = requires(T) {
  std::is_convertible_v<decltype(T::value), bool>;
  typename T::type;
};

// Concrete implmentation for the front case being `false`.
// This will simply strip the first two types and replac with the next type
// expanded and then increase the `Index`.
// So this will effectively never terminate but rather fail the compiler.
// But users will ensure there is always a `true` case at the end.
template<bool IfThen0Value, std::size_t Index, typename IfThen0Type, typename IfThen1, typename... OtherCases>
struct CasesForwardImpl : CasesForwardImpl<IfThen1::value, Index + 1, typename IfThen1::type, OtherCases...> {};

// Concrete implmentation for the front case being `true`.
// Will set the struct with `type` and `index` and then terminate unpacking.
template<std::size_t Index, typename IfThen0Type, typename IfThen1, typename... OtherCases>
struct CasesForwardImpl<true, Index, IfThen0Type, IfThen1, OtherCases...> {
  using type = IfThen0Type;
  static constexpr std::size_t index = Index;
};

// We forward as a `struct` because we have to change where the parameter-pack
// expansion occurs. Otherwise we could go with `using`.
template<typename IfThen0, typename... OtherCases>
struct CasesForward : CasesForwardImpl<IfThen0::value, 0, typename IfThen0::type, OtherCases...> {};

// Helper type to generate if-then case types.
template<bool If, typename Then>
struct IfThen final {
  static constexpr bool value = If;
  using type = Then;
};

// Helper type to generate else cases which are always true and must go last.
template<typename Else>
struct IfElse final {
  static constexpr bool value = true;
  using type = Else;
};

// Helper type to inject default cases and to ensure the required type expansion
// is always possible.
struct IfTrueThenVoid final {
  static constexpr bool value = true;
  using type = void;
};

// Helper typ that can be used to skip a case.
struct IfFalseThenVoid {
  static constexpr bool value = false;
  using type = void;
};

template<typename... IfThenCases>
requires(IsIfThen<IfThenCases> && ...)
struct CasesImpl
    : types_internal::CasesForward<
          IfThenCases...,
          // Provide `if_then_void` twice, so two cases can always be expanded.
          IfTrueThenVoid,
          IfTrueThenVoid> {};

template<typename... IfThenCases>
struct CaseIndexImpl {
 private:
  static constexpr std::size_t kRawIndex = CasesImpl<IfThenCases...>::index;

 public:
  static constexpr std::size_t index = kRawIndex >= sizeof...(IfThenCases) ? 0 : kRawIndex + 1;
};

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_CASES_H_
