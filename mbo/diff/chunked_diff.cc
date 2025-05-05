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

#include "mbo/diff/chunked_diff.h"

#include <cstddef>
#include <string>

#include "absl/status/statusor.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

ChunkedDiff::ChunkedDiff(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options)
    : BaseDiff(lhs, rhs, options), chunk_(lhs, rhs, Header(), Options()) {}

absl::StatusOr<std::string> ChunkedDiff::Finalize() {
  while (!LhsData().Done()) {
    const std::size_t l_idx = LhsData().Idx();
    Chunk().PushLhs(l_idx, RhsData().Idx(), LhsData().Next());
  }
  while (!RhsData().Done()) {
    const std::size_t r_idx = RhsData().Idx();
    Chunk().PushRhs(LhsData().Idx(), r_idx, RhsData().Next());
  }
  return Chunk().MoveOutput();
}

void ChunkedDiff::PushEqual() {
  Chunk().PushBoth(LhsData().Idx(), RhsData().Idx(), LhsData().Line());
  LhsData().Next();
  RhsData().Next();
}

void ChunkedDiff::PushDiff() {
  Chunk().PushLhs(LhsData().Idx(), RhsData().Idx(), LhsData().Line());
  Chunk().PushRhs(LhsData().Idx(), RhsData().Idx(), RhsData().Line());
  Chunk().MoveDiffs();
  LhsData().Next();
  RhsData().Next();
}

}  // namespace mbo::diff
