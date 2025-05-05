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

#include "mbo/diff/diff.h"

#include <string>

#include "absl/status/statusor.h"
#include "mbo/diff/impl/diff_direct.h"
#include "mbo/diff/impl/diff_unified.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

absl::StatusOr<std::string> Diff::FileDiff(
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const Options& options) {
  switch (options.algorithm) {
    case Diff::Options::Algorithm::kUnified: return DiffUnified::FileDiff(lhs, rhs, options);
    case Diff::Options::Algorithm::kDirect: return DiffDirect::FileDiff(lhs, rhs, options);
  }
  return absl::InvalidArgumentError("Unknown algorithm selected.");
}

}  // namespace mbo::diff
