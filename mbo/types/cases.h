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

#ifndef MBO_TYPES_CASES_H_
#define MBO_TYPES_CASES_H_

#include <cstddef>

#include "mbo/types/internal/cases.h"  // IWYU pragma: export

namespace mbo::types {

// Helper type to generate if-then cases types.
template<bool If, typename Then>
using IfThen = types_internal::IfThen<If, Then>;

// Helper type to generate else cases which are always true and must go last.
template<typename Else>
using IfElse = types_internal::IfElse<Else>;

// Helper type to inject default cases and to ensure the required type expansion
// is always possible.
using types_internal::IfTrueThenVoid;

// Helper type that can be used to skip a case.
using types_internal::IfFalseThenVoid;

// The meta type `cases` allows to switch types based on conditions:
//
// For example:
//
// ```c++
// using Type = Cases<IfThen<if_0, type_0>, IfThen<if_1, type_1>>
// ```
//
// will evaluate to:
// . `type_0` if `if_0` evaluates to true, or
// . `type_1` if `if_1` evaluates to true, or
// . `void` if none of the `IfThen` case has an `If` that evaluates to true.
//
// The expression `Cases<>` will evaluate to void.
//
// The meta type `IfThen` is a convenience to implement `Cases`. The actual
// requirement is defined in the concept `types_internal::IsIfThen` which checks
// that the provided type has a member `value` that can be converted to a `bool`
// and a member `type` that will be used as the 'type' if the case is selected.
//
// Similarly `IfElse` can be used to create else/default cases that are always
// true. E.g.:
//
//   `Cases<IfThen<cond1, type1>, IfThen<cond2, type2>, IfElse<default_type>>`
//
template<typename... IfThenCases>
using Cases = typename types_internal::CasesImpl<IfThenCases...>::type;

// While `Cases` requires special condition types of the form (condition, type)
// like `IfThen`, the `CaseIndex` only requires conditions and evaluates to
// the 1-based index of the first condition that evaluates to true.
//
// Examples:
//   CaseIndex<> == 0
//   CaseIndex<0> == 0
//   CaseIndex<1> == 1
//   CaseIndex<0, 0> == 0
//   CaseIndex<0, 1> == 2
//   CaseIndex<1, 0> == 1
//   CaseIndex<1, 1> == 1
template<bool... Conditions>
inline constexpr size_t CaseIndex =  // NOLINT(readability-identifier-naming)
    types_internal::CaseIndexImpl<IfThen<Conditions, void>...>::index;

}  // namespace mbo::types

#endif  // MBO_TYPES_CASES_H_
