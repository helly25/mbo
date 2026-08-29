// SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
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

#include "mbo/diff/diff_options.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::diff {
namespace {

using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Field;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::NotNull;
using ::testing::Optional;

struct DiffOptionsTest : ::testing::Test {};

// Flag parsing. Every parser returns nullopt for an unknown value, and callers are
// expected to check - a caller that dereferences blindly is how `--file_header_use`
// once became undefined behaviour.

TEST_F(DiffOptionsTest, ParsesEveryAlgorithm) {
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("naive"), Optional(Eq(DiffOptions::Algorithm::kNaive)));
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("direct"), Optional(Eq(DiffOptions::Algorithm::kDirect)));
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("myers"), Optional(Eq(DiffOptions::Algorithm::kMyers)));
}

TEST_F(DiffOptionsTest, UnifiedIsADeprecatedAliasForMyers) {
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("unified"), Optional(Eq(DiffOptions::Algorithm::kMyers)));
}

TEST_F(DiffOptionsTest, RejectsAnUnknownAlgorithm) {
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("garbage").has_value(), IsFalse());
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("").has_value(), IsFalse());
  EXPECT_THAT(DiffOptions::ParseAlgorithmFlag("Myers").has_value(), IsFalse()) << "matching is case sensitive";
}

TEST_F(DiffOptionsTest, ParsesEveryOutputFormat) {
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("unified"), Optional(Eq(DiffOptions::OutputFormat::kUnified)));
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("context"), Optional(Eq(DiffOptions::OutputFormat::kContext)));
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("normal"), Optional(Eq(DiffOptions::OutputFormat::kNormal)));
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("side-by-side"), Optional(Eq(DiffOptions::OutputFormat::kSideBySide)));
}

TEST_F(DiffOptionsTest, RejectsAnUnknownOutputFormat) {
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("garbage").has_value(), IsFalse());
  EXPECT_THAT(DiffOptions::ParseOutputFormatFlag("side_by_side").has_value(), IsFalse()) << "the separator is a dash";
}

TEST_F(DiffOptionsTest, ParsesEveryFileHeaderUse) {
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("none"), Optional(Eq(DiffOptions::FileHeaderUse::kNone)));
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("both"), Optional(Eq(DiffOptions::FileHeaderUse::kBoth)));
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("left"), Optional(Eq(DiffOptions::FileHeaderUse::kLeft)));
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("right"), Optional(Eq(DiffOptions::FileHeaderUse::kRight)));
}

TEST_F(DiffOptionsTest, RejectsAnUnknownFileHeaderUse) {
  // This is the exact input that used to reach an unchecked dereference.
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("garbage").has_value(), IsFalse());
  EXPECT_THAT(DiffOptions::ParseFileHeaderUse("").has_value(), IsFalse());
}

// Regex replace ---------------------------------------------------------------

TEST_F(DiffOptionsTest, FourPartsIsTheAcceptedShape) {
  // "/only/two/" LOOKS malformed but is not: splitting on '/' yields exactly
  // {"", "only", "two", ""}, which is the required shape.
  EXPECT_THAT(
      DiffOptions::ParseRegexReplaceFlag("/only/two/"), Optional(Field(&DiffOptions::RegexReplace::replace, "two")));
}

TEST_F(DiffOptionsTest, ParsesARegexReplaceFlag) {
  EXPECT_THAT(
      DiffOptions::ParseRegexReplaceFlag("/foo/bar/"),
      Optional(AllOf(
          Field(&DiffOptions::RegexReplace::replace, "bar"), Field(&DiffOptions::RegexReplace::regex, NotNull()))));
}

TEST_F(DiffOptionsTest, RegexReplaceTakesItsSeparatorFromTheFirstCharacter) {
  // Any character can be the separator, which is what lets a pattern contain '/'.
  EXPECT_THAT(DiffOptions::ParseRegexReplaceFlag(",a/b,c,"), Optional(Field(&DiffOptions::RegexReplace::replace, "c")));
}

TEST_F(DiffOptionsTest, EmptyRegexReplaceFlagIsNullopt) {
  EXPECT_THAT(DiffOptions::ParseRegexReplaceFlag("").has_value(), IsFalse());
}

TEST_F(DiffOptionsTest, MalformedRegexReplaceFlagAborts) {
  // NOTE: unlike every other parser here, this one does not return nullopt for bad
  // input - it ABSL_CHECKs that the flag splits into exactly four parts with empty
  // ends. A user typo therefore terminates the process rather than producing a flag
  // error. Pinned as current behaviour, not endorsed as good behaviour.
  // Three parts, not four: "/a/" splits to {"", "a", ""}.
  EXPECT_DEATH({ (void)DiffOptions::ParseRegexReplaceFlag("/a/"); }, "");
  // Five parts: one separator too many.
  EXPECT_DEATH({ (void)DiffOptions::ParseRegexReplaceFlag("/a/b/c/"); }, "");
}

TEST_F(DiffOptionsTest, DefaultIsStable) {
  const DiffOptions& first = DiffOptions::Default();
  const DiffOptions& second = DiffOptions::Default();
  EXPECT_THAT(&first == &second, IsTrue()) << "Default() returns the same instance";
}

}  // namespace
}  // namespace mbo::diff
