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

// This header implements `Stringify`.
//
#ifndef MBO_TYPES_STRINGIFY_OSTREAM_H_
#define MBO_TYPES_STRINGIFY_OSTREAM_H_

// IWYU pragma: private, include "mbo/types/extend.h"

#include <concepts>  // IWYU pragma: keep
#include <iostream>
#include <memory>
#include <ostream>

#include "mbo/types/stringify.h"

namespace mbo::types {
namespace types_internal {

std::shared_ptr<const Stringify> GetStringifyForOstream();

}  // namespace types_internal

// Set the global Stringify stream options by mode.
//
// While this is thread-safe, there is no guarantee that the same options will be used.
// That is becasue the options could be changed before the intended stream call.
void SetStringifyOstreamOutputMode(Stringify::OutputMode output_mode);

// Set the global Stringify stream options by `StringifyOptions`.
//
// While this is thread-safe, there is no guarantee that the same options will be used.
// That is becasue the options could be changed before the intended stream call.
void SetStringifyOstreamOptions(const StringifyOptions& options);
void SetStringifyOstreamOptions(const StringifyOptions&& options) = delete;

}  // namespace mbo::types

// Add `Stringify` ostream support to *ALL* types that claim support, see `HasMboTypesStringifySupport`.
std::ostream& operator<<(std::ostream& os, const mbo::types::HasMboTypesStringifySupport auto& v) {
  mbo::types::types_internal::GetStringifyForOstream()->Stream(os, v);
  return os;
}

#endif  // MBO_TYPES_STRINGIFY_OSTREAM_H_
