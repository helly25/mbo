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

#include "mbo/diff/diff.h"

#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/container/convert_container.h"
#include "mbo/file/artefact.h"
#include "mbo/status/status_macros.h"
#include "mbo/strings/indent.h"
#include "mbo/testing/status.h"
#include "re2/re2.h"

namespace mbo::diff {
namespace {

using ::mbo::strings::DropIndent;
using ::mbo::strings::DropIndentAndSplit;
using ::mbo::testing::IsOk;
using ::mbo::testing::IsOkAndHolds;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Not;

class DiffTest : public ::testing::Test {
 public:
  // IMPORTANT: Uses a global cache, so cannot be used simultaneously.
  static absl::StatusOr<std::vector<std::string>> Diff(
      file::Artefact lhs,
      file::Artefact rhs,
      const Diff::Options& options = Diff::Options::Default()) {
    lhs.data = DropIndent(lhs.data);
    rhs.data = DropIndent(rhs.data);
    MBO_ASSIGN_OR_RETURN(std::string result, mbo::diff::Diff::FileDiff(lhs, rhs, options));
    if (result.empty()) {
      return std::vector<std::string>{};
    }
    return std::vector<std::string>(mbo::container::ConvertContainer(DropIndentAndSplit(result)));
  }

  static std::string ToLines(std::string_view input) {
    std::string result;
    for (const char chr : input) {
      result.append(1, chr);
      result.append(1, '\n');
    }
    return result;
  }

