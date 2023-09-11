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

#ifndef MBO_DIFF_UNIFIED_DIFF_H_
#define MBO_DIFF_UNIFIED_DIFF_H_

#include <cstddef>
#include <string>

#include "absl/status/statusor.h"
#include "mbo/file/artefact.h"
#include "mbo/strings/strip.h"

namespace mbo::diff {

// Creates the unified line-by-line diff between `lhs_text` and `rhs_text`.
//
// `lhs_name` and `rhs_name` are used as the file names in the diff headers,
// `context_size` is ignored (TODO implement).
//
// If left and right are identical, returns an empty string.
//
// The implementation is in no way meant to be optimized. It rather aims at
// matching `diff -du` output API as close as possible. Documentation used:
// https://en.wikipedia.org/wiki/Diff#Unified_format
// https://www.gnu.org/software/diffutils/manual/html_node/Detailed-Unified.html
//
// Most implementations follow LCS (longest common subsequence) approach. Here
// we implement shortest diff approach, both of which work well with the `patch`
// tool.
//
// The complexity of the algorithm used is O(L*R) in the worst case. In practise
// the algorithm is closer to O(max(L,R)) for small differences. In detail the
// algorithm has a complexity of O(max(L,R)+dL*R+L*dR).
class UnifiedDiff final {
 private:
  struct NoCommentStripping {};

 public:
  UnifiedDiff() = delete;

  using StripCommentOptions =
      std::variant<NoCommentStripping, mbo::strings::StripCommentArgs, mbo::strings::StripParsedCommentArgs>;

  struct Options final {
    static const Options& Default() noexcept;

    std::size_t context_size = 3;
    std::string time_format = "%F %H:%M:%E3S %z";

    bool ignore_case = false;
    bool ignore_space_change = false;
    StripCommentOptions strip_comments = NoCommentStripping{};
  };

  static absl::StatusOr<std::string> Diff(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const Options& options = Options::Default());

 private:
  class Impl;
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_UNIFIED_DIFF_H_
