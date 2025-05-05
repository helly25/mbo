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

#ifndef MBO_DIFF_IMPL_DIFF_UNIFIED_H_
#define MBO_DIFF_IMPL_DIFF_UNIFIED_H_

#include <cstddef>
#include <string>

#include "absl/status/statusor.h"
#include "mbo/diff/chunked_diff.h"
#include "mbo/diff/diff_options.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

class DiffUnified final : private ChunkedDiff {
 public:
  static absl::StatusOr<std::string> Diff(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const DiffOptions& options);

  DiffUnified() = delete;

 protected:
  DiffUnified(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options)
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

#endif  // MBO_DIFF_IMPL_DIFF_UNIFIED_H_
