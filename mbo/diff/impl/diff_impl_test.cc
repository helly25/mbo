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

#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/diff/diff_options.h"
#include "mbo/diff/impl/diff_direct.h"
#include "mbo/diff/impl/diff_myers.h"
#include "mbo/diff/impl/diff_naive.h"
#include "mbo/file/artefact.h"

// Tests the three diff algorithm implementations directly, not through the
// `Diff::FileDiff` dispatcher: each is its own library, and each deserves its own
// contract checks. All three replay their edit scripts through `ChunkedDiff`, so
// where their algorithms must agree (a single-line change has exactly one minimal
// script) their unified output must be byte-identical; where they legitimately
// differ (naive is greedy, direct is positional side-by-side) the tests say so
// rather than over-constraining.

namespace mbo::diff {
namespace {

using ::testing::HasSubstr;
using ::testing::IsEmpty;

struct DiffImplTest : ::testing::Test {
  // Bare hunks: no file headers, no context - the smallest comparable output.
  static DiffOptions BareOptions() {
    return DiffOptions{
        .context_size = 0,
        .file_header_use = DiffOptions::FileHeaderUse::kNone,
    };
  }

  static file::Artefact Text(std::string_view text) { return {.data = std::string(text)}; }
};

// Identical inputs -------------------------------------------------------------

TEST_F(DiffImplTest, MyersIdenticalInputsProduceNoDiff) {
  const auto result = DiffMyers::FileDiff(Text("a\nb\n"), Text("a\nb\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, IsEmpty());
}

TEST_F(DiffImplTest, NaiveIdenticalInputsProduceNoDiff) {
  const auto result = DiffNaive::FileDiff(Text("a\nb\n"), Text("a\nb\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, IsEmpty());
}

TEST_F(DiffImplTest, DirectIdenticalInputsProduceNoDiff) {
  const auto result = DiffDirect::FileDiff(Text("a\nb\n"), Text("a\nb\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, IsEmpty());
}

// A single-line change has exactly one minimal edit script, so Myers and Naive
// must agree byte for byte. -----------------------------------------------------

TEST_F(DiffImplTest, MyersSingleLineChange) {
  const auto result = DiffMyers::FileDiff(Text("a\nb\nc\n"), Text("a\nX\nc\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, "@@ -2 +2 @@\n-b\n+X\n");
}

TEST_F(DiffImplTest, NaiveAgreesWithMyersOnASingleLineChange) {
  const auto myers = DiffMyers::FileDiff(Text("a\nb\nc\n"), Text("a\nX\nc\n"), BareOptions());
  const auto naive = DiffNaive::FileDiff(Text("a\nb\nc\n"), Text("a\nX\nc\n"), BareOptions());
  ASSERT_THAT(myers.status(), absl::OkStatus());
  ASSERT_THAT(naive.status(), absl::OkStatus());
  EXPECT_THAT(*naive, *myers);
}

TEST_F(DiffImplTest, MyersPureInsertion) {
  const auto result = DiffMyers::FileDiff(Text("a\nc\n"), Text("a\nb\nc\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, "@@ -1,0 +2 @@\n+b\n");
}

TEST_F(DiffImplTest, MyersPureDeletion) {
  const auto result = DiffMyers::FileDiff(Text("a\nb\nc\n"), Text("a\nc\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, "@@ -2 +1,0 @@\n-b\n");
}

TEST_F(DiffImplTest, MyersEmptyVersusContent) {
  const auto result = DiffMyers::FileDiff(Text(""), Text("a\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, HasSubstr("+a"));
}

// Context lines pass through ChunkedDiff identically for both unified algorithms.

TEST_F(DiffImplTest, ContextLinesAreEmitted) {
  DiffOptions options = BareOptions();
  options.context_size = 1;
  const auto result = DiffMyers::FileDiff(Text("a\nb\nc\n"), Text("a\nX\nc\n"), options);
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, "@@ -1,3 +1,3 @@\n a\n-b\n+X\n c\n");
}

// Naive is documented as greedy and non-minimal: it must still produce a diff
// that names every changed line, even where Myers would emit fewer edits.

TEST_F(DiffImplTest, NaiveStillCoversAllChanges) {
  const auto result = DiffNaive::FileDiff(Text("a\nb\na\nb\n"), Text("b\na\nb\na\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, Not(IsEmpty()));
}

TEST_F(DiffImplTest, MyersMinimalOptionYieldsAMinimalScriptOnTheSameInput) {
  DiffOptions options = BareOptions();
  options.minimal = true;
  const auto capped = DiffMyers::FileDiff(Text("a\nb\na\nb\n"), Text("b\na\nb\na\n"), BareOptions());
  const auto minimal = DiffMyers::FileDiff(Text("a\nb\na\nb\n"), Text("b\na\nb\na\n"), options);
  ASSERT_THAT(capped.status(), absl::OkStatus());
  ASSERT_THAT(minimal.status(), absl::OkStatus());
  // For an input this small the cost cap never triggers, so both must be the
  // one minimal script: delete the leading "a", append a trailing "a".
  EXPECT_THAT(*minimal, *capped);
}

// Direct is a positional side-by-side view, not a unified diff: a changed line
// shows both versions on one row.

TEST_F(DiffImplTest, DirectShowsChangedLinesSideBySide) {
  const auto result = DiffDirect::FileDiff(Text("a\nb\nc\n"), Text("a\nX\nc\n"), BareOptions());
  ASSERT_THAT(result.status(), absl::OkStatus());
  EXPECT_THAT(*result, HasSubstr("b"));
  EXPECT_THAT(*result, HasSubstr("X"));
}

}  // namespace
}  // namespace mbo::diff
