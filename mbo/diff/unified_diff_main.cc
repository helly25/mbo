// Copyright 2023 M. Boerger (helly25.com)
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

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "mbo/diff/unified_diff.h"
#include "mbo/diff/update_absl_log_flags.h"
#include "mbo/file/artefact.h"
#include "mbo/strings/indent.h"
#include "mbo/strings/strip.h"
#include "re2/re2.h"

// NOLINTBEGIN(abseil-no-namespace)
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

ABSL_FLAG(bool, ignore_blank_lines, false, "Ignore chunks which include only blank lines.");
ABSL_FLAG(bool, ignore_case, false, "Whether to ignore the case of letters.");
ABSL_FLAG(
    std::string,
    ignore_matching_lines,
    "",
    "\
Ignore lines that match this regexp (https://github.com/google/re2/wiki/Syntax). By default this \
applies only for chunks where all insertions and deletions match. Using --ignore_matching_chunks=0 \
this can be changed to apply to lines where both the left and the right side match the given \
regular expression.");
ABSL_FLAG(
    bool,
    ignore_matching_chunks,
    true,
    "\
Controls whether `--ignore_matching_lines` applies to full chunks (Default) or just to single lines.");
ABSL_FLAG(bool, ignore_space_change, false, "Ignore leading and trailing whitespace changes.");
ABSL_FLAG(std::string, file_header_use, "both", R"(Select which file header to use:
- both: Both file names are used (left uses left file name and right uses right file name).
- left: The left and right header both use left file name.
- right: The left and right header both use right file name.
)");
ABSL_FLAG(std::size_t, max_lines, 0, "Read (and compare) at most the given number of lines (ignored if 0).");
ABSL_FLAG(bool, skip_left_deletions, false, "Ignore left deletions.");
ABSL_FLAG(bool, skip_time, false, "Sets the time to the unix epoch 0.");
ABSL_FLAG(std::string, strip_comments, "", "Can be used to strip comments.");
ABSL_FLAG(
    std::string,
    strip_file_header_prefix,
    "",
    "If this is a prefix to a filename in the header, then remove from header.");
ABSL_FLAG(
    bool,
    strip_parsed_comments,
    true,
    "\
Whether to perform line parsing (default) or simple substring finding. Parsing respects single and \
double quotes as well as escape sequences, see (https://en.cppreference.com/w/cpp/language/escape \
and custom escapes for any of '(){}[]<>,;&'). If the substring is found, then all line content to \
its right will be removed and any remaining trailing line whitespace will be stripped. In the \
latter form of simple substring finding, the substring will be searched for as is.");
ABSL_FLAG(std::size_t, unified, 3, "Produces a diff with number lines of context");

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTEND(abseil-no-namespace)

namespace {

using mbo::file::Artefact;

absl::StatusOr<Artefact> Read(std::string_view file_name) {
  const Artefact::Options options{.skip_time = absl::GetFlag(FLAGS_skip_time)};
  const std::size_t max_lines = absl::GetFlag(FLAGS_max_lines);
  auto result =
      max_lines > 0 ? Artefact::ReadMaxLines(file_name, max_lines, options) : Artefact::Read(file_name, options);
  if (!result.ok()) {
    LOG(ERROR) << "ERROR: " << result.status();
  }
  return result;
}

mbo::diff::UnifiedDiff::Options::FileHeaderUse GetFileHeaderUse() {
  const std::string mode = absl::GetFlag(FLAGS_file_header_use);
  if (mode == "left") {
    return mbo::diff::UnifiedDiff::Options::FileHeaderUse::kLeft;
  }
  if (mode == "right") {
    return mbo::diff::UnifiedDiff::Options::FileHeaderUse::kRight;
  }
  return mbo::diff::UnifiedDiff::Options::FileHeaderUse::kBoth;
}

int Diff(std::string_view lhs_name, std::string_view rhs_name) {
  const auto lhs = Read(lhs_name);
  if (!lhs.ok()) {
    return 1;
  }
  const auto rhs = Read(rhs_name);
  if (!rhs.ok()) {
    return 1;
  }
  const std::string strip_comments = absl::GetFlag(FLAGS_strip_comments);
  mbo::diff::UnifiedDiff::Options diff_options{
      .context_size = absl::GetFlag(FLAGS_unified),
      .file_header_use = GetFileHeaderUse(),
      .ignore_blank_lines = absl::GetFlag(FLAGS_ignore_blank_lines),
      .ignore_case = absl::GetFlag(FLAGS_ignore_case),
      .ignore_matching_lines = [&]() -> std::optional<RE2> {
        std::string regex = absl::GetFlag(FLAGS_ignore_matching_lines);
        if (regex.empty()) {
          return std::nullopt;
        }
        return std::make_optional<RE2>(regex);
      }(),
      .ignore_space_change = absl::GetFlag(FLAGS_ignore_space_change),
      .skip_left_deletions = absl::GetFlag(FLAGS_skip_left_deletions),
      .strip_comments = [&]() -> mbo::diff::UnifiedDiff::StripCommentOptions {
        if (strip_comments.empty()) {
          return {};
        } else if (absl::GetFlag(FLAGS_strip_parsed_comments)) {
          return mbo::strings::StripParsedCommentArgs{
              .parse = {
                  .stop_at_str = strip_comments,
                  .remove_quotes = false,
              }};
        } else {
          return mbo::strings::StripCommentArgs{
              .comment_start = strip_comments,
          };
        }
      }(),
      .strip_file_header_prefix = absl::GetFlag(FLAGS_strip_file_header_prefix),
  };
  const auto result = mbo::diff::UnifiedDiff::Diff(*lhs, *rhs, diff_options);
  if (!result.ok()) {
    LOG(ERROR) << "ERROR: " << result.status();
    return 1;
  }
  if (!result->empty()) {
    std::cout << *result;
    return 1;
  }
  return 0;
}

}  // namespace

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(mbo::strings::DropIndent(R"(
    [ <flags>> ] <old/lef> <new/right>

    Performs a unified diff (diff -du) between files <old/left> and <new/right>.
  )"));
  absl::InitializeLog();
  const std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  mbo::UpdateAbslLogFlags();
  if (args.size() != 3) {  // [0] = program
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cerr << "Exactly two files are required. Use: " << fs::path(argv[0]).filename().string() << " --help"
              << std::endl;
    return 1;
  }
  return Diff(args[1], args[2]);
}
