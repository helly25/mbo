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

#ifndef MBO_TYPES_INTERNAL_EXTENDER_H_
#define MBO_TYPES_INTERNAL_EXTENDER_H_

// IWYU pragma private, include "mbo/types/extend.h"

namespace mbo::types::types_internal {

// Determine whether type `Extended` is an `Extend`ed type.
template<typename Extended>
concept IsExtended = requires {
  typename Extended::Type;
  typename Extended::RegisteredExtenders;
  { Extended::RegisteredExtenderNames() };
};

// Access to the extender implementation for constructing the CRTP chain using
// `ExtendBuildChain`.
// This is done via a distinct struct, so that the non specialized templates
// can be private.
template<typename ExtenderBase, typename Extender>
struct UseExtender {
  using type = typename Extender::template Impl<ExtenderBase>;
};

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_EXTENDER_H_
