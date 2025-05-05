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
    // Unified diff like `diff -u` or `git diff`.
    // See implementation in `mbo::diff::DiffUnified`.
    kUnified = 0,

    // Compare two inputs and output in direct side by side format.
    // See implementation in `mbo::diff::DiffDirect`.
    kDirect = 1,
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

  static std::optional<FileHeaderUse> ParseFileHeaderUse(std::string_view flag);

  static std::optional<RegexReplace> ParseRegexReplaceFlag(std::string_view flag);

  static const DiffOptions& Default() noexcept;

  Algorithm algorithm = Algorithm::kUnified;

  std::size_t context_size = 3;

  FileHeaderUse file_header_use = FileHeaderUse::kBoth;

  bool ignore_blank_lines : 1 = false;
  bool ignore_case : 1 = false;
  bool ignore_matching_chunks : 1 = true;
  bool ignore_all_space : 1 = false;
  bool ignore_consecutive_space : 1 = false;
  bool ignore_trailing_space : 1 = false;
  bool show_chunk_headers : 1 = true;
  bool skip_left_deletions : 1 = false;

  std::optional<RE2> ignore_matching_lines;
  StripCommentOptions strip_comments = NoCommentStripping{};
  std::optional<RegexReplace> regex_replace_lhs;
  std::optional<RegexReplace> regex_replace_rhs;
  std::string strip_file_header_prefix;

  std::size_t max_diff_chunk_length = 1'337'000;  // NOLINT(*-magic-numbers)

  std::string time_format = "%F %H:%M:%E3S %z";
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_DIFF_OPTIONS_H_
