// Copyright 2023 M. Boerger (helly25.com)
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

#include "mbo/file/artefact.h"

#include <string>
#include <string_view>

#include "absl/time/time.h"
#include "mbo/file/file.h"

namespace mbo::file {

absl::StatusOr<Artefact> Artefact::Read(std::string_view filename, const Artefact::Options& options) {
  const auto data = mbo::file::GetContents(filename);
  if (!data.ok()) {
    return data.status();
  }
  const auto time = [&]() -> absl::StatusOr<absl::Time> {
    if (options.skip_time) {
      return absl::UnixEpoch();
    } else {
      return mbo::file::GetMTime(filename);
    }
  }();
  if (!time.ok()) {
    return time.status();
  }
  return Artefact{
      .data = *data,
      .name = std::string(filename),
      .time = *time,
      .tz = options.tz,
  };
}

absl::StatusOr<Artefact> Artefact::ReadMaxLines(std::string_view filename, std::size_t max_lines, const Artefact::Options& options) {
  const auto data = mbo::file::GetMaxLines(filename, max_lines);
  if (!data.ok()) {
    return data.status();
  }
  const auto time = [&]() -> absl::StatusOr<absl::Time> {
    if (options.skip_time) {
      return absl::UnixEpoch();
    } else {
      return mbo::file::GetMTime(filename);
    }
  }();
  if (!time.ok()) {
    return time.status();
  }
  return Artefact{
      .data = *data,
      .name = std::string(filename),
      .time = *time,
      .tz = options.tz,
  };
}

}  // namespace mbo::file
