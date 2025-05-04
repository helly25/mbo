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

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "absl/status/statusor.h"
#include "mbo/file/artefact.h"
#include "mbo/strings/strip.h"
#include "re2/re2.h"

namespace mbo::diff {

// Creates the unified line-by-line diff between `lhs_text` and `rhs_text`.
//
// `lhs_name` and `rhs_name` are used as the file names in the diff headers.
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
class Diff final {
 public:
  struct NoCommentStripping final {};

  using StripCommentOptions =
      std::variant<NoCommentStripping, mbo::strings::StripCommentArgs, mbo::strings::StripParsedCommentArgs>;

  struct RegexReplace {
    std::unique_ptr<RE2> regex;
    std::string replace;
  };

  static std::optional<RegexReplace> ParseRegexReplaceFlag(std::string_view flag);

  struct Options final {
    enum class FileHeaderUse {
      kBoth = 0,   // In header both file names are used (left uses left file name and right uses right file name).
      kLeft = 1,   // In header left and right file both use left file name.
      kRight = 2,  // In header left and right file both use right file name.
    };

    enum class Algorithm {
      // Unified diff like `diff -u` or `git diff`.
      kUnified = 0,

      // Compare two inputs and output in direct side by side format.
      // That is similar to unified format, but it is assumed that the left and
      // right content are meant to line up with only changed lines and no added
      // or removed lines. The changes are then presented next to each other.
      // This mode does not allow for context.
      kDirect = 1,
    };

    static const Options& Default() noexcept;

    std::size_t context_size = 3;

    FileHeaderUse file_header_use = FileHeaderUse::kBoth;
    bool ignore_blank_lines = false;
    bool ignore_case = false;
    bool ignore_matching_chunks = true;
    std::optional<RE2> ignore_matching_lines;
    bool ignore_all_space = false;
    bool ignore_consecutive_space = false;
    bool ignore_trailing_space = false;
    bool skip_left_deletions = false;
    StripCommentOptions strip_comments = NoCommentStripping{};
    std::optional<RegexReplace> regex_replace_lhs;
    std::optional<RegexReplace> regex_replace_rhs;
    std::string strip_file_header_prefix;

    std::size_t max_diff_chunk_length = 1'337'000;  // NOLINT(*-magic-numbers)

    std::string time_format = "%F %H:%M:%E3S %z";

    Algorithm algorithm = Algorithm::kUnified;
  };

  // Compare two inputs and output differences in unified format.
  static absl::StatusOr<std::string> DiffUnified(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const Options& options = Options::Default());

  // Compare two inputs and output in direct side by side format.
  // See `Algorithm::kDirect`.
  static absl::StatusOr<std::string> DiffDirect(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const Options& options = Options::Default());

  // Algorithm selected by `options.algorithm`.
  static absl::StatusOr<std::string> DiffSelect(
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const Options& options = Options::Default());

  Diff() = delete;
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_DIFF_H_
