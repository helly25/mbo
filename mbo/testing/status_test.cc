// Copyright 2023 M. Boerger (helly25.com)
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

#include "mbo/testing/status.h"

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::testing {
namespace {

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
// NOLINTBEGIN(readability-magic-numbers)

using ::mbo::testing::IsOk;          // Explicit
using ::mbo::testing::IsOkAndHolds;  // Explicit
using ::mbo::testing::StatusIs;      // Explicit
using ::mbo::testing::testing_internal::IsOkAndHoldsMatcher;
using ::mbo::testing::testing_internal::IsOkMatcher;
using ::mbo::testing::testing_internal::StatusIsMatcher;
using ::testing::_;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pair;

class StatusMatcherTest : public ::testing::Test {
 public:
  template<typename Matcher>
  static std::string Describe(const Matcher& matcher) {
    std::ostringstream os;
    matcher.DescribeTo(&os);
    return os.str();
  }

  template<typename Matcher>
  static std::string DescribeNegation(const Matcher& matcher) {
    std::ostringstream os;
    matcher.DescribeNegationTo(&os);
    return os.str();
  }

  template<typename Matcher, typename V>
  static std::pair<bool, std::string> MatchAndExplain(
      const Matcher& matcher,
      const V& value) {
    ::testing::StringMatchResultListener os;
    const bool result = matcher.MatchAndExplain(value, &os);
    return {result, os.str()};
  }
};

TEST_F(StatusMatcherTest, IsOk) {
  EXPECT_THAT(absl::OkStatus(), IsOk());
  EXPECT_THAT(absl::AbortedError(""), Not(IsOk()));
  absl::StatusOr<int> val;
  EXPECT_THAT(val, Not(IsOk()));
  val = 42;
  EXPECT_THAT(val, IsOk());
  val = absl::UnknownError("");
  EXPECT_THAT(val, Not(IsOk()));

  {
    const ::testing::Matcher<absl::Status> matcher = IsOkMatcher();
    EXPECT_THAT(Describe(matcher), "is OK");
    EXPECT_THAT(DescribeNegation(matcher), "is not OK");
    EXPECT_THAT(MatchAndExplain(matcher, absl::OkStatus()), Pair(true, ""));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("Message")),
        Pair(false, "which has status UNKNOWN: 'Message'"));
  }
  {
    const ::testing::Matcher<absl::StatusOr<int>> matcher = IsOkMatcher();
    EXPECT_THAT(Describe(matcher), "is OK");
    EXPECT_THAT(DescribeNegation(matcher), "is not OK");
    EXPECT_THAT(MatchAndExplain(matcher, 42), Pair(true, ""));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("Message")),
        Pair(false, "which has status UNKNOWN: 'Message'"));
  }
}

