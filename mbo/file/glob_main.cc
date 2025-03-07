// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "absl//container/btree_map.h"
#include "absl//container/btree_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "mbo/file/glob.h"
#include "mbo/strings/numbers.h"

// NOLINTBEGIN: Bad flags macros

ABSL_FLAG(bool, dotdir, true, "Whether to allow or skip directories starting with a '.'.");
ABSL_FLAG(bool, dotfile, true, "Whether to allow or skip files starting with a '.'.");
ABSL_FLAG(bool, entries, true, "Whether to show entries.");
ABSL_FLAG(bool, fast, false, "Whether to show entries fast (no buffering and no field alignment).");
ABSL_FLAG(
    bool,
    re2,
    false,
    "Whether to use re2 regular exressions. This program uses Google's Re2: "
    "https://github.com/google/re2/wiki/Syntax.");
ABSL_FLAG(bool, size, false, "Whether to show file sizes.");
ABSL_FLAG(bool, sum, false, "Whether to show summary. This is automatically enabled if --sum_every > 0.");
ABSL_FLAG(std::size_t, sum_every, 0, "If greater zero, then show a summary after every X entry.");
ABSL_FLAG(bool, sum_extensions, false, "Whether to show an extension summary.");

// NOLINTEND

namespace fs = std::filesystem;

using mbo::strings::BigNumber;
using mbo::strings::BigNumberLen;

class Entries {
 public:
  ~Entries() = default;

  Entries()
      : dotdir_(absl::GetFlag(FLAGS_dotdir)),
        dotfile_(absl::GetFlag(FLAGS_dotfile)),
        show_fast_(absl::GetFlag(FLAGS_fast)),
        show_size_(absl::GetFlag(FLAGS_size)),
        sum_extensions_(absl::GetFlag(FLAGS_sum_extensions)),
        sum_every_(absl::GetFlag(FLAGS_sum_every)),
        stats_{
            {"Dirs", dirs_},          {"FileSize", bytes_}, {"Files", files_}, {"Links", links_},
            {"MaxDepth", max_depth_}, {"Other", other_},    {"Total", seen_},
        } {}

  Entries(const Entries&) = delete;
  Entries& operator=(const Entries&) = delete;
  Entries(Entries&&) noexcept = default;
  Entries& operator=(Entries&&) = delete;

  mbo::file::GlobEntryAction Add(const mbo::file::GlobEntry& entry) {
    // Filter
    if (entry.entry.is_directory()) {
      if (!dotdir_ && entry.entry.path().filename().string().starts_with('.')) {
        return mbo::file::GlobEntryAction::kDoNotRecurse;
      }
    } else if (entry.entry.is_regular_file()) {
      if (!dotfile_ && entry.entry.path().filename().string().starts_with('.')) {
        return mbo::file::GlobEntryAction::kContinue;
      }
    }
    // Actual Add
    if (entry.entry.is_directory()) {
      ++dirs_;
    } else if (entry.entry.is_symlink()) {
      ++links_;
    } else if (entry.entry.is_regular_file()) {
      ++files_;
      const std::size_t size = entry.FileSize();
      bytes_ += size;
      bytes_len_ = std::max(bytes_len_, BigNumberLen(size));
      if (sum_extensions_) {
        if (entry.entry.path().has_extension()) {
          const auto ext = entry.entry.path().extension().string();
          ++extensions_[absl::StrCat("FileExt(", ext, ")")];
          extensions_[absl::StrCat("FileSize(", ext, ")")] += size;
        } else if (entry.entry.path().filename().string().starts_with(".")) {
          // Dot files (/.conf) are shown as the full file name.
          const auto ext = entry.entry.path().filename().string();
          ++extensions_[absl::StrCat("FileExt(", ext, ")")];
          extensions_[absl::StrCat("FileSize(", ext, ")")] += size;
        } else {
          ++extensions_["FileExt()"];
          extensions_["FileSize()"] += size;
        }
      }
    } else {
      ++other_;
    }
    max_depth_ = std::max(max_depth_, static_cast<std::size_t>(entry.depth));
    ++seen_;
    // Maybe Print
    if (show_fast_) {
      PrintFastEntry(entry);
    } else {
      entries_.emplace(entry);
    }
    if (sum_every_ > 0 && seen_ % sum_every_ == 0) {
      std::cout << "\n";
      PrintSummary();
    }
    return mbo::file::GlobEntryAction::kContinue;
  }

  void PrintEntry(const mbo::file::GlobEntry& entry) const {
    if (show_size_) {
      std::cout << absl::StreamFormat("%*s ", bytes_len_, BigNumber(entry.FileSize()));
    }
    std::cout << entry.MaybeRelativePath().string() << "\n";
  }

  void PrintFastEntry(const mbo::file::GlobEntry& entry) const {
    if (show_size_) {
      std::cout << absl::StreamFormat("%*s ", bytes_len_, BigNumber(entry.FileSize()));
    }
    std::cout << entry.MaybeRelativePath().string() << "\n";
  }

