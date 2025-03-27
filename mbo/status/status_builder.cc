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

#include "mbo/status/status_builder.h"

#include <string_view>
#include <utility>

#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"

namespace mbo::status {

StatusBuilder& StatusBuilder::SetPrepend() & {
  return *this;
}

StatusBuilder&& StatusBuilder::SetPrepend() && {
  this->SetPrepend();
  return std::move(*this);
}

StatusBuilder& StatusBuilder::SetAppend() & {
  if (data_) {
    if (!data_->appended) {
      data_->stream << data_->status.message();
    }
    data_->appended = true;
  }
  return *this;
}

StatusBuilder&& StatusBuilder::SetAppend() && {
  this->SetAppend();
  return std::move(*this);
}

StatusBuilder::operator absl::Status() const & {
  if (!data_) {
    return absl::OkStatus();
  }
  absl::Status result(
      data_->status.code(),
      data_->appended ? data_->stream.str() : absl::StrCat(data_->status.message(), data_->stream.str()));
  data_->status.ForEachPayload(
      [&result](std::string_view url, const absl::Cord& payload) { result.SetPayload(url, payload); });
  return result;
}

}  // namespace mbo::status
