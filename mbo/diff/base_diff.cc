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

#include "mbo/diff/base_diff.h"

#include <cstddef>
#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "mbo/diff/diff_options.h"
#include "mbo/file/artefact.h"
#include "re2/re2.h"

namespace mbo::diff {

std::string BaseDiff::FileHeaders(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options) {
  std::string output;
  if (options.file_header_use != DiffOptions::FileHeaderUse::kNone) {
    absl::StrAppend(&output, "--- ", SelectFileHeader(lhs, lhs, rhs, options), "\n");
    absl::StrAppend(&output, "+++ ", SelectFileHeader(rhs, lhs, rhs, options), "\n");
  }
  return output;
}

std::string BaseDiff::FileHeader(const file::Artefact& info, const DiffOptions& options) {
  std::string_view name;
  if (options.strip_file_header_prefix.find_first_of(".*?()[]|") == std::string::npos) {
    name = absl::StripPrefix(info.name, options.strip_file_header_prefix);
  } else {
    name = info.name;
    RE2::Consume(&name, options.strip_file_header_prefix);
  }
  return absl::StrCat(info.name.empty() ? "-" : name, " ", absl::FormatTime(options.time_format, info.time, info.tz));
}

std::string BaseDiff::SelectFileHeader(
    const file::Artefact& either,
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const DiffOptions& options) {
  switch (options.file_header_use) {
    case DiffOptions::FileHeaderUse::kBoth:
      break;  // Do not use default, but break for this case, so it becomes the function return.
    case DiffOptions::FileHeaderUse::kNone: return "";
    case DiffOptions::FileHeaderUse::kLeft: return FileHeader(lhs, options);
    case DiffOptions::FileHeaderUse::kRight: return FileHeader(rhs, options);
  }
  return FileHeader(either, options);
}

bool BaseDiff::CompareEq(std::size_t lhs, std::size_t rhs) const {
  const auto& lhs_cache = LhsData().GetCache(lhs);
  const auto& rhs_cache = RhsData().GetCache(rhs);
  if (lhs_cache.matches_ignore && rhs_cache.matches_ignore) {
    return true;
  }
  if (Options().ignore_case) {
    return absl::EqualsIgnoreCase(lhs_cache.processed, rhs_cache.processed);
  } else {
    return lhs_cache.processed == rhs_cache.processed;
  }
}

}  // namespace mbo::diff
