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
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/container/convert_container.h"
#include "mbo/file/artefact.h"
#include "mbo/status/status_macros.h"
#include "mbo/strings/indent.h"
#include "mbo/testing/status.h"

namespace mbo::diff {
namespace {

using ::mbo::strings::DropIndent;
using ::mbo::strings::DropIndentAndSplit;
using ::mbo::testing::IsOkAndHolds;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

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
      Diff({ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"}, {.context_size = 0}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(R"txt(
    --- lhs 1970-01-01 00:00:00.000 +0000
    +++ rhs 1970-01-01 00:00:00.000 +0000
    @@ -3,0 +3 @@
    +e
    @@ -5 +6,0 @@
    -e
    @@ -7 +7,0 @@
    -c
    @@ -9,0 +8,3 @@
    +b
    +a
    +b
  )txt"))));
  // Context_size = 1 is the mosty the same as context_size = 2:
  // Skip the first/last line, rest of the diff is the same.
  EXPECT_THAT(
      Diff({ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"}, {.context_size = 1}),
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
      Diff({ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"}),
      IsOkAndHolds(ElementsAreArray(DropIndentAndSplit(kOneChunk))));
  for (const std::size_t context_size : {2, 3, 5, 50}) {
    EXPECT_THAT(
        Diff({ToLines("acbdeacbed"), "lhs"}, {ToLines("acebdabbabed"), "rhs"}, {.context_size = context_size}),
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
    @@ -10,2 +10,0 @@
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
