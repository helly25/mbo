// Copyright 2023 M.Boerger
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

// NOLINTBEGIN(abseil-no-namespace)
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

ABSL_FLAG(std::size_t, unified, 3, "Produces a diff with number lines of context");
ABSL_FLAG(bool, skip_time, false, "Sets the time to the unix epoch 0.");
ABSL_FLAG(std::string, strip_comments, "", "Can be used to strip comments.");
ABSL_FLAG(bool, strip_parsed_comments, true, R"(
Whether to use comment parsing or single character finding. In the former
form (default), the string in `--strip_comments` functions as a single
sub-string, for instance '//'. If found, then all line content to its
right will be removed and any remaining trailing line whitespec will be
stripped. In single character finding mode (`--nostrip_parsed_comments`),
the line gets capped as soon as any character of that set is found and
also all remaining trailing whitespec will be stripped.)");

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTEND(abseil-no-namespace)

namespace {

using mbo::file::Artefact;

int Diff(std::string_view lhs_name, std::string_view rhs_name) {
  const Artefact::Options options{.skip_time = absl::GetFlag(FLAGS_skip_time)};
  const auto lhs = Artefact::Read(lhs_name, options);
  if (!lhs.ok()) {
    LOG(ERROR) << "ERROR: " << lhs.status();
    return 1;
  }
  const auto rhs = Artefact::Read(rhs_name, options);
  if (!rhs.ok()) {
    LOG(ERROR) << "ERROR: " << rhs.status();
    return 1;
  }
  const std::string strip_comments = absl::GetFlag(FLAGS_strip_comments);
  mbo::diff::UnifiedDiff::Options diff_options{
      .context_size = absl::GetFlag(FLAGS_unified),
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
