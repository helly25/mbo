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

// If the macro `TEST_FNMATCH` is defined, and the <fnmatch.h> include is available, then the test
// for `Glob2Re2` verifies that its result matches that of `fnmatch`.
#define TEST_FNMATCH

#include "mbo/file/glob.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <source_location>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/status/status_macros.h"
#include "mbo/testing/status.h"
#include "re2/re2.h"

#if !__has_include(<fnmatch.h>)
# undef TEST_FNMATCH
#endif
#ifdef TEST_FNMATCH
# include <fnmatch.h>
#endif  // TEST_FNMATCH

namespace mbo::file {
namespace {

using namespace mbo::file::file_internal;

using ::mbo::testing::IsOk;
using ::mbo::testing::IsOkAndHolds;
using ::mbo::testing::StatusIs;
using ::testing::NotNull;
using ::testing::UnorderedElementsAreArray;

struct GlobTest : ::testing::Test {
  static void Glob2Re2Match(
      std::string_view glob_pattern,
      std::string_view text,
      bool expected = true,
      const std::source_location sloc = std::source_location::current()) {
    SCOPED_TRACE(absl::StrCat(
        "\n", sloc.file_name(), ":", sloc.line(), "\n  Pattern: '", glob_pattern, "'\n  Text: '", text, "'"));
    MBO_ASSERT_OK_AND_MOVE_TO(Glob2Re2(glob_pattern), const std::unique_ptr<const RE2> re2_pattern);
    ASSERT_THAT(re2_pattern, NotNull());
    EXPECT_THAT(re2::RE2::FullMatch(text, *re2_pattern), expected);
#ifdef TEST_FNMATCH
    EXPECT_THAT(fnmatch(std::string(glob_pattern).c_str(), std::string(text).c_str(), 0) == 0, expected);
#endif  // TEST_FNMATCH
  }

