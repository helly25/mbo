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

// If the macro `TEST_FNMATCH` is defined, and the <fnmatch.h> include is available, then the test
// for `Glob2Re2` verifies that its result matches that of `fnmatch`.
#define TEST_FNMATCH

#if !__has_include(<fnmatch.h>)
# undef TEST_FNMATCH
#endif

#include "mbo/file/glob.h"

#ifdef TEST_FNMATCH
# include <fnmatch.h>
#endif  // TEST_FNMATCH
#include <array>
#include <memory>
#include <source_location>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/status.h"
#include "re2/re2.h"

namespace mbo::file {
namespace {

using namespace mbo::file::file_internal;

using ::mbo::testing::IsOk;
using ::mbo::testing::IsOkAndHolds;
using ::mbo::testing::StatusIs;
using ::testing::NotNull;

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

TEST_F(GlobTest, GlobSplit) {
  EXPECT_THAT(
      GlobSplit("\\"), StatusIs(absl::StatusCode::kInvalidArgument, "No character left to escape at end of pattern."));
  EXPECT_THAT(GlobSplit(""), IsOkAndHolds(HasParts("", "", false)));
  EXPECT_THAT(GlobSplit("/"), IsOkAndHolds(HasParts("/", "", false)));
  EXPECT_THAT(GlobSplit("//"), IsOkAndHolds(HasParts("/", "", false)));
  EXPECT_THAT(GlobSplit("a/b"), IsOkAndHolds(HasParts("a", "b", false)));
  EXPECT_THAT(GlobSplit("a//b"), IsOkAndHolds(HasParts("a", "b", false)));
  EXPECT_THAT(GlobSplit("a/b/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a/**/c"), IsOkAndHolds(HasParts("a/**", "c", false)));
  EXPECT_THAT(GlobSplit("a/b/**"), IsOkAndHolds(HasParts("a/b/**", "", false)));
}

TEST_F(GlobTest, GlobSplitWithRanges) {
  EXPECT_THAT(GlobSplit("a[/]b/[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b/[-/]c"), IsOkAndHolds(HasParts("a/b/[-/]c", "", true)));
  EXPECT_THAT(GlobSplit("a[/]b/[!/]c"), IsOkAndHolds(HasParts("a/b", "[!/]c", false)));
  EXPECT_THAT(GlobSplit("a[/]b/[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b/[-/]/c"), IsOkAndHolds(HasParts("a/b/[-/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b/[!/]/c"), IsOkAndHolds(HasParts("a/b/[!/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b/[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b/[-/]c"), IsOkAndHolds(HasParts("a/b/[-/]c", "", true)));
  EXPECT_THAT(GlobSplit("a[/]/b/[!/]c"), IsOkAndHolds(HasParts("a/b", "[!/]c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b/[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b/[-/]/c"), IsOkAndHolds(HasParts("a/b/[-/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b/[!/]/c"), IsOkAndHolds(HasParts("a/b/[!/]", "c", false)));

  EXPECT_THAT(GlobSplit("a[/]b[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b[-/]c"), IsOkAndHolds(HasParts("a/b[-/]c", "", true)));
  EXPECT_THAT(GlobSplit("a[/]b[!/]c"), IsOkAndHolds(HasParts("a", "b[!/]c", false)));
  EXPECT_THAT(GlobSplit("a[/]b[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b[-/]/c"), IsOkAndHolds(HasParts("a/b[-/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]b[!/]/c"), IsOkAndHolds(HasParts("a/b[!/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b[/]c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b[-/]c"), IsOkAndHolds(HasParts("a/b[-/]c", "", true)));
  EXPECT_THAT(GlobSplit("a[/]/b[!/]c"), IsOkAndHolds(HasParts("a", "b[!/]c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b[/]/c"), IsOkAndHolds(HasParts("a/b", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b[-/]/c"), IsOkAndHolds(HasParts("a/b[-/]", "c", false)));
  EXPECT_THAT(GlobSplit("a[/]/b[!/]/c"), IsOkAndHolds(HasParts("a/b[!/]", "c", false)));
}

}  // namespace
}  // namespace mbo::file
