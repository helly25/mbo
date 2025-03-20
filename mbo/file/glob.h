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

#ifndef MBO_FILE_GLOB_H_
#define MBO_FILE_GLOB_H_

#include <concepts>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "mbo/types/extend.h"
#include "re2/re2.h"

namespace mbo::file {

struct GlobOptions {
  // Whether to apply pattern matching to the path part relative to the given root or to just the
  // full entry path. In other words, if true, then the entries will generally be prefixed with the
  // requested root. If that root is itself relative, then it will be relative to the current path.
  // Note that computing the relative path is not free.
  // If true, then `GlobEntry.rel_path` will be set and `GlobEntry.MaybeRelativePath` will return
  // the relative path as opposed to `entry.path`.
  bool use_rel_path : 1 = false;

  // By default the globbing is done in recursive fashion. But that can be disabled in which case
  // only the given root directory will be iteratoed.
  bool recursive : 1 = true;

  // Options passed to the creation of the `std::filesystem::recursive_directory_iterator`.
  std::filesystem::directory_options dir_options = std::filesystem::directory_options::skip_permission_denied;
};

struct Glob2Re2Options {
  bool allow_star_star : 1 = true;
  bool allow_ranges : 1 = true;

  RE2::Options re2_options;
};

struct RootAndPattern : mbo::types::Extend<RootAndPattern> {
  std::string root;
  std::string pattern;
};

namespace file_internal {

struct GlobParts : mbo::types::Extend<GlobParts> {
  std::string path_pattern;
  std::string file_pattern;
  bool mixed = false;  // Parts or all of the path can be a file component.
};

// Split a glob expression into its path and filename components.
//
// The function is "incomplete" in its ability to identify ranges that can accept slashes.
// - while a range `[/]` is normalized to a single `/` and handled as such,
// - any other range that can accept a slash can either be a path or file name component. Those cases will
//   be reported as just a path component with the `mixed = true`.
absl::StatusOr<GlobParts> GlobSplitParts(std::string_view pattern, const Glob2Re2Options& options = {});

// Convert `pattern` into a RE2 expression.
// Supported syntax:
// - '*' -> '[^/]*'
// - '?' -> '[^/]'
// - '**' -> '.*' which requires `options.allow_star_star` and allows '/'.
//   The generated pattern changes if enclosed in '/' or by pattern start/end to either
//   '(/.+)?' or '(.+/)?'
//   That is, the sequence '**/**' will not enforce a directory level. If that is required, then '*/**' or similar has
//   to be used where the single '*' cannot match a '/' but the presence of a '/' requires a directory level.
// - Ranges (requires `options.allow_ranges`):
//   - '[...]': '...' may not be empty. The result is a matching positive range.
//   - '[!...]': '...' may not be empty. The result is a matching negative range.
//   - '[]]' -> '[\\]]' which matches the ']'.
//   - '[!]]' -> '[^\\]]' which matches everythign but ']'.
//   - ranges may incorrectly match against '/' as that is not handled.
// Character classes (Posix extension):
//   Character classes are translated directly. However, they require to be supported in RE2 as
//   documneted in https://github.com/google/re2/wiki/Syntax.
absl::StatusOr<std::string> Glob2Re2Expression(std::string_view pattern, const Glob2Re2Options& options = {});

// Convert `pattern` into a RE2 instance.
// See `Glob2Re2Expression` for supported syntax.
absl::StatusOr<std::unique_ptr<const RE2>> Glob2Re2(std::string_view pattern, const Glob2Re2Options& options = {});

absl::StatusOr<RootAndPattern> GlobSplit(std::string_view pattern, const Glob2Re2Options& options = {});

}  // namespace file_internal

// Splits a pattern into the root part and the actual pattern for use with `Glob`.
template<typename T>
requires(std::same_as<T, std::filesystem::path> || std::convertible_to<T, std::string_view>)
inline absl::StatusOr<RootAndPattern> GlobSplit(const T& pattern, const Glob2Re2Options& options = {}) {
  if constexpr (std::same_as<T, std::filesystem::path>) {
    return file_internal::GlobSplit(pattern.native(), options);
  } else {
    return file_internal::GlobSplit(pattern, options);
  }
}

struct GlobEntry : mbo::types::Extend<GlobEntry> {
  const std::optional<const std::filesystem::path> rel_path;  // Must be first
  const std::filesystem::directory_entry entry;
  const int depth;

  // Returns the path for the `entry` either as is or relative to `root` if that was requested using
  // `GlobOptions.use_rel_path`.
  const std::filesystem::path& MaybeRelativePath() const noexcept {
    return rel_path.has_value() ? *rel_path : entry.path();
  }

  // Returns the file's size or 0 if the entry is not a regular file.
  // If the file size cannot be retrieved, then the function also returns 0.
  std::size_t FileSize() const noexcept {
    if (!entry.is_regular_file()) {
      return 0;
    }
    std::error_code error;
    std::size_t result = entry.file_size(error);
    return error ? 0 : result;
  }
};

enum class GlobEntryAction {
  // Normal continuation.
  kContinue = 0,

  // Prevents recursion into a directory by calling `disable_recursion_pending` on the dir iterator.
  kDoNotRecurse = 1,

  // Stops the iteration without generating an error. If an error is appropriate, then the callback
  // should return with `absl::CancelledError(reason);
  kStop = 2,
};

using GlobEntryFunc = std::function<absl::StatusOr<GlobEntryAction>(const GlobEntry&)>;

// Recursive glob function that uses `RE2` regular expressions for path/filename matching.
absl::Status GlobRe2(
    const std::filesystem::path& root,
    const RE2& regex,
    const GlobOptions& options,
    const GlobEntryFunc& func);

// Recursive glob function that uses `glob` expressions for path/filename matching.
absl::Status Glob(
    const std::filesystem::path& root,
    std::string_view pattern,
    const Glob2Re2Options& re2_convert_options,
    const GlobOptions& options,
    const GlobEntryFunc& func);

// Recursive glob to be called with `GlobSplit` as pattern argument.
//
// Example:
// ```
// std::vector<std::string> found;
// const auto result = mbo::file::Glob(
//     mbo::file::GlobSplit("/home/you/*", {}, {}, [&](const GlobEntry& e) {
//       found.push_back(e.entry.path().native());
//       return mbo::file::GlobEntryAction::kContinue;
//     }));
// ```
absl::Status Glob(
    const absl::StatusOr<RootAndPattern>& pattern,
    const Glob2Re2Options& re2_convert_options,
    const GlobOptions& options,
    const GlobEntryFunc& func);

}  // namespace mbo::file

#endif  // MBO_FILE_GLOB_H_
