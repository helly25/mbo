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

#include "mbo/strings/parse.h"

#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/status/status_macros.h"
#include "mbo/testing/status.h"

namespace mbo::strings {
namespace {

using ::mbo::testing::IsOkAndHolds;
using ::mbo::testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;

class ParseTest : public ::testing::Test {
 public:
  static absl::StatusOr<std::pair<std::string, std::string>> ParseString(
      const ParseOptions& options,
      std::string_view data) {
    MBO_STATUS_ASSIGN_OR_RETURN(std::string result, mbo::strings::ParseString(options, data));
    return std::make_pair(std::move(result), std::string(data));
  }

  static absl::StatusOr<std::pair<std::vector<std::string>, std::string>> ParseStringList(
      const ParseOptions& options,
      std::string_view data) {
    MBO_STATUS_ASSIGN_OR_RETURN(std::vector<std::string> result, mbo::strings::ParseStringList(options, data));
    return std::make_pair(std::move(result), std::string(data));
  }
};

TEST_F(ParseTest, ParseString) {
  EXPECT_THAT(ParseString({}, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString({}, "42"), IsOkAndHolds(Pair("42", "")));
  EXPECT_THAT(ParseString({}, "4,2"), IsOkAndHolds(Pair("4,2", "")));
}

TEST_F(ParseTest, ParseStringOctal) {
  EXPECT_THAT(ParseString({}, "\\1234"), IsOkAndHolds(Pair("S4", "")));
  EXPECT_THAT(ParseString({}, "\\19"), IsOkAndHolds(Pair("\19", "")));
  EXPECT_THAT(
      ParseString({}, "\\9a"),
      StatusIs(absl::StatusCode::kInvalidArgument, "StringList has unsupported escape sequence."));
  EXPECT_THAT(ParseString({}, "\\o{123}9"), IsOkAndHolds(Pair("S9", "")));
  EXPECT_THAT(ParseString({}, "4,2"), IsOkAndHolds(Pair("4,2", "")));
}

TEST_F(ParseTest, ParseStringHex) {
  EXPECT_THAT(ParseString({}, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString({}, "\\x4"), IsOkAndHolds(Pair("\4", "")));
  EXPECT_THAT(ParseString({}, "\\x42"), IsOkAndHolds(Pair("\x42", "")));
  EXPECT_THAT(ParseString({}, "\\x423"), IsOkAndHolds(Pair(absl::StrCat("\x42", "3"), "")));
  EXPECT_THAT(ParseString({}, "\\x{42}"), IsOkAndHolds(Pair("\x42", "")));
  EXPECT_THAT(ParseString({}, "\\x{4}"), IsOkAndHolds(Pair("\4", "")));
  EXPECT_THAT(
      ParseString({}, "\\x{423}"),
      StatusIs(absl::StatusCode::kInvalidArgument, "StringList has bad hex C++23 sequence."));
}

TEST_F(ParseTest, ParseStringStopAtAnyOf) {
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, "."), IsOkAndHolds(Pair("", ".")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, ","), IsOkAndHolds(Pair("", ",")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, "4;2"), IsOkAndHolds(Pair("4;2", "")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, "4,2"), IsOkAndHolds(Pair("4", ",2")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, "4.2"), IsOkAndHolds(Pair("4", ".2")));
  EXPECT_THAT(ParseString({.stop_at_any_of = ".,"}, "."), IsOkAndHolds(Pair("", ".")));
}

TEST_F(ParseTest, ParseStringRemoveQuotes) {
  ASSERT_THAT(ParseOptions().remove_quotes, true);
  ASSERT_THAT(ParseOptions().allow_unquoted, true);
  EXPECT_THAT(ParseString({}, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString({}, "."), IsOkAndHolds(Pair(".", "")));
  EXPECT_THAT(ParseString({}, "'1,2'\",3\""), IsOkAndHolds(Pair("1,2,3", "")));
  EXPECT_THAT(ParseString({}, "'1,2',3"), IsOkAndHolds(Pair("1,2,3", "")));
  static constexpr ParseOptions kOpts{
    .remove_quotes = false,
  };
  EXPECT_THAT(ParseString(kOpts, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString(kOpts, "."), IsOkAndHolds(Pair(".", "")));
  EXPECT_THAT(ParseString(kOpts, "'1,2'\",3\""), IsOkAndHolds(Pair("'1,2'\",3\"", "")));
  EXPECT_THAT(ParseString(kOpts, "'1,2',3"), IsOkAndHolds(Pair("'1,2',3", "")));
}

TEST_F(ParseTest, ParseStringAllowUnquoted) {
  ASSERT_THAT(ParseOptions().remove_quotes, true);
  ASSERT_THAT(ParseOptions().allow_unquoted, true);
  EXPECT_THAT(ParseString({}, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString({}, "."), IsOkAndHolds(Pair(".", "")));
  EXPECT_THAT(ParseString({}, "'1,2'\",3\""), IsOkAndHolds(Pair("1,2,3", "")));
  EXPECT_THAT(ParseString({}, "'1,2',3"), IsOkAndHolds(Pair("1,2,3", "")));
  static constexpr ParseOptions kOpts{
    .allow_unquoted = false,
  };
  EXPECT_THAT(ParseString(kOpts, ""), IsOkAndHolds(Pair("", "")));
  EXPECT_THAT(ParseString(kOpts, "."), IsOkAndHolds(Pair("", ".")));
  EXPECT_THAT(ParseString(kOpts, "'1,2'\",3\""), IsOkAndHolds(Pair("1,2,3", "")));
  EXPECT_THAT(ParseString(kOpts, "'1,2',3"), IsOkAndHolds(Pair("1,2", ",3")));
}

TEST_F(ParseTest, ParseStringList) {
  EXPECT_THAT(ParseStringList({}, ""), IsOkAndHolds(Pair(IsEmpty(), "")));
  EXPECT_THAT(ParseStringList({}, "1,2"), IsOkAndHolds(Pair(ElementsAre("1", "2"), "")));
}

TEST_F(ParseTest, ParseStringListSplitAtAnyOf) {
  EXPECT_THAT(ParseStringList({.split_at_any_of = "."}, ""), IsOkAndHolds(Pair(IsEmpty(), "")));
  EXPECT_THAT(ParseStringList({.split_at_any_of = "."}, "."), IsOkAndHolds(Pair(ElementsAre("", ""), "")));
  EXPECT_THAT(ParseStringList({.split_at_any_of = "."}, ".4,2"), IsOkAndHolds(Pair(ElementsAre("", "4,2"), "")));
  EXPECT_THAT(ParseStringList({.split_at_any_of = "."}, "4,2."), IsOkAndHolds(Pair(ElementsAre("4,2", ""), "")));
  EXPECT_THAT(ParseStringList({.split_at_any_of = "."}, "4,2"), IsOkAndHolds(Pair(ElementsAre("4,2"), "")));
  EXPECT_THAT(ParseStringList({.split_at_any_of = ","}, "4,2"), IsOkAndHolds(Pair(ElementsAre("4", "2"), "")));
  EXPECT_THAT(
      ParseStringList({.split_at_any_of = ".,;"}, "4,3;2.1"), IsOkAndHolds(Pair(ElementsAre("4", "3", "2", "1"), "")));
  EXPECT_THAT(
      ParseStringList({.split_at_any_of = ".,;"}, ".,;."), IsOkAndHolds(Pair(ElementsAre("","", "", "", ""), "")));
}

TEST_F(ParseTest, ParseStringListStopAtAnyOf) {
  static constexpr ParseOptions kOpts{
    .stop_at_any_of = ".",
  };
  EXPECT_THAT(ParseStringList(kOpts, ""), IsOkAndHolds(Pair(IsEmpty(), "")));
  EXPECT_THAT(ParseStringList(kOpts, "."), IsOkAndHolds(Pair(IsEmpty(), ".")));
  EXPECT_THAT(ParseStringList(kOpts, "1,2.3,4"), IsOkAndHolds(Pair(ElementsAre("1", "2"), ".3,4")));
  EXPECT_THAT(ParseStringList(kOpts, "1,2,.3,4"), IsOkAndHolds(Pair(ElementsAre("1", "2", ""), ".3,4")));
}

}  // namespace
}  // namespace mbo::strings
