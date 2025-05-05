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

#include "mbo/diff/impl/diff_direct.h"

#include <string>

#include "absl/status/statusor.h"
#include "mbo/diff/diff_options.h"

namespace mbo::diff {

absl::StatusOr<std::string> DiffDirect::Diff(
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const DiffOptions& options) {
  if (lhs.data == rhs.data) {
    return std::string();
  }
  DiffDirect diff(lhs, rhs, options);
  return diff.Compute();
}

absl::StatusOr<std::string> DiffDirect::Compute() {
  while (!LhsData().Done() && !RhsData().Done()) {
    if (CompareEq(0, 0)) {
      Chunk().PushBoth(LhsData().Idx(), RhsData().Idx(), LhsData().Line());
      LhsData().Next();
      RhsData().Next();
    } else {
      Chunk().PushLhs(LhsData().Idx(), RhsData().Idx(), LhsData().Next());
      Chunk().PushRhs(LhsData().Idx(), RhsData().Idx(), RhsData().Next());
      Chunk().MoveDiffs();
    }
  }
  return Finalize();
}

}  // namespace mbo::diff
