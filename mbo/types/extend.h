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

#ifndef MBO_TYPES_EXTEND_H_
#define MBO_TYPES_EXTEND_H_

#include "mbo/types/extender.h"         // IWYU pragma: export
#include "mbo/types/internal/extend.h"  // IWYU pragma: export

namespace mbo::types {

// The meta type `Extend` provides a convenience injection mechanism for structs
// and classes, so that the actual definitions can stay simple while providing
// general functionality out of the box in a consistant manner.
//
// For exmaple:
//
// ```c++
// template<typename T, typename... Extender>
// using mbo::types::Extend;
//
// struct Name : Extend<Name> {
//    std::string first;
//    std::string last;
// };
//
// std::cout << Name{.first = "First", .last = "Last"} << std::endl;
// ```
//
// The struct `Name` automatically gains the ability to print, stream and
// compare itself. In the above example `{"First", "Last"}` will be printed.
// If compiled on Clang it will print `{first: "First", last: "Last"}` (see
// AbslStringify for restrictions). Also, the field names can be suppressed
// by adding `using MboTypesExtendDoNotPrintFieldNames = void;` to the type.
//
// NOTE: No member may be an anonymous union or struct.
template<typename T, typename... Extender>
struct Extend : ::mbo::extender::Extend<T, ::mbo::types::extender::Default, Extender...> {};

// Same as `Extend` but without default extenders. This alows to control the
// exact extender set to be used.
//
// Example:
// ```c++
// struct Name : mbo::types::ExtendNoDEfault<mbo::types::Comparable> {
//    std::string first;
//    std::string last;
// };
// ```
//
// Here `Name` gets injected with all comparison operators but it will not get
// print, stream or hash functionality.
//
// NOTE: No member may be an anonymous union or struct.
template<typename T, typename... Extender>
struct ExtendNoDefault : ::mbo::extender::Extend<T, Extender...> {};

}  // namespace mbo::types

namespace std {

// Support for std::hash in addition to abseil hash support.
// This requires that Extended classes/structs Extend with `AbslHashable`.
//
// Example:
// ```c++
// struct Name : mbo::types::Extend<Name> {
//   std::string first;
//   std::string last;
// };
//
// void demo() {
//   std::size_t hash = std::hash<Name>{}(
//       Name{.first = "first", .last = "last"});
// }
// ```
template<typename Extended>
requires mbo::types::types_internal::HasExtender<Extended, mbo::types::extender::AbslHashable>
struct hash<Extended> {  // NOLINT(cert-dcl58-cpp)

  std::size_t operator()(const Extended& obj) const noexcept { return absl::HashOf(obj); }
};

}  // namespace std

#endif  // MBO_TYPES_EXTEND_H_