  // Applies a unified diff produced with `context_size = 0` and no file
  // headers back onto `lhs_text`. Returns std::nullopt on unparsable input.
  static std::optional<std::string> ApplyUnifiedDiff(std::string_view lhs_text, std::string_view diff) {
    if (diff.empty()) {
      return std::string(lhs_text);
    }
    std::vector<std::string_view> lhs_lines = absl::StrSplit(lhs_text, '\n');
    if (!lhs_lines.empty() && lhs_lines.back().empty()) {
      lhs_lines.pop_back();
    }
    std::vector<std::string_view> result;
    std::size_t next = 0;
    for (const std::string_view line : absl::StrSplit(diff, '\n')) {
      if (line.empty()) {
        continue;  // Trailing newline artifact.
      }
      if (line[0] == '@') {
        std::string_view range = line;
        if (!absl::ConsumePrefix(&range, "@@ -")) {
          return std::nullopt;
        }
        range = range.substr(0, range.find(' '));
        const std::size_t comma = range.find(',');
        std::size_t start = 0;
        std::size_t size = 1;
        if (comma == std::string_view::npos) {
          if (!absl::SimpleAtoi(range, &start)) {
            return std::nullopt;
          }
        } else if (
            !absl::SimpleAtoi(range.substr(0, comma), &start) || !absl::SimpleAtoi(range.substr(comma + 1), &size)) {
          return std::nullopt;
        }
        if (size != 0 && start == 0) {
          return std::nullopt;
        }
        // An empty range names the line before the gap, otherwise the first
        // line of the hunk.
        const std::size_t copy_until = size == 0 ? start : start - 1;
        if (copy_until < next || copy_until + size > lhs_lines.size()) {
          return std::nullopt;
        }
        while (next < copy_until) {
          result.push_back(lhs_lines[next++]);
        }
        next += size;
      } else if (line[0] == '+') {
        result.push_back(line.substr(1));
      } else if (line[0] != '-') {
        return std::nullopt;  // Context lines are unexpected with context 0.
      }
    }
    while (next < lhs_lines.size()) {
      result.push_back(lhs_lines[next++]);
    }
    if (result.empty()) {
      return std::string();
    }
    return absl::StrCat(absl::StrJoin(result, "\n"), "\n");
  }
};

TEST_F(DiffTest, Empty) {
  EXPECT_THAT(Diff({}, {}), IsOkAndHolds(IsEmpty()));
  EXPECT_THAT(Diff({"\n", "lhs"}, {"\n", "rhs"}), IsOkAndHolds(IsEmpty()));
}

TEST_F(DiffTest, Equal) {
  for (const std::string txt : {"a", "a\nb", "a\nb\n"}) {
    EXPECT_THAT(Diff({txt, "lhs"}, {txt, "rhs"}), IsOkAndHolds(IsEmpty()));
  }
}

TEST_F(DiffTest, OnlyLhs) {
  const std::string txt = R"txt(
    l
  )txt";
  EXPECT_THAT(Diff({txt, "lhs"}, {"\n", "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    +
  )txt"))));
  EXPECT_THAT(Diff({txt, "lhs"}, {"", "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +0,0 @@
    -l
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("alb"), "lhs"}, {ToLines("ab"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,2 @@
     a
    -l
     b
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("1234_L_5678"), "lhs"}, {ToLines("12345678"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,9 +2,6 @@
     2
     3
     4
    -_
    -L
    -_
     5
     6
     7
  )txt"))));
}

TEST_F(DiffTest, OnlyRhs) {
  const std::string txt = R"txt(
    r
  )txt";
  EXPECT_THAT(Diff({"\n", "lhs"}, {txt, "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -
    +r
  )txt"))));
  EXPECT_THAT(Diff({"", "lhs"}, {txt, "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -0,0 +1 @@
    +r
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("ab"), "lhs"}, {ToLines("arb"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,2 +1,3 @@
     a
    +r
     b
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("12345678"), "lhs"}, {ToLines("1234_R_5678"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,6 +2,9 @@
     2
     3
     4
    +_
    +R
    +_
     5
     6
     7
  )txt"))));
}

TEST_F(DiffTest, NoNewLine) {
  EXPECT_THAT(Diff({ToLines("l"), "lhs"}, {"r", "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    +r
    \ No newline at end of file
  )txt"))));
  EXPECT_THAT(Diff({"l", "lhs"}, {ToLines("r"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    \ No newline at end of file
    +r
  )txt"))));
  EXPECT_THAT(Diff({"l", "lhs"}, {"r", "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    \ No newline at end of file
    +r
    \ No newline at end of file
  )txt"))));
}

TEST_F(DiffTest, CompletelyDifferent) {
  EXPECT_THAT(
      Diff({ToLines("l"), "lhs"}, {ToLines("r"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    +r
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("l1"), "lhs"}, {ToLines("r2"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,2 +1,2 @@
    -l
    -1
    +r
    +2
  )txt"))));
}

TEST_F(DiffTest, Diff) {
  EXPECT_THAT(
      Diff({ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,3 @@
     a
    -1
    +2
     b
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("a12b"), "lhs"}, {ToLines("a3b"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,4 +1,3 @@
     a
    -1
    -2
    +3
     b
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("a1b"), "lhs"}, {ToLines("a23b"), "rhs"}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,4 @@
     a
    -1
    +2
    +3
     b
  )txt"))));
}

TEST_F(DiffTest, Multi1) {
  EXPECT_THAT(
      Diff(
          {ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"},
          {.algorithm = Diff::Options::Algorithm::kNaive, .context_size = 0}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,0 +3 @@
    +e
    @@ -5 +5,0 @@
    -e
    @@ -7 +6,0 @@
    -c
    @@ -8,0 +8,3 @@
    +b
    +a
    +b
  )txt"))));
  // Context_size = 1 is the mosty the same as context_size = 2:
  // Skip the first/last line, rest of the diff is the same.
  EXPECT_THAT(
      Diff(
          {ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"},
          {.algorithm = Diff::Options::Algorithm::kNaive, .context_size = 1}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,8 +2,10 @@
     c
    +e
     b
     d
    -e
     a
    -c
     b
    +b
    +a
    +b
     e
  )txt"))));
  constexpr std::string_view kOneChunk = R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,10 +1,12 @@
     a
     c
    +e
     b
     d
    -e
     a
    -c
     b
    +b
    +a
    +b
     e
     d
  )txt";
  EXPECT_THAT(
      Diff(
          {ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"},
          {.algorithm = Diff::Options::Algorithm::kNaive}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(kOneChunk))));
  for (const std::size_t context_size : {2, 3, 5, 50}) {
    EXPECT_THAT(
        Diff(
            {ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"},
            {.algorithm = Diff::Options::Algorithm::kNaive, .context_size = context_size}),
        IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(kOneChunk))));
  }
}

TEST_F(DiffTest, Multi2) {
  EXPECT_THAT(
      Diff({ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,11 +2,9 @@
     2
     3
     4
    -5
    -6
    +a
    +b
     7
     8
     9
    -a
    -c
     0
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"}, {.context_size = 2}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -3,10 +3,8 @@
     3
     4
    -5
    -6
    +a
    +b
     7
     8
     9
    -a
    -c
     0
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"}, {.context_size = 1}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -4,4 +4,4 @@
     4
    -5
    -6
    +a
    +b
     7
    @@ -9,4 +9,2 @@
     9
    -a
    -c
     0
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"}, {.context_size = 0}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -5,2 +5,2 @@
    -5
    -6
    +a
    +b
    @@ -10,2 +9,0 @@
    -a
    -c
  )txt"))));
}

TEST_F(DiffTest, Multi3) {
  EXPECT_THAT(
      Diff({ToLines("123456789XYZac0"), "lhs"}, {ToLines("1234ab789XYZ0"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,14 +2,12 @@
     2
     3
     4
    -5
    -6
    +a
    +b
     7
     8
     9
     X
     Y
     Z
    -a
    -c
     0
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("123456789_XYZac0"), "lhs"}, {ToLines("1234ab789_XYZ0"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -2,8 +2,8 @@
     2
     3
     4
    -5
    -6
    +a
    +b
     7
     8
     9
    @@ -11,6 +11,4 @@
     X
     Y
     Z
    -a
    -c
     0
  )txt"))));
  EXPECT_THAT(
      Diff({ToLines("123456789_XYZac0"), "lhs"}, {ToLines("1234ab789_XYZ0"), "rhs"}, {.context_size = 4}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,16 +1,14 @@
     1
     2
     3
     4
    -5
    -6
    +a
    +b
     7
     8
     9
     _
     X
     Y
     Z
    -a
    -c
     0
  )txt"))));
}

TEST_F(DiffTest, ContextFormat) {
  const Diff::Options options{.output_format = Diff::Options::OutputFormat::kContext};
  // Deletions paired with insertions show as '!' change blocks on both sides.
  EXPECT_THAT(
      Diff({ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"}, options),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 1,3 ****
      a
    ! 1
      b
    --- 1,3 ----
      a
    ! 2
      b
  )txt"))));
  // A side without changes omits its body (here the unchanged lhs).
  EXPECT_THAT(
      Diff({ToLines("ab"), "lhs"}, {ToLines("arb"), "rhs"}, options),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 1,2 ****
    --- 1,3 ----
      a
    + r
      b
  )txt"))));
  // Pure deletions keep '-' and the unchanged rhs omits its body.
  EXPECT_THAT(
      Diff({ToLines("alb"), "lhs"}, {ToLines("ab"), "rhs"}, options),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 1,3 ****
      a
    - l
      b
    --- 1,2 ----
  )txt"))));
}

TEST_F(DiffTest, ContextFormatMultiChunk) {
  EXPECT_THAT(
      Diff(
          {ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"},
          {.output_format = Diff::Options::OutputFormat::kContext, .context_size = 1}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 4,7 ****
      4
    ! 5
    ! 6
      7
    --- 4,7 ----
      4
    ! a
    ! b
      7
    ***************
    *** 9,12 ****
      9
    - a
    - c
      0
    --- 9,10 ----
  )txt"))));
}

TEST_F(DiffTest, ContextFormatEmptyFile) {
  const Diff::Options options{.output_format = Diff::Options::OutputFormat::kContext};
  // An empty range shows the line before it, so an empty file shows as 0.
  EXPECT_THAT(
      Diff({ToLines("abc"), "lhs"}, {"", "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 1,3 ****
    - a
    - b
    - c
    --- 0 ----
  )txt"))));
  EXPECT_THAT(
      Diff({"", "lhs"}, {ToLines("abc"), "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 0 ****
    --- 1,3 ----
    + a
    + b
    + c
  )txt"))));
}

TEST_F(DiffTest, ContextFormatNoNewLine) {
  EXPECT_THAT(
      Diff({"l", "lhs"}, {"r", "rhs"}, {.output_format = Diff::Options::OutputFormat::kContext}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    *** lhs 1970-01-01 00:00:00.000 +0000
    --- rhs 1970-01-01 00:00:00.000 +0000
    ***************
    *** 1 ****
    ! l
    \ No newline at end of file
    --- 1 ----
    ! r
    \ No newline at end of file
  )txt"))));
}

TEST_F(DiffTest, NormalFormat) {
  // Normal format shows no file headers and no context lines. The default
  // `context_size` merely groups nearby changes into one internal chunk, the
  // rendered a/d/c commands are unaffected by it.
  for (const std::size_t context_size : {0, 3}) {
    const Diff::Options options{
        .output_format = Diff::Options::OutputFormat::kNormal,
        .context_size = context_size,
    };
    EXPECT_THAT(
        Diff({ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"}, options),
        IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
      2c2
      < 1
      ---
      > 2
    )txt"))))
        << "context_size: " << context_size;
    EXPECT_THAT(
        Diff({ToLines("ab"), "lhs"}, {ToLines("arb"), "rhs"}, options),
        IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
      1a2
      > r
    )txt"))))
        << "context_size: " << context_size;
    EXPECT_THAT(
        Diff({ToLines("alb"), "lhs"}, {ToLines("ab"), "rhs"}, options),
        IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
      2d1
      < l
    )txt"))))
        << "context_size: " << context_size;
    EXPECT_THAT(
        Diff({ToLines("123456789ac0"), "lhs"}, {ToLines("1234ab7890"), "rhs"}, options),
        IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
      5,6c5,6
      < 5
      < 6
      ---
      > a
      > b
      10,11d9
      < a
      < c
    )txt"))))
        << "context_size: " << context_size;
  }
}

TEST_F(DiffTest, NormalFormatEmptyFile) {
  const Diff::Options options{.output_format = Diff::Options::OutputFormat::kNormal};
  EXPECT_THAT(
      Diff({ToLines("abc"), "lhs"}, {"", "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    1,3d0
    < a
    < b
    < c
  )txt"))));
  EXPECT_THAT(
      Diff({"", "lhs"}, {ToLines("abc"), "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    0a1,3
    > a
    > b
    > c
  )txt"))));
}

TEST_F(DiffTest, NormalFormatNoNewLine) {
  EXPECT_THAT(
      Diff({"l", "lhs"}, {"r", "rhs"}, {.output_format = Diff::Options::OutputFormat::kNormal}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    1c1
    < l
    \ No newline at end of file
    ---
    > r
    \ No newline at end of file
  )txt"))));
}

TEST_F(DiffTest, ContextFormatDirectAlgorithm) {
  // The direct algorithm pushes interleaved deletion/insertion pairs. They
  // classify as adjacent change blocks and render as one '!' block per side.
  EXPECT_THAT(
      Diff(
          {ToLines("123"), "lhs"}, {ToLines("xy3"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kDirect,
              .output_format = Diff::Options::OutputFormat::kContext,
              .context_size = 0,
              .file_header_use = Diff::Options::FileHeaderUse::kNone,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    ***************
    *** 1,2 ****
    ! 1
    ! 2
    --- 1,2 ----
    ! x
    ! y
  )txt"))));
}

TEST_F(DiffTest, NormalFormatDirectAlgorithm) {
  // The direct algorithm flushes each line pair separately, so adjacent
  // changed lines become one command per line (unlike the unified algorithm
  // which groups them into a single `1,2c1,2`).
  EXPECT_THAT(
      Diff(
          {ToLines("123"), "lhs"}, {ToLines("xy3"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kDirect,
              .output_format = Diff::Options::OutputFormat::kNormal,
              .context_size = 0,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    1c1
    < 1
    ---
    > x
    2c2
    < 2
    ---
    > y
  )txt"))));
}

TEST_F(DiffTest, NormalFormatNoChunkHeaders) {
  EXPECT_THAT(
      Diff(
          {ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"},
          {
              .output_format = Diff::Options::OutputFormat::kNormal,
              .show_chunk_headers = false,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    < 1
    ---
    > 2
  )txt"))));
}

TEST_F(DiffTest, NormalFormatSkipLeftDeletions) {
  // Dropped deletions turn changes into pure insertions.
  EXPECT_THAT(
      Diff(
          {ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"},
          {
              .output_format = Diff::Options::OutputFormat::kNormal,
              .skip_left_deletions = true,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    1a2
    > 2
  )txt"))));
}

TEST_F(DiffTest, FormatsIgnoreBlankLines) {
  // Chunk suppression happens before rendering, so it applies to all formats.
  for (const Diff::Options::OutputFormat output_format : {
           Diff::Options::OutputFormat::kUnified,
           Diff::Options::OutputFormat::kContext,
           Diff::Options::OutputFormat::kNormal,
       }) {
    EXPECT_THAT(
        Diff(
            {"a\n\nb\n", "lhs"}, {"a\nb\n", "rhs"},
            {
                .output_format = output_format,
                .ignore_blank_lines = true,
            }),
        IsOkAndHolds(IsEmpty()))
        << "output_format: " << static_cast<int>(output_format);
  }
}

TEST_F(DiffTest, MyersAlgorithm) {
  const Diff::Options options{.algorithm = Diff::Options::Algorithm::kMyers};
  // On inputs where the naive algorithm is already minimal the output is
  // identical (see the mirrored expectations in `Diff`).
  EXPECT_THAT(
      Diff({ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"}, options),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,3 @@
     a
    -1
    +2
     b
  )txt"))));
  EXPECT_THAT(Diff({"l", "lhs"}, {"r", "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1 +1 @@
    -l
    \ No newline at end of file
    +r
    \ No newline at end of file
  )txt"))));
  EXPECT_THAT(
      Diff({"", "lhs"}, {ToLines("abc"), "rhs"}, options), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -0,0 +1,3 @@
    +a
    +b
    +c
  )txt"))));
}

TEST_F(DiffTest, MyersMinimal) {
  // The classic example from the Myers paper: the minimal script has 5 edits.
  // The naive algorithm needs 7 on the same input.
  EXPECT_THAT(
      Diff(
          {ToLines("ABCABBA"), "lhs"}, {ToLines("CBABAC"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .context_size = 0,
              .file_header_use = Diff::Options::FileHeaderUse::kNone,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    @@ -1 +1 @@
    -A
    +C
    @@ -3 +2,0 @@
    -C
    @@ -6 +4,0 @@
    -B
    @@ -7,0 +6 @@
    +C
  )txt"))));
}

TEST_F(DiffTest, MyersFormats) {
  EXPECT_THAT(
      Diff(
          {ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .output_format = Diff::Options::OutputFormat::kContext,
              .file_header_use = Diff::Options::FileHeaderUse::kNone,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    ***************
    *** 1,3 ****
      a
    ! 1
      b
    --- 1,3 ----
      a
    ! 2
      b
  )txt"))));
  EXPECT_THAT(
      Diff(
          {ToLines("a1b"), "lhs"}, {ToLines("a2b"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .output_format = Diff::Options::OutputFormat::kNormal,
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    2c2
    < 1
    ---
    > 2
  )txt"))));
}

TEST_F(DiffTest, MyersIgnoreOptions) {
  // Case folding and the ignore-matching token both make lines compare equal.
  EXPECT_THAT(
      Diff(
          {ToLines("aBc"), "lhs"}, {ToLines("AbC"), "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .ignore_case = true,
          }),
      IsOkAndHolds(IsEmpty()));
  EXPECT_THAT(
      Diff(
          {"a\nFOO 1\nb\n", "lhs"}, {"a\nFOO 2\nb\n", "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .ignore_matching_lines = std::make_optional<RE2>("FOO"),
          }),
      IsOkAndHolds(IsEmpty()));
}

TEST_F(DiffTest, MyersRoundTrip) {
  // Property test: applying the produced diff to the left input always
  // reconstructs the right input, including on inputs large and noisy enough
  // to trigger the cost cap fallback.
  const Diff::Options options{
      .algorithm = Diff::Options::Algorithm::kMyers,
      .context_size = 0,
      .file_header_use = Diff::Options::FileHeaderUse::kNone,
  };
  std::mt19937 rng(20'260'703);  // NOLINT(*-magic-numbers)
  const auto make_file = [&rng](std::size_t lines, std::size_t alphabet, std::string_view tag) {
    std::string text;
    for (std::size_t i = 0; i < lines; ++i) {
      absl::StrAppend(&text, tag, rng() % alphabet, "\n");
    }
    return text;
  };
  std::vector<std::pair<std::string, std::string>> cases;
  cases.emplace_back("", "");
  cases.emplace_back("", make_file(50, 5, "x"));
  cases.emplace_back(make_file(50, 5, "x"), "");
  cases.emplace_back(make_file(300, 1, "left"), make_file(300, 1, "right"));  // Disjoint: cap fallback.
  for (const std::size_t lines : {2U, 5U, 20U, 100U, 250U}) {
    for (const std::size_t alphabet : {2U, 5U, 40U}) {
      cases.emplace_back(make_file(lines, alphabet, "l"), make_file(lines + (rng() % 20), alphabet, "l"));
    }
  }
  for (std::size_t idx = 0; idx < cases.size(); ++idx) {
    const auto& [lhs, rhs] = cases[idx];
    const auto result = mbo::diff::Diff::FileDiff({lhs, "lhs"}, {rhs, "rhs"}, options);
    ASSERT_THAT(result, mbo::testing::IsOk()) << "case: " << idx;
    const std::optional<std::string> applied = ApplyUnifiedDiff(lhs, *result);
    ASSERT_TRUE(applied.has_value()) << "case: " << idx << " diff:\n" << *result;
    EXPECT_EQ(*applied, rhs) << "case: " << idx << " diff:\n" << *result;
  }
}

TEST_F(DiffTest, AlgorithmFeatureMatrix) {
  // Every comparison-affecting option must work identically for every
  // algorithm: inputs that differ only in the ignored aspect produce an empty
  // diff. The preprocessing (Data) and chunk filtering (Chunk) layers are
  // shared, the algorithms only consume the preprocessed comparison results.
  using Algorithm = Diff::Options::Algorithm;
  const auto expect_empty = [](std::string_view name, std::string_view lhs, std::string_view rhs, auto make_options) {
    for (const auto& [algo_name, algorithm] : {
             std::pair{"naive", Algorithm::kNaive},
             std::pair{"myers", Algorithm::kMyers},
             std::pair{"direct", Algorithm::kDirect},
         }) {
      const Diff::Options options = make_options(algorithm);
      EXPECT_THAT(Diff({std::string(lhs), "lhs"}, {std::string(rhs), "rhs"}, options), IsOkAndHolds(IsEmpty()))
          << "feature: " << name << " algorithm: " << algo_name;
    }
  };
  expect_empty("ignore_case", "aBc\n", "AbC\n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_case = true};
  });
  expect_empty("ignore_all_space", "a b\nc\n", "ab\n c \n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_all_space = true};
  });
  expect_empty("ignore_consecutive_space", "a  b\n", " a b \n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_consecutive_space = true};
  });
  expect_empty("ignore_trailing_space", "a\t \nb\n", "a\nb\n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_trailing_space = true};
  });
  // Trailing blank lines: a blank-only chunk for all algorithms (the direct
  // algorithm pairs lines positionally, so mid-file blank insertions pair
  // non-blank content and are reported by design).
  expect_empty("ignore_blank_lines", "a\nb\n", "a\nb\n\n\n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_blank_lines = true};
  });
  expect_empty("ignore_matching_lines", "a\nFOO 1\nb\n", "a\nFOO 2\nb\n", [](Algorithm algo) {
    return Diff::Options{.algorithm = algo, .ignore_matching_lines = std::make_optional<RE2>("FOO")};
  });
  expect_empty("strip_comments", "a # one\nb\n", "a # two\nb\n", [](Algorithm algo) {
    return Diff::Options{
        .algorithm = algo,
        .strip_comments = mbo::strings::StripCommentArgs{.comment_start = "#"},
    };
  });
  expect_empty("regex_replace", "a\nerror 1\nb\n", "a\nerror 2\nb\n", [](Algorithm algo) {
    return Diff::Options{
        .algorithm = algo,
        .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/error.*/X/"),
        .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag("/error.*/X/"),
    };
  });
  // Sanity: without the option every algorithm reports the difference.
  for (const auto algorithm : {Algorithm::kNaive, Algorithm::kMyers, Algorithm::kDirect}) {
    EXPECT_THAT(Diff({"aBc\n", "lhs"}, {"AbC\n", "rhs"}, {.algorithm = algorithm}), IsOkAndHolds(Not(IsEmpty())))
        << "algorithm: " << static_cast<int>(algorithm);
  }
}

TEST_F(DiffTest, IgnoreCaseWithIgnoreMatchingLines) {
  // Documented corner case: a line matching `ignore_matching_lines` and a
  // case-fold-equal line that does not match. The naive algorithm compares
  // the pair equal (case folding wins), the token based Myers algorithm does
  // not (matching lines form their own token). Write case-insensitive
  // expressions (`(?i)...`) when combining both options.
  const std::string lhs = "foo\n";  // Matches "[a-z]+" ...
  const std::string rhs = "FOO\n";  // ... does not match.
  EXPECT_THAT(
      Diff(
          {lhs, "lhs"}, {rhs, "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kNaive,
              .ignore_case = true,
              .ignore_matching_lines = std::make_optional<RE2>("[a-z]+"),
          }),
      IsOkAndHolds(IsEmpty()));
  EXPECT_THAT(
      Diff(
          {lhs, "lhs"}, {rhs, "rhs"},
          {
              .algorithm = Diff::Options::Algorithm::kMyers,
              .ignore_case = true,
              .ignore_matching_lines = std::make_optional<RE2>("[a-z]+"),
          }),
      IsOkAndHolds(Not(IsEmpty())));
  // With a case-insensitive expression both algorithms agree.
  for (const auto algorithm : {Diff::Options::Algorithm::kNaive, Diff::Options::Algorithm::kMyers}) {
    EXPECT_THAT(
        Diff(
            {lhs, "lhs"}, {rhs, "rhs"},
            {
                .algorithm = algorithm,
                .ignore_case = true,
                .ignore_matching_lines = std::make_optional<RE2>("(?i)[a-z]+"),
            }),
        IsOkAndHolds(IsEmpty()))
        << "algorithm: " << static_cast<int>(algorithm);
  }
}

TEST_F(DiffTest, RegexReplace) {
  const file::Artefact lhs{
      .data = R"txt(
    foo
    bar ERROR 1
    baz
    )txt",
      .name = "lhs",
  };
  const file::Artefact rhs{
      .data = R"txt(
    foo
    bar ERROR 2
    baz
    )txt",
      .name = "rhs",
  };
  const file::Artefact oth{
      .data = R"txt(
    foo
    oth ERROR 3
    baz
    )txt",
      .name = "rhs",
  };
  EXPECT_THAT(Diff(lhs, rhs, {}), IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,3 @@
     foo
    -bar ERROR 1
    +bar ERROR 2
     baz
  )txt"))));
  EXPECT_THAT(
      Diff(
          lhs, rhs,
          {
              .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/(.*)ERROR.*/SAME/"),
              .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag(",(.*)ERROR.*,SAME,"),
          }),
      IsOkAndHolds(IsEmpty()))
      << "The replacement made LHS and RHS the same, so there should be no difference.";
  EXPECT_THAT(
      Diff(
          lhs, rhs,
          {
              .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/(.*)ERROR.*/\\1 SAME/"),
              .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag(",(.*)ERROR.*,\\1 SAME,"),
          }),
      IsOkAndHolds(IsEmpty()))
      << "The replacement made LHS and RHS the same, so there should be no differences.";
  EXPECT_THAT(
      Diff(
          lhs, rhs,
          {
              .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/(.*)ERROR.*/\\1 LHS/"),
              .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag(",(.*)ERROR.*,\\1 RHS,"),
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,3 @@
     foo
    -bar ERROR 1
    +bar ERROR 2
     baz
  )txt"))))
      << "The replacement changes the difference, still different though.";
  EXPECT_THAT(
      Diff(
          lhs, oth,
          {
              .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/ERROR.*//"),
              .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag(",ERROR.*,,"),
          }),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -1,3 +1,3 @@
     foo
    -bar ERROR 1
    +oth ERROR 3
     baz
  )txt"))))
      << "The replacement equalizes the errors, still different though.";
  EXPECT_THAT(
      Diff(
          lhs, oth,
          {
              .regex_replace_lhs = Diff::Options::ParseRegexReplaceFlag("/bar/oth/"),
              .regex_replace_rhs = Diff::Options::ParseRegexReplaceFlag(",ERROR 3,ERROR 1,"),
          }),
      IsOkAndHolds(IsEmpty()))
      << "Different replacements that producr the same results lead to same zero differences.";
}

}  // namespace
}  // namespace mbo::diff