TEST_F(StatusMatcherTest, StatusIs) {
  EXPECT_THAT(absl::OkStatus(), StatusIs(absl::StatusCode::kOk));
  EXPECT_THAT(absl::OkStatus(), StatusIs(absl::StatusCode::kOk, ""));
  EXPECT_THAT(absl::AbortedError(""), StatusIs(absl::StatusCode::kAborted));
  EXPECT_THAT(
      absl::AbortedError("a-sub-c"),
      StatusIs(absl::StatusCode::kAborted, "a-sub-c"));
  EXPECT_THAT(
      absl::AbortedError("a-sub-c"),
      StatusIs(absl::StatusCode::kAborted, HasSubstr("-sub-")));
  EXPECT_THAT(
      absl::AbortedError("a-sub-c"),
      StatusIs(absl::StatusCode::kAborted, Not(HasSubstr("not-a-substring"))));
  absl::StatusOr<int> test;
  EXPECT_THAT(test, Not(StatusIs(absl::StatusCode::kOk)));
  test = 42;
  EXPECT_THAT(test, StatusIs(absl::StatusCode::kOk));
  test = absl::UnknownError("");
  EXPECT_THAT(test, StatusIs(absl::StatusCode::kUnknown));

  {
    const StatusIsMatcher matcher(absl::StatusCode::kOk, "x");
    EXPECT_THAT(Describe(matcher), "OK and the message is equal to \"x\"");
    EXPECT_THAT(
        DescribeNegation(matcher),
        "not (OK and the message is equal to \"x\")");
    EXPECT_THAT(MatchAndExplain(matcher, absl::OkStatus()), Pair(true, ""))
        << "Must ignore status message if Ok";
  }
  {
    const StatusIsMatcher matcher(absl::StatusCode::kOk, "");
    EXPECT_THAT(Describe(matcher), "OK and the message is equal to \"\"");
    EXPECT_THAT(
        DescribeNegation(matcher), "not (OK and the message is equal to \"\")");
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("")),
        Pair(
            false,
            "which has status UNKNOWN that isn't OK and has a matching "
            "message"));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("Message")),
        Pair(
            false,
            "which has status UNKNOWN that isn't OK and has message 'Message' which does not match the expected empty "
            "message"));
  }
  {
    const StatusIsMatcher matcher(absl::StatusCode::kCancelled, "Wanted");
    EXPECT_THAT(
        Describe(matcher), "CANCELLED and the message is equal to \"Wanted\"");
    EXPECT_THAT(
        DescribeNegation(matcher),
        "not (CANCELLED and the message is equal to \"Wanted\")");
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::OkStatus()),
        Pair(false, "which has status OK that isn't CANCELLED"));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::AbortedError("Wanted")),
        Pair(
            false,
            "which has status ABORTED that isn't CANCELLED "
            "and has a matching message"));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::CancelledError("Message")),
        Pair(
            false,
            "which has matching status CANCELLED "
            "and has message 'Message' which does not match the expected empty message"));
  }
  {
    const StatusIsMatcher matcher(absl::StatusCode::kDataLoss, IsEmpty());
    EXPECT_THAT(Describe(matcher), "DATA_LOSS and the message is empty");
    EXPECT_THAT(
        DescribeNegation(matcher), "not (DATA_LOSS and the message is empty)");
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::DataLossError("Message")),
        Pair(
            false,
            "which has matching status DATA_LOSS "
            "and has message 'Message' which does not match 'whose size is 7'"));
  }
}

TEST_F(StatusMatcherTest, IsOkAndHolds) {
  {
    absl::StatusOr<int> test;
    EXPECT_THAT(test, Not(IsOkAndHolds(_)));
    test = 42;
    EXPECT_THAT(test, IsOkAndHolds(42));
    EXPECT_THAT(test, IsOkAndHolds(Not(25)));
    test = absl::UnknownError("");
    EXPECT_THAT(test, Not(IsOkAndHolds(_)));
  }
  {
    const ::testing::Matcher<absl::StatusOr<int>> matcher =
        IsOkAndHoldsMatcher<int>(42);
    EXPECT_THAT(Describe(matcher), "is OK and has a value that is equal to 42");
    EXPECT_THAT(
        DescribeNegation(matcher),
        "isn't OK or has a value that isn't equal to 42");
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::StatusOr<int>{42}), Pair(true, ""));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::StatusOr<int>{25}),
        Pair(false, "which contains value 25"));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("")),
        Pair(false, "which has status UNKNOWN"));
  }
  {
    const ::testing::Matcher<absl::StatusOr<int>> matcher =
        IsOkAndHolds(Ge(42));
    EXPECT_THAT(Describe(matcher), "is OK and has a value that is >= 42");
    EXPECT_THAT(
        DescribeNegation(matcher), "isn't OK or has a value that isn't >= 42");
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::StatusOr<int>{42}), Pair(true, ""));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::StatusOr<int>{25}),
        Pair(false, "which contains value 25"));
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::UnknownError("")),
        Pair(false, "which has status UNKNOWN"));
  }
  {
    const ::testing::Matcher<absl::StatusOr<std::string>> matcher =
        IsOkAndHolds(IsEmpty());
    EXPECT_THAT(
        MatchAndExplain(matcher, absl::StatusOr<std::string>{"25"}),
        Pair(false, "which contains value \"25\", whose size is 2"));
  }
}

// NOLINTEND(readability-magic-numbers)
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}  // namespace
}  // namespace mbo::testing
