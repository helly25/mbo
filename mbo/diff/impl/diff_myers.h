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

#ifndef MBO_DIFF_IMPL_DIFF_MYERS_H_
#define MBO_DIFF_IMPL_DIFF_MYERS_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "mbo/diff/chunked_diff.h"
#include "mbo/diff/diff_options.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

// Myers diff implementation ("An O(ND) Difference Algorithm and Its
// Variations", Myers 1986) using the linear space middle-snake divide and
// conquer variant that GNU diff and git are built on.
//
// Lines are interned into integer tokens up front (all comparison options are
// already materialized in the preprocessed line cache), so the search only
// compares integers. The resulting edit script is minimal and gets replayed
// through `ChunkedDiff`, meaning chunking, context handling, filtering and
// all output formats are shared with the other algorithms.
//
// Complexity is O((L+R)*D) time and O(L+R) space where D is the number of
// differing lines. Once a subdivision exceeds a cost of max(64, sqrt(L+R))
// it splits at the furthest reaching path found so far (the same heuristic
// git uses), which bounds pathological inputs at the expense of minimality.
class DiffMyers final : private ChunkedDiff {
 public:
  static absl::StatusOr<std::string> FileDiff(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const DiffOptions& options);

  DiffMyers() = delete;

 private:
  using Token = std::uint32_t;

  // A pending piece of work: either a not yet diffed range of lines
  // (`*_end` exclusive) or, if `equals != 0`, a run of common lines to emit.
  struct Span {
    std::size_t lhs_begin = 0;
    std::size_t lhs_end = 0;
    std::size_t rhs_begin = 0;
    std::size_t rhs_end = 0;
    std::size_t equals = 0;
  };

  // A (possibly empty) run of common lines that splits a span into two
  // independently diffable halves.
  struct Snake {
    std::size_t lhs_begin = 0;
    std::size_t rhs_begin = 0;
    std::size_t length = 0;
  };

  DiffMyers(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options);

  absl::StatusOr<std::string> Compute();

  void Tokenize();
  void Loop();
  Snake FindMiddleSnake(const Span& span);

  std::vector<Token> lhs_tokens_;
  std::vector<Token> rhs_tokens_;
  std::vector<std::ptrdiff_t> fwd_;  // Forward furthest-reaching x per diagonal.
  std::vector<std::ptrdiff_t> bwd_;  // Backward equivalent, in reversed coordinates.
  std::size_t max_cost_ = 0;
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_IMPL_DIFF_MYERS_H_
