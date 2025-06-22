// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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

#ifndef MBO_TYPES_INTERNAL_TRAITS_H_
#define MBO_TYPES_INTERNAL_TRAITS_H_

// IWYU pragma: private, include "mbo/types/traits.h"
// IWYU pragma: friend "mbo/types/internal/.*"

#include <concepts>  // IWYU pragma: keep

namespace mbo::types::types_internal {

// NOLINTBEGIN(*-magic-numbers)

template<typename T>
concept IsAggregate = std::is_aggregate_v<T>;

template<typename T>
concept IsEmptyType = std::is_empty_v<T>;

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_TRAITS_H_
