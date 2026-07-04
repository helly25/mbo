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

#ifndef MBO_DIFF_DIFF_OPTIONS_H_
#define MBO_DIFF_DIFF_OPTIONS_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "mbo/strings/strip.h"
#include "re2/re2.h"

namespace mbo::diff {

struct DiffOptions final {
  enum class Algorithm {
    // Naive line diff that greedily resynchronizes on the closest matching
    // line; simple but not minimal. Selectable as `naive`.
    // See implementation in `mbo::diff::DiffNaive`.
    kNaive = 0,

    // Compare two inputs line by line without resynchronization.
    // See implementation in `mbo::diff::DiffDirect`.
    kDirect = 1,

    // Myers O(ND) line diff producing minimal diffs (the algorithm GNU diff
    // and git implement). The default. The old flag name `unified` maps here
    // (and implies unified format). See `mbo::diff::DiffMyers`.
    kMyers = 2,
  };

  enum class OutputFormat {
    // Unified output: `@@ -R +R @@` chunks whose lines use the one-char
    // prefixes '-'/'+'/' ' (like `diff -u` or `git diff`).
    kUnified = 0,

    // Context output: `*** R ****` / `--- R ----` sections whose lines use
    // the two-char prefixes '! '/'- '/'+ '/'  ' (like `diff -c`).
    kContext = 1,

    // Normal output: `RaR`/`RdR`/`RcR` commands whose lines use the prefixes
    // '< '/'> ', without context lines or file headers (like plain `diff`).
    kNormal = 2,

    // Side-by-side output: two space-padded columns of `side_by_side_width`
    // total width with a 3-char ' X ' gutter, X being ' ' (common), '|'
    // (changed), '<' (left only) or '>' (right only). Like `diff -y
    // --expand-tabs`; meant for eyeballing, not for `patch`. The CLI defaults
    // `--context` to unbounded here (full files); an explicit context renders
    // only the chunks around changes.
    kSideBySide = 3,
  };

  enum class FileHeaderUse {
    kNone = 0,   // No file header will be emoitted.
    kBoth = 1,   // In header both file names are used (left uses left file name and right uses right file name).
    kLeft = 2,   // In header left and right file both use left file name.
    kRight = 3,  // In header left and right file both use right file name.
  };

  struct RegexReplace {
    std::unique_ptr<RE2> regex;
    std::string replace;
  };

  struct NoCommentStripping final {};

  using StripCommentOptions =
      std::variant<NoCommentStripping, mbo::strings::StripCommentArgs, mbo::strings::StripParsedCommentArgs>;

  static std::optional<Algorithm> ParseAlgorithmFlag(std::string_view flag);

  static std::optional<OutputFormat> ParseOutputFormatFlag(std::string_view flag);

  static std::optional<FileHeaderUse> ParseFileHeaderUse(std::string_view flag);

  static std::optional<RegexReplace> ParseRegexReplaceFlag(std::string_view flag);

  static const DiffOptions& Default() noexcept;

  Algorithm algorithm = Algorithm::kMyers;

  OutputFormat output_format = OutputFormat::kUnified;

  std::size_t context_size = 3;

  // Total width of `OutputFormat::kSideBySide` output (two columns of
  // `(side_by_side_width - 3) / 2` characters plus the gutter); longer lines
  // are truncated.
  std::size_t side_by_side_width = 130;  // NOLINT(*-magic-numbers)

  FileHeaderUse file_header_use = FileHeaderUse::kBoth;

  bool ignore_blank_lines : 1 = false;
  bool ignore_case : 1 = false;
  bool ignore_matching_chunks : 1 = true;
  bool ignore_all_space : 1 = false;
  bool ignore_consecutive_space : 1 = false;
  bool ignore_trailing_space : 1 = false;
  // Disables the `kMyers` cost cap: diffs are guaranteed minimal at the price
  // of O((L+R)*D) worst case time on highly divergent inputs (like GNU
  // `diff --minimal`). No effect on the other algorithms.
  bool minimal : 1 = false;
  bool show_chunk_headers : 1 = true;
  bool skip_left_deletions : 1 = false;

  // Lines matching the expression compare equal to each other. When combined
  // with `ignore_case` the expression should be written case-insensitively
  // (e.g. `(?i)...`): a matching and a non-matching line that differ only in
  // case compare equal under `kNaive` but not under `kMyers` (token based).
  std::optional<RE2> ignore_matching_lines;
  StripCommentOptions strip_comments = NoCommentStripping{};
  std::optional<RegexReplace> regex_replace_lhs;
  std::optional<RegexReplace> regex_replace_rhs;
  std::string strip_file_header_prefix;

  // Safety bound of the `kNaive` resynchronization loop. `kMyers` bounds its
  // work with an internal cost cap instead and `kDirect` needs no bound.
  std::size_t max_diff_chunk_length = 1'337'000;  // NOLINT(*-magic-numbers)

  std::string time_format = "%F %H:%M:%E3S %z";
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_DIFF_OPTIONS_H_
