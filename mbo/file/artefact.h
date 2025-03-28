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

#ifndef MBO_FILE_ARTIFACT_H_
#define MBO_FILE_ARTIFACT_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "absl/time/time.h"

namespace mbo::file {

struct Artefact final {
  struct Options final {
    static inline Options Default() noexcept;

    bool skip_time = false;
    absl::TimeZone tz = absl::UTCTimeZone();
  };

  static absl::StatusOr<Artefact> Read(std::string_view filename, const Options& options = Options::Default());
  static absl::StatusOr<Artefact> ReadMaxLines(
      std::string_view filename,
      std::size_t max_lines,
      const Options& options = Options::Default());

  std::string data;                            // Artefact's 'data' (text or binary content).
  std::string name = "-";                      // Artefact's 'name'.
  absl::Time time = absl::FromUnixSeconds(0);  // Last update/modify 'time'.
  absl::TimeZone tz = absl::UTCTimeZone();
};

// NOTE: Cannot be `constexpr` because `absl::TimeZone` is not a literal type.
inline Artefact::Options Artefact::Options::Default() noexcept {
  return {};
}

}  // namespace mbo::file

#endif  // MBO_FILE_ARTIFACT_H_