  static absl::StatusOr<std::filesystem::path> GetTempDir(std::string_view sub_dir) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* const root = std::getenv("TEST_TMPDIR");
    if (root == nullptr) {
      return absl::NotFoundError("Environment variable TEST_TMPDIR not found.");
    }
    if (root[0] == '\0') {  // NOLINT(*-pointer-arithmetic)
      return absl::InvalidArgumentError("Environment variable TEST_TMPDIR is empty.");
    }
    return std::filesystem::path(root) / sub_dir;
  }
};

TEST_F(GlobTest, Glob2Re2Pattern) {
  EXPECT_THAT(Glob2Re2Expression(""), IsOkAndHolds(""));
  EXPECT_THAT(Glob2Re2Expression("/"), IsOkAndHolds("/"));
  EXPECT_THAT(Glob2Re2Expression("//"), IsOkAndHolds("/"));
  EXPECT_THAT(Glob2Re2Expression("/\\//\\/\\/"), IsOkAndHolds("/"));
  EXPECT_THAT(Glob2Re2Expression("*"), IsOkAndHolds("[^/]*"));
  EXPECT_THAT(Glob2Re2Expression("**"), IsOkAndHolds(".*"));
  EXPECT_THAT(Glob2Re2Expression("***"), IsOkAndHolds(".*"));
  EXPECT_THAT(Glob2Re2Expression("****"), IsOkAndHolds(".*"));
  EXPECT_THAT(Glob2Re2Expression("*****"), IsOkAndHolds(".*"));
  EXPECT_THAT(Glob2Re2Expression("**/**"), IsOkAndHolds("(.+/)+.*"));
  EXPECT_THAT(Glob2Re2Expression("**/**/"), IsOkAndHolds("(.+/)+.*"));
  EXPECT_THAT(Glob2Re2Expression("/**/**"), IsOkAndHolds("(/.+)?(/.+)?"));
  EXPECT_THAT(Glob2Re2Expression("/**/**/"), IsOkAndHolds("(/.+)?(/.+)?"));
  EXPECT_THAT(Glob2Re2Expression("*/*"), IsOkAndHolds("[^/]*/[^/]*"));
  EXPECT_THAT(Glob2Re2Expression("**", {.allow_star_star = false}), IsOkAndHolds("[^/]*"));
  EXPECT_THAT(Glob2Re2Expression("***", {.allow_star_star = false}), IsOkAndHolds("[^/]*"));
  EXPECT_THAT(Glob2Re2Expression("?"), IsOkAndHolds("[^/]"));
  EXPECT_THAT(Glob2Re2Expression("??"), IsOkAndHolds("[^/][^/]"));
  EXPECT_THAT(Glob2Re2Expression("."), IsOkAndHolds("\\."));
  EXPECT_THAT(Glob2Re2Expression(".."), IsOkAndHolds("\\.\\."));
  EXPECT_THAT(Glob2Re2Expression("+"), IsOkAndHolds("\\+"));
  EXPECT_THAT(Glob2Re2Expression("/"), IsOkAndHolds("/"));
  EXPECT_THAT(Glob2Re2Expression("\\\\"), IsOkAndHolds("\\\\"));
  EXPECT_THAT(Glob2Re2Expression("a\\\\b"), IsOkAndHolds("a\\\\b"));
  EXPECT_THAT(Glob2Re2Expression("*/*****?/"), IsOkAndHolds("[^/]*/.*[^/]"));
  EXPECT_THAT(Glob2Re2Expression("/*/*****?/"), IsOkAndHolds("/[^/]*/.*[^/]"));
  EXPECT_THAT(Glob2Re2Expression("**/abc/**"), IsOkAndHolds("(.+/)?abc(/.+)?"));
  EXPECT_THAT(Glob2Re2Expression("abc/**"), IsOkAndHolds("abc(/.+)?"));
  EXPECT_THAT(Glob2Re2Expression("**/abc"), IsOkAndHolds("(.+/)?abc"));
  EXPECT_THAT(Glob2Re2Expression("[]]"), IsOkAndHolds("[\\]]"));
  EXPECT_THAT(Glob2Re2Expression("[!]]"), IsOkAndHolds("[^\\]]"));
  EXPECT_THAT(Glob2Re2Expression("[:]"), IsOkAndHolds("[:]"));
  EXPECT_THAT(Glob2Re2Expression("[:]"), IsOkAndHolds("[:]"));
  EXPECT_THAT(Glob2Re2Expression("[[:alnum:]]"), IsOkAndHolds("[[:alnum:]]"));
  EXPECT_THAT(Glob2Re2Expression("[[:alpha:][:digit:]]"), IsOkAndHolds("[[:alpha:][:digit:]]"));
  EXPECT_THAT(Glob2Re2Expression("/**file/**/.*[.](cc|h)"), IsOkAndHolds("/.*file(/.+)?/\\.[^/]*[.]\\(cc\\|h\\)"));
  EXPECT_THAT(Glob2Re2Expression("ftp://foo"), IsOkAndHolds("ftp://foo"));
  EXPECT_THAT(Glob2Re2Expression("ftp\\://foo"), IsOkAndHolds("ftp\\:/foo"));
  EXPECT_THAT(Glob2Re2Expression("foo/bar"), IsOkAndHolds("foo/bar"));
  EXPECT_THAT(Glob2Re2Expression("foo//bar"), IsOkAndHolds("foo/bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\/bar"), IsOkAndHolds("foo/bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\//bar"), IsOkAndHolds("foo/bar"));
  EXPECT_THAT(Glob2Re2Expression("foo/\\//bar"), IsOkAndHolds("foo/bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\*bar"), IsOkAndHolds("foo\\*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\**bar"), IsOkAndHolds("foo\\*[^/]*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\***bar"), IsOkAndHolds("foo\\*.*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\****bar"), IsOkAndHolds("foo\\*.*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\*bar", {.allow_star_star = false}), IsOkAndHolds("foo\\*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\**bar", {.allow_star_star = false}), IsOkAndHolds("foo\\*[^/]*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\***bar", {.allow_star_star = false}), IsOkAndHolds("foo\\*[^/]*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\****bar", {.allow_star_star = false}), IsOkAndHolds("foo\\*[^/]*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\\\*bar"), IsOkAndHolds("foo\\\\[^/]*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\\\**bar"), IsOkAndHolds("foo\\\\.*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\\\***bar"), IsOkAndHolds("foo\\\\.*bar"));
  EXPECT_THAT(Glob2Re2Expression("foo\\\\****bar"), IsOkAndHolds("foo\\\\.*bar"));
}

TEST_F(GlobTest, Glob2Re2PatternErrors) {
  const auto no_char_to_escape =
      StatusIs(absl::StatusCode::kInvalidArgument, "No character left to escape at end of pattern.");
  EXPECT_THAT(Glob2Re2Expression("\\"), no_char_to_escape);
  const auto unterminated_range = StatusIs(absl::StatusCode::kInvalidArgument, "Unterminated range expression.");
  EXPECT_THAT(Glob2Re2Expression("[]"), unterminated_range);
  EXPECT_THAT(Glob2Re2Expression("[!]"), unterminated_range);
  const auto unterminated_char_class = StatusIs(absl::StatusCode::kInvalidArgument, "Unterminated character-class.");
  EXPECT_THAT(Glob2Re2Expression("[[:]"), unterminated_char_class);
  EXPECT_THAT(Glob2Re2Expression("[[:]]"), unterminated_char_class);
  const auto empty_character_class =
      StatusIs(absl::StatusCode::kInvalidArgument, "Invalid empty character-class name.");
  EXPECT_THAT(Glob2Re2Expression("[[::]]"), empty_character_class);
  EXPECT_THAT(Glob2Re2Expression("[[::]][[:alpha:]]"), empty_character_class);
  EXPECT_THAT(
      Glob2Re2Expression("[[:::]]"), StatusIs(absl::StatusCode::kInvalidArgument, "Invalid character-class name ':'."));
  const Glob2Re2Options disable_ranges{.allow_ranges = false};
  constexpr std::array<std::string_view, 6> kRangeIssues{
      "[]", "[!]", "[[:]", "[[:]]", "[[::]]", "[[::]][[:alpha:]]",
  };
  for (const std::string_view issue : kRangeIssues) {
    EXPECT_THAT(Glob2Re2Expression(issue, disable_ranges), IsOk());
  }
}

TEST_F(GlobTest, Glob2Re2) {
  Glob2Re2Match("[]]", "]");
  Glob2Re2Match("[]]", "x", false);
  Glob2Re2Match("[]]", "", false);
  Glob2Re2Match("[!]]", "]", false);
  Glob2Re2Match("[!]]", "", false);
  Glob2Re2Match("[!]]", "x");
  Glob2Re2Match("[!]]", "]", false);
  Glob2Re2Match("[!]]", "x]", false);
  Glob2Re2Match("[!]]", "!]", false);
  Glob2Re2Match("[!]]]", "x]");
  Glob2Re2Match("[[:alpha:][:digit:]]", "", false);
  Glob2Re2Match("[[:alpha:][:digit:]]", "a");
  Glob2Re2Match("[[:alpha:][:digit:]]", "0");
  Glob2Re2Match("[[:alpha:][:digit:]]", "!", false);
  Glob2Re2Match("[[:alpha:]![:digit:]]", "!");
}

MATCHER_P3(HasParts, path_matcher, file_matcher, is_mixed, "") {
  using ::testing::AllOf;
  using ::testing::Field;
  return ::testing::ExplainMatchResult(
      AllOf(
          Field("path_pattern", &GlobParts::path_pattern, path_matcher),
          Field("file_pattern", &GlobParts::file_pattern, file_matcher), Field("mixed", &GlobParts::mixed, is_mixed)),
      arg, result_listener);
}

TEST_F(GlobTest, GlobSplitParts) {
  EXPECT_THAT(
      GlobSplitParts("\\"),
      StatusIs(absl::StatusCode::kInvalidArgument, "No character left to escape at end of pattern."));
  EXPECT_THAT(GlobSplitParts(""), IsOkAndHolds(HasParts("", "", false)));
  EXPECT_THAT(GlobSplitParts("/"), IsOkAndHolds(HasParts("/", "", false)));
  EXPECT_THAT(GlobSplitParts("//"), IsOkAndHolds(HasParts("/", "", false)));
  EXPECT_THAT(GlobSplitParts("a/b"), IsOkAndHolds(HasParts("a", "b", false)));
  EXPECT_THAT(GlobSplitParts("a//b"), IsOkAndHolds(HasParts("a", "b", false)));
  EXPECT_THAT(GlobSplitParts("a/b/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a/**/c"), IsOkAndHolds(HasParts("a/**", "c", false)));
  EXPECT_THAT(GlobSplitParts("a/b/**"), IsOkAndHolds(HasParts("a/b/**", "", false)));
}

TEST_F(GlobTest, GlobSplitPartsWithRanges) {
  EXPECT_THAT(GlobSplitParts("a[/]b/[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b/[-/]c"), IsOkAndHolds(HasParts("a/b/[-/]c", "", true)));
  EXPECT_THAT(GlobSplitParts("a[/]b/[!/]c"), IsOkAndHolds(HasParts("a/b", "[!/]c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b/[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b/[-/]/c"), IsOkAndHolds(HasParts("a/b/[-/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b/[!/]/c"), IsOkAndHolds(HasParts("a/b/[!/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[-/]c"), IsOkAndHolds(HasParts("a/b/[-/]c", "", true)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[!/]c"), IsOkAndHolds(HasParts("a/b", "[!/]c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[-/]/c"), IsOkAndHolds(HasParts("a/b/[-/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b/[!/]/c"), IsOkAndHolds(HasParts("a/b/[!/]", "c", false)));

  EXPECT_THAT(GlobSplitParts("a[/]b[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b[-/]c"), IsOkAndHolds(HasParts("a/b[-/]c", "", true)));
  EXPECT_THAT(GlobSplitParts("a[/]b[!/]c"), IsOkAndHolds(HasParts("a", "b[!/]c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b[-/]/c"), IsOkAndHolds(HasParts("a/b[-/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]b[!/]/c"), IsOkAndHolds(HasParts("a/b[!/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[-/]c"), IsOkAndHolds(HasParts("a/b[-/]c", "", true)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[!/]c"), IsOkAndHolds(HasParts("a", "b[!/]c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[-/]/c"), IsOkAndHolds(HasParts("a/b[-/]", "c", false)));
  EXPECT_THAT(GlobSplitParts("a[/]/b[!/]/c"), IsOkAndHolds(HasParts("a/b[!/]", "c", false)));

  EXPECT_THAT(GlobSplitParts("a/b[-1]c"), IsOkAndHolds(HasParts("a", "b[-1]c", false)));
  EXPECT_THAT(GlobSplitParts("a/b[.-]c"), IsOkAndHolds(HasParts("a", "b[.-]c", false)));
  EXPECT_THAT(GlobSplitParts("a/b[.-1]c"), IsOkAndHolds(HasParts("a/b[.-1]c", "", true)));
  EXPECT_THAT(GlobSplitParts("a/b[0-1]c"), IsOkAndHolds(HasParts("a", "b[0-1]c", false)));
}

MATCHER_P2(HasSplit, root_matcher, pattern_matcher, "") {
  using ::testing::AllOf;
  using ::testing::Field;
  return ::testing::ExplainMatchResult(
      AllOf(
          Field("root", &RootAndPattern::root, root_matcher),
          Field("pattern", &RootAndPattern::pattern, pattern_matcher)),
      arg, result_listener);
}

TEST_F(GlobTest, GlobSplit) {
  EXPECT_THAT(GlobSplit(""), IsOkAndHolds(HasSplit("", "")));
  EXPECT_THAT(GlobSplit("a/?/c"), IsOkAndHolds(HasSplit("a", "?/c")));
  EXPECT_THAT(GlobSplit("a/b/?/c"), IsOkAndHolds(HasSplit("a/b", "?/c")));
  EXPECT_THAT(GlobSplit("a/*/c"), IsOkAndHolds(HasSplit("a", "*/c")));
  EXPECT_THAT(GlobSplit("a/b/*/c"), IsOkAndHolds(HasSplit("a/b", "*/c")));
  EXPECT_THAT(GlobSplit("a/**/c"), IsOkAndHolds(HasSplit("a", "**/c")));
  EXPECT_THAT(GlobSplit("a/b/**/c"), IsOkAndHolds(HasSplit("a/b", "**/c")));
  EXPECT_THAT(GlobSplit("/"), IsOkAndHolds(HasSplit("/", "")));
  EXPECT_THAT(GlobSplit("/a/**/c"), IsOkAndHolds(HasSplit("/a", "**/c")));
  EXPECT_THAT(GlobSplit("/a/b/**/c"), IsOkAndHolds(HasSplit("/a/b", "**/c")));
  EXPECT_THAT(GlobSplit("//"), IsOkAndHolds(HasSplit("/", "")));
  EXPECT_THAT(GlobSplit("//a//**//c"), IsOkAndHolds(HasSplit("/a", "**/c")));
  EXPECT_THAT(GlobSplit("//a//b/**//c"), IsOkAndHolds(HasSplit("/a/b", "**/c")));
  EXPECT_THAT(GlobSplit("a/[/]/c"), IsOkAndHolds(HasSplit("a/c", "")));
  EXPECT_THAT(GlobSplit("a/b/[/]/c"), IsOkAndHolds(HasSplit("a/b/c", "")));
  EXPECT_THAT(GlobSplit("a/[xy]/c"), IsOkAndHolds(HasSplit("a", "[xy]/c")));
  EXPECT_THAT(GlobSplit("a/b/[xy]/c"), IsOkAndHolds(HasSplit("a/b", "[xy]/c")));
  EXPECT_THAT(GlobSplit("a/x?y/c"), IsOkAndHolds(HasSplit("a", "x?y/c")));
  EXPECT_THAT(GlobSplit("a/b/x?y/c"), IsOkAndHolds(HasSplit("a/b", "x?y/c")));
  EXPECT_THAT(GlobSplit("a/x*y/c"), IsOkAndHolds(HasSplit("a", "x*y/c")));
  EXPECT_THAT(GlobSplit("a/b/x*y/c"), IsOkAndHolds(HasSplit("a/b", "x*y/c")));
}

template<typename C = std::initializer_list<std::string_view>>
requires(std::same_as<std::remove_cvref_t<typename std::remove_cvref_t<C>::value_type>, std::string_view>)
absl::StatusOr<std::filesystem::path> CreateFileSystemEntries(
    const absl::StatusOr<std::filesystem::path>& root,
    const C& entries) {
  namespace fs = std::filesystem;
  MBO_RETURN_IF_ERROR(root);
  if (!fs::create_directories(*root)) {
    return absl::AbortedError(absl::StrCat("Cannot create test root dir: ", root->native()));
  }
  std::set<std::string> created;
  for (const std::string_view& entry : entries) {
    const auto [path, file] = [&]() {
      std::vector<std::string> split = absl::StrSplit(entry, absl::MaxSplits(':', 2));
      while (split.size() < 2) {
        split.emplace_back("");
      }
      return std::make_pair(split[0], split[1]);
    }();
    if (!path.empty() && created.emplace(path).second && !fs::create_directories(*root / path)) {
      return absl::AbortedError(absl::StrCat("Cannot create dir: ", path));
    }
    if (!file.empty()) {
      std::ofstream output(*root / path / file, std::ios::binary);
    }
  }
  return *root;
}

struct GlobFileTest : GlobTest {
  static void SetUpTestSuite() {
    MBO_ASSERT_OK_AND_ASSIGN(
        root_glob_test,  // NL
        CreateFileSystemEntries(
            GetTempDir("glob_test"),  // NL
            {
                ":top",
                "dir",
                "sub/dir:file1",
                "sub/dir:file2",
                "sub/two/dir:file1",
                "sub/two/dir:file2",
                "sub/two/dir:file3",
            }));
  }

  static std::filesystem::path root_glob_test;
};

std::filesystem::path GlobFileTest::root_glob_test;

TEST_F(GlobFileTest, GlobStopImmediately) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(root_glob_test, "*", {}, {}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kStop;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "top",
             }));
}

TEST_F(GlobFileTest, GlobStar) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(root_glob_test, "*", {}, {}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kContinue;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "top",
                 "dir",
                 "sub",
             }));
}

TEST_F(GlobFileTest, GlobStarStar) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(root_glob_test, "**", {}, {}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kContinue;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "top",
                 "dir",
                 "sub",
                 "sub/dir",
                 "sub/dir/file1",
                 "sub/dir/file2",
                 "sub/two",
                 "sub/two/dir",
                 "sub/two/dir/file1",
                 "sub/two/dir/file2",
                 "sub/two/dir/file3",
             }));
}

TEST_F(GlobFileTest, GlobStarStarNonRecursive) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(root_glob_test, "**/???", {}, {.recursive = false}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kContinue;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "top",
                 "dir",
                 "sub",
             }));
}

TEST_F(GlobFileTest, GlobStarStarMatch) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(root_glob_test, "**/dir", {}, {}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kContinue;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "dir",
                 "sub/dir",
                 "sub/two/dir",
             }));
}

TEST_F(GlobFileTest, GlobStarStarMatchSquareBrackets) {
  std::vector<std::string> found;
  ASSERT_OK(Glob(GlobSplit(root_glob_test / "**/file[23]"), {}, {}, [&](const GlobEntry& entry) -> GlobEntryAction {
    found.push_back(entry.entry.path().lexically_relative(root_glob_test));
    return GlobEntryAction::kContinue;
  }));
  EXPECT_THAT(
      found, UnorderedElementsAreArray({
                 "sub/dir/file2",
                 "sub/two/dir/file2",
                 "sub/two/dir/file3",
             }));
}

}  // namespace
}  // namespace mbo::file
