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

#ifndef MBO_STATUS_STATUS_BUILDER_H_
#define MBO_STATUS_STATUS_BUILDER_H_

#include <concepts>  // IWYU pragma: keep
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

#include "absl/status/status.h"    // IWYU pragma: keep
#include "absl/status/statusor.h"  // IWYU pragma: keep
#include "absl/strings/cord.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep

namespace mbo::status {

// Helper to construct modified `absl::Status`.
// Mainly this allows to stream text onto the message of a satus.
//
// Example:
// ```
// absl::Status extended =
//     StatusBuilder(status).Prepend() << "Prefix:" << StatusBuilder::Append << "Suffix";
// ```
class StatusBuilder final {
 public:
  ~StatusBuilder() noexcept = default;
  StatusBuilder() noexcept = default;

  explicit StatusBuilder(absl::Status status)
      : data_(status.ok() ? nullptr : std::unique_ptr<Data>(new Data{.status = std::move(status)})) {}

  template<typename T>
  explicit StatusBuilder(const absl::StatusOr<T>& status) : StatusBuilder(status.status()) {}

  template<typename T>
  explicit StatusBuilder(absl::StatusOr<T>&& status) : StatusBuilder(std::move(status).status()) {}

  StatusBuilder(const StatusBuilder&) = delete;
  StatusBuilder& operator=(const StatusBuilder&) = delete;
  StatusBuilder(StatusBuilder&&) noexcept = default;
  StatusBuilder& operator=(StatusBuilder&&) noexcept = default;

  bool ok() const noexcept { return data_ == nullptr; }  // NOLINT(*-identifier-naming)

  template<typename T>
  StatusBuilder& operator<<(const T& msg) & {
    if (data_) {
      data_->stream << msg;
    }
    return *this;
  }

  template<typename T>
  StatusBuilder&& operator<<(const T& msg) && {
    *this << msg;
    return std::move(*this);
  }

  // Switching the builder to append mode.
  enum Append { Append };  // NOLINT(*-identifier-naming)

  StatusBuilder& operator<<(enum Append /* unused */) & { return SetAppend(); }

  StatusBuilder&& operator<<(enum Append /* unused */) && { return std::move(*this).SetAppend(); }

  // Allows the builder to stream message parts in front of the message provided
  // by the status. This can only be used once. calling `SetAppend` later works.
  StatusBuilder& SetPrepend() &;
  StatusBuilder&& SetPrepend() &&;

  // Ensures the message from the original status has been streamed and thus all
  // new streamed messages will be appended. Once this is called `SetPrepend`
  // no longer works.
  StatusBuilder& SetAppend() &;
  StatusBuilder&& SetAppend() &&;

  StatusBuilder& SetPayload(std::string_view type_url, types::ConstructibleFrom<absl::Cord> auto&& payload) & {
    if (data_) {
      data_->status.SetPayload(type_url, absl::Cord(std::forward<decltype(payload)>(payload)));
    }
    return *this;
  }

  StatusBuilder& SetPayload(std::string_view type_url, std::string_view payload) & {
    return SetPayload(type_url, absl::Cord(payload));
  }

  StatusBuilder&& SetPayload(std::string_view type_url, auto&& payload) && {
    return std::move(this->SetPayload(type_url, std::forward<decltype(payload)>(payload)));
  }

  explicit operator absl::Status() const &;

  operator absl::Status() && {  // NOLINT(*-explicit-*)
    return absl::Status{*this};
  }

 private:
  struct Data {
    absl::Status status;
    std::stringstream stream;
    bool appended = false;
  };

  std::unique_ptr<Data> data_;
};

}  // namespace mbo::status

#endif  // MBO_STATUS_STATUS_BUILDER_H_
