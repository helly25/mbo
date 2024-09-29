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

#ifndef MBO_TYPES_TUPLE_H_
#define MBO_TYPES_TUPLE_H_

#include <tuple>

namespace mbo::types {

namespace types_internal {

template<typename T>
struct IsTuple : std::false_type {};

template<typename... Ts>
struct IsTuple<std::tuple<Ts...>> : std::true_type {};

}  // namespace types_internal

template<typename T>
concept IsTuple = types_internal::IsTuple<T>::value;

namespace types_internal {

template<typename... T>
struct TupleCat;

template<typename... T, typename... U>
struct TupleCat<std::tuple<T...>, std::tuple<U...>> {
  using type = std::tuple<T..., U...>;
};

}  // namespace types_internal

// Concatenate tuples (both `T` and `U` must be tuples).
template<IsTuple T, IsTuple U>
using TupleCat = types_internal::TupleCat<T, U>::type;

// Append type `U` to tuple `T` (type `U` can be a tuple).
template<IsTuple T, typename U>
using TupleAdd = TupleCat<T, std::tuple<U>>;

}  // namespace mbo::types

#endif  // MBO_TYPES_TUPLE_H_
