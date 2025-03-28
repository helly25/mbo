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

#ifndef MBO_STATUS_STATUS_H_
#define MBO_STATUS_STATUS_H_

#include <utility>

#include "absl/status/status.h"    // IWYU pragma: keep
#include "absl/status/statusor.h"  // IWYU pragma: keep

namespace mbo::status {

// Conversion of any `absl::Status` or `absl::StatusOr<T>` to an `absl::Status`.
inline absl::Status GetStatus(absl::Status v) {
  return v;
}

template<typename T>
inline absl::Status GetStatus(const absl::StatusOr<T>& v) {
  return v.status();
}

template<typename T>
inline absl::Status GetStatus(absl::StatusOr<T>&& v) {
  return std::move(v).status();
}

template<typename T>
requires(std::constructible_from<::absl::Status, T> || std::convertible_to<T, absl::Status>)
inline ::absl::Status GetStatus(T&& status) {
  return absl::Status(std::forward<decltype(status)>(status));
}

}  // namespace mbo::status

#endif  // MBO_STATUS_STATUS_H_
