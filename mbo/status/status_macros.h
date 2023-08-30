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

#include <utility>

#include "absl/status/status.h"  // IWYU pragma: keep
#include "absl/status/statusor.h" // IWYU pragma: keep

// Macro that allows to return from a non-OkStatus in a single line.
// This is pure syntactical sugar for readability:
//
// Instead of:
//
// ```c++
// auto result = FooBar();
// if (!result.ok()) {
//   return result;
// }
// ```
//
// The same can be written as:
//
// ```c++
// MBO_STATUS_RETURN_IF_ERROR(FooBar());
// ```
//
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MBO_STATUS_RETURN_IF_ERROR(expr) \
  if (!(expr).ok()) return absl::Status(expr)

// PRIVATE MACRO - DO NOT USE.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define _MBO_VAR_CAT_(var, line) var ## line

// Similar to MBO_STATUS_RETURN_IF_ERROR but this assigns the result os an `absl::StatusOr<T>`:
//
// Instead of:
//
// ```c++
// absl::StatusOr<T> var_or = FooBar();
// if (!var_or.ok()) {
//   return var_or.status();
// }
// T var = *std::move(var_or);
// ```
//
// This can be simplified to:
//
// ```c++
// MBO_STATUS_ASSIGN_OR_RETURN(T var, FooBar());
// ```
//
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MBO_STATUS_ASSIGN_OR_RETURN(res, expr) \
  auto _MBO_VAR_CAT_(var, __LINE__) = (expr); \
  if (_MBO_VAR_CAT_(var, __LINE__).ok()) { \
    (res) = *std::move(_MBO_VAR_CAT_(var, __LINE__));\
  } else return _MBO_VAR_CAT_(var, __LINE__).status()
