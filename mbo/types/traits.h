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

#ifndef MBO_TYPES_TRAITS_H_
#define MBO_TYPES_TRAITS_H_

#include <cstddef>

#include "mbo/types/internal/decompose_count.h"          // IWYU pragma: export
#include "mbo/types/internal/is_braces_constructible.h"  // IWYU pragma: export

namespace mbo::types {

// If `T` is a struct and the below conditions are met, return the number of
// variable fields it decomposes into.
//
// Requirements:
// * T is an aggregate.
// * T has no or only empty base classes.
template<typename T>
inline constexpr size_t DecomposeCountV = types_internal::DecomposeCountImpl<T>::value;

// If a `T` cannot be decomposed, then `decompose_count` returns this value.
inline constexpr size_t NotDecomposableV = types_internal::NotDecomposableImpl::value;

// Returns whether `T` can be constructed from `Args...`.
template<typename T, typename... Args>
inline constexpr bool IsBracesConstructibleV = types_internal::IsBracesConstructibleImplT<T, Args...>::value;

}  // namespace mbo::types

#endif  // MBO_TYPES_TRAITS_H_
