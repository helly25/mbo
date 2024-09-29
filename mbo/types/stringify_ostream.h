
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

// This header implements `Stringify`.
//
#ifndef MBO_TYPES_STRINGIFY_OSTREAM_H_
#define MBO_TYPES_STRINGIFY_OSTREAM_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <concepts>  // IWYU pragma: keep
#include <iostream>

#include "mbo/types/stringify.h"

// Add `Stringify` ostream support to *ALL* types that claim support, see `HasMboTypesStringifySupport`.
std::ostream& operator<<(std::ostream& os, const mbo::types::HasMboTypesStringifySupport auto& v) {
  static mbo::types::Stringify sfy;
  sfy.Stream(os, v);
  return os;
}

#endif  // MBO_TYPES_STRINGIFY_OSTREAM_H_
