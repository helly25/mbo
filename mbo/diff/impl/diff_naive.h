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

#ifndef MBO_DIFF_IMPL_DIFF_NAIVE_H_
#define MBO_DIFF_IMPL_DIFF_NAIVE_H_

#include <cstddef>
#include <string>

#include "absl/status/statusor.h"
#include "mbo/diff/chunked_diff.h"
#include "mbo/diff/diff_options.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

// Naive line diff implementation (previously misnamed `unified`, which is an
// output format rather than an algorithm).
//
// The implementation is in no way meant to be optimized. On a mismatch it
// greedily resynchronizes on the closest matching line pair, which keeps the
// extra ignore/strip options easy to reason about, but does not produce
// minimal diffs (see `DiffMyers` for that). Documentation used:
// https://en.wikipedia.org/wiki/Diff#Unified_format
// https://www.gnu.org/software/diffutils/manual/html_node/Detailed-Unified.html
//
// The complexity of the algorithm used is O(L*R) in the worst case. In practise
// the algorithm is closer to O(max(L,R)) for small differences. In detail the
// algorithm has a complexity of O(max(L,R)+dL*R+L*dR).
class DiffNaive final : private ChunkedDiff {
 public:
  static absl::StatusOr<std::string> FileDiff(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const DiffOptions& options);

  DiffNaive() = delete;

 protected:
  DiffNaive(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options)
      : ChunkedDiff(lhs, rhs, options) {}

  absl::StatusOr<std::string> Compute() {
    Loop();
    return Finalize();
  }

 private:
  void Loop();
  void LoopBoth();
  bool FindNext();
  std::tuple<std::size_t, std::size_t, bool> FindNextRight();
  std::tuple<std::size_t, std::size_t, bool> FindNextLeft();
  bool PastMaxDiffChunkLength(std::size_t& loop);
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_IMPL_DIFF_NAIVE_H_