  void PrintAllEntries() const {
    for (const auto& entry : entries_) {
      PrintEntry(entry);
    }
  }

  void PrintSummary(bool show_extensions = false) const {
    std::size_t name_len{1};
    unsigned val_len{1};
    absl::btree_map<std::string, std::size_t> data;
    if (show_extensions) {
      for (const auto& [name, count] : extensions_) {
        const auto len = data.emplace(absl::StrCat(name, ":"), count).first->first.size();
        name_len = std::max(name_len, len);
        val_len = std::max(val_len, BigNumberLen(count));
      }
    }
    for (const auto& [name, value] : stats_) {
      const auto len = data.emplace(absl::StrCat(name, ":"), value).first->first.size();
      name_len = std::max(name_len, len);
      val_len = std::max(val_len, BigNumberLen(value));
    }
    for (const auto& [name, value] : data) {
      std::cout << absl::StrFormat("%-*s %*s\n", name_len, name, val_len, BigNumber(value));
    }
  }

 private:
  const bool dotdir_{true};
  const bool dotfile_{true};
  const bool show_fast_{false};
  const bool show_size_{false};
  const bool sum_extensions_{false};
  const std::size_t sum_every_{0};
  absl::btree_set<mbo::file::GlobEntry> entries_;
  absl::btree_map<std::string, std::size_t> extensions_;
  std::size_t dirs_{0};
  std::size_t links_{0};
  std::size_t files_{0};
  std::size_t other_{0};
  std::size_t bytes_{0};
  std::size_t max_depth_{0};
  std::size_t seen_{0};
  const absl::btree_map<std::string, std::size_t&> stats_;
  unsigned bytes_len_{1};
};

constexpr std::string_view kUsage = R"usage(glob [<flags>*] <path> [<pattern>]

Glog is a simple recursive file finder that can produce a summary. If a pattern
is given then it follows `fnmame` convention or Google's RE2 is --re2 is set.

The default glob expressions support:
- '*':          Any number of characters not including '/'.
- '**':         Any number of characters including '/'.
- '?':          A single character (not '/').
- '\?':         the character '?'. Note that all characters can be escaped.
- '[<range>]':  Requiring the given <range> or ranges.
- '[!<range>]': Excluding the given <range> or ranges.
- '[]':         An empty range is not allowed.
                This means that ']' can be allowed as the first character of a
                range, e.g. '[!]]' allows all but ']'.

A range is interpreted as follows:
- 'x':          The single character x.
- 'x-z':        Any character from 'x' to 'z' inclusive.
- 'x-' or '-x': The characters 'x' and '-'.

Examples:

- Show all files under the current directory with their sizes but un-aligned:
    glob . --fast --size

- Show only a summary of all files:
    glob . --noentries --sum

- Show only a summary for all files excluding all directories and files starting
  with a '.' while showing stats per file extension:
    glob . --nodotdir --nodotfile --noentries --sum_extensions

- Show only a summary of files under the current directory whose extension is
  one of [.cc, .cpp, .h] and which are not under any directory that starts with
  a dot (.):
    glob . '.*[.](cc|cpp|h)' --nodotdir --noentries --re2 --sum
    glob . '([^/]|/[^.])*[.](cc|cpp|h)' --noentries --re2 --sum

Flags:)usage";

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage(kUsage);
  const std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  if (args.empty() || args.size() > 3) {
    std::cerr << "Requires at most 2 arguments: glob [<path>] [<pattern>]\n";
    return 1;
  }
  if (absl::GetFlag(FLAGS_sum_every) > 0 || absl::GetFlag(FLAGS_sum_extensions)) {
    absl::SetFlag(&FLAGS_sum, true);
  }
  const fs::path root = fs::path(args.size() >= 2 ? args[1] : ".").lexically_normal();
  const std::string_view default_pattern = absl::GetFlag(FLAGS_re2) ? ".*" : "**";
  const std::string pattern{args.size() == 3 ? args[2] : default_pattern};
  Entries entries;
  const auto collect = std::bind_front(&Entries::Add, &entries);
  const auto result = absl::GetFlag(FLAGS_re2) ? mbo::file::GlobRe2(root, pattern, {}, collect)
                                               : mbo::file::Glob(root, pattern, {}, {}, collect);
  if (!result.ok()) {
    std::cerr << "ERROR: " << result.message() << "\n";
    return 1;
  }
  if (absl::GetFlag(FLAGS_entries) && !absl::GetFlag(FLAGS_fast)) {
    entries.PrintAllEntries();
  }
  if (absl::GetFlag(FLAGS_sum)) {
    std::cout << "\n";
    entries.PrintSummary(absl::GetFlag(FLAGS_sum_extensions));
  }
  return 0;
}
