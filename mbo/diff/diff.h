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

#ifndef MBO_DIFF_DIFF_H_
#define MBO_DIFF_DIFF_H_

#include <string>

#include "absl/status/statusor.h"
#include "mbo/diff/diff_options.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

// Creates the unified line-by-line diff between `lhs_text` and `rhs_text`.
//
// `lhs_name` and `rhs_name` are used as the file names in the diff headers.
//
// If left and right are identical, returns an empty string.
class Diff final {
 public:
  using Options = DiffOptions;

  // Algorithm selected by `options.algorithm`.
  static absl::StatusOr<std::string> FileDiff(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const Options& options = Options::Default());

  Diff() = delete;
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_DIFF_H_
