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

#ifndef MBO_TESTING_STATUS_H_
#define MBO_TESTING_STATUS_H_

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/status/status_macros.h"

namespace mbo::testing {
namespace testing_internal {

inline const ::absl::Status& GetStatus(const ::absl::Status& status) {
  return status;
}

template<typename T>
inline const ::absl::Status& GetStatus(const ::absl::StatusOr<T>& status) {
  return status.status();
}

inline std::string DescribeStatus(absl::Status status) {
  std::string code = ::testing::PrintToString(status.code());
  if (status.message().empty()) {
    return code;
  } else {
    return absl::StrCat(code, ": '", status.message(), "'");
  }
}

// Monomorphic implementation of matcher IsOk() for a given type T.
// T can be Status, StatusOr<>, or a reference to either of them.
template<typename T>
class MonoIsOkMatcherImpl : public ::testing::MatcherInterface<T> {
 public:
  MonoIsOkMatcherImpl() = default;

  void DescribeTo(std::ostream* os) const override { *os << "is OK"; }

  void DescribeNegationTo(std::ostream* os) const override { *os << "is not OK"; }

  bool MatchAndExplain(T actual_value, ::testing::MatchResultListener* result) const override {
    const absl::Status status = testing_internal::GetStatus(actual_value);
    if (!status.ok()) {
      *result << "which has status " << DescribeStatus(status);
    }
    return status.ok();
  }
};

// Implements IsOk() as a polymorphic matcher.
class IsOkMatcher {
 public:
  IsOkMatcher() = default;

  template<typename T>
  operator ::testing::Matcher<T>() const {  // NOLINT
    return ::testing::Matcher<T>(new MonoIsOkMatcherImpl<T>());
  }
};

// Monomorphic implementation of matcher IsOkAndHolds(m).  StatusOrType is a
// reference to StatusOr<T>.
template<typename StatusOrType>
class IsOkAndHoldsMatcherImpl : public ::testing::MatcherInterface<StatusOrType> {
 public:
  // NOLINTNEXTLINE(readability-identifier-naming)
  using value_type = typename std::remove_reference<StatusOrType>::type::value_type;

  template<typename InnerMatcher, std::enable_if_t<!std::is_same_v<InnerMatcher, IsOkAndHoldsMatcherImpl>, int> = 0>
  explicit IsOkAndHoldsMatcherImpl(InnerMatcher&& inner_matcher)
      : inner_matcher_(::testing::SafeMatcherCast<const value_type&>(std::forward<InnerMatcher>(inner_matcher))) {}

  void DescribeTo(std::ostream* os) const override {
    *os << "is OK and has a value that ";
    inner_matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "isn't OK or has a value that ";
    inner_matcher_.DescribeNegationTo(os);
  }

  bool MatchAndExplain(StatusOrType actual_value, ::testing::MatchResultListener* result_listener) const override {
    if (!actual_value.ok()) {
      *result_listener << "which has status " << DescribeStatus(actual_value.status());
      return false;
    }

    ::testing::StringMatchResultListener inner;
    const bool matches = inner_matcher_.MatchAndExplain(*actual_value, &inner);
    if (!inner.str().empty()) {
      *result_listener << "which contains value " << ::testing::PrintToString(*actual_value) << ", " << inner.str();
    } else if (!matches) {
      *result_listener << "which contains value " << ::testing::PrintToString(*actual_value);
    }
    return matches;
  }

 private:
  const ::testing::Matcher<const value_type&> inner_matcher_;
};

// Implements IsOkAndHolds(m) as a polymorphic matcher.
template<typename InnerMatcher>
class IsOkAndHoldsMatcher {
 public:
  explicit IsOkAndHoldsMatcher(InnerMatcher inner_matcher) : inner_matcher_(std::move(inner_matcher)) {}

  // Converts this polymorphic matcher to a monomorphic matcher of the
  // given type.  StatusOrType can be either StatusOr<T> or a
  // reference to StatusOr<T>.
  template<typename StatusOrType>
  operator ::testing::Matcher<StatusOrType>() const {  // NOLINT
    return ::testing::Matcher<StatusOrType>(new IsOkAndHoldsMatcherImpl<const StatusOrType&>(inner_matcher_));
  }

 private:
  const InnerMatcher inner_matcher_;
};

// Implements a status matcher interface to verify a status error code, and
// error message if set, matches an expected value.
//
// Sample usage:
//   EXPECT_THAT(MyCall(), StatusIs(absl::StatusCode::kNotFound,
//                                  HasSubstr("message")));
//
// Sample output on failure:
//   Value of: MyCall()
//   Expected: NOT_FOUND and has substring "message"
//     Actual: UNKNOWN: descriptive error (of type absl::lts_2020_02_25::Status)
class StatusIsMatcher {
 public:
  StatusIsMatcher(const absl::StatusCode& status_code, const ::testing::Matcher<const std::string&>& message_matcher)
      : status_code_(status_code), message_matcher_(message_matcher) {}

  void DescribeTo(std::ostream* os) const {
    *os << status_code_ << " and the message ";
    message_matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "not (";
    DescribeTo(os);
    *os << ")";
  }

  template<typename StatusType>
  bool MatchAndExplain(const StatusType& actual, ::testing::MatchResultListener* listener) const {
    const absl::Status& actual_status = testing_internal::GetStatus(actual);
    const bool code_match = actual_status.code() == status_code_;
    if (status_code_ == absl::StatusCode::kOk && code_match) {
      return true;
    }
    ::testing::StringMatchResultListener inner;
    const bool message_match = message_matcher_.MatchAndExplain(std::string{actual_status.message()}, &inner);
    if (code_match && message_match) {
      return true;
    }
    if (code_match) {
      *listener << "which has matching status " << actual_status.code();
    } else {
      *listener << "which has status " << actual_status.code() << " that isn't " << status_code_;
    }
    if (actual_status.code() != absl::StatusCode::kOk) {
      *listener << " and ";
      if (message_match) {
        *listener << "has a matching message";
      } else {
        *listener << "has message ";
        if (actual_status.message().empty()) {
          *listener << " an empty message ";
        } else {
          *listener << "'" << actual_status.message() << "' ";
        }
        *listener << "which does not match";
        if (inner.str().empty()) {
          *listener << " the expected empty message";
        } else {
          *listener << " '" << inner.str() << "'";
        }
      }
    }
    return false;
  }

 private:
  const absl::StatusCode status_code_;
  const ::testing::Matcher<const std::string&> message_matcher_;
};

}  // namespace testing_internal

// Returns a gMock matcher that matches a Status or StatusOr<> which is OK.
inline testing_internal::IsOkMatcher IsOk() {
  return {};
}

// Returns a gMock matcher that matches a StatusOr<> whose status is
// OK and whose value matches the inner matcher.
template<typename InnerMatcher>
testing_internal::IsOkAndHoldsMatcher<typename std::decay<InnerMatcher>::type> IsOkAndHolds(
    InnerMatcher&& inner_matcher) {
  return testing_internal::IsOkAndHoldsMatcher<typename std::decay<InnerMatcher>::type>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      std::forward<InnerMatcher>(inner_matcher));
}

// Status matcher that checks the StatusCode for an expected value.
inline ::testing::PolymorphicMatcher<testing_internal::StatusIsMatcher> StatusIs(const absl::StatusCode& code) {
  return ::testing::MakePolymorphicMatcher(testing_internal::StatusIsMatcher(code, ::testing::_));
}

// Status matcher that checks the StatusCode and message for expected values.
template<typename MessageMatcher>
::testing::PolymorphicMatcher<testing_internal::StatusIsMatcher> StatusIs(
    const absl::StatusCode& code,
    const MessageMatcher& message) {
  return ::testing::MakePolymorphicMatcher(
      testing_internal::StatusIsMatcher(code, ::testing::MatcherCast<const std::string&>(message)));
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
// NOLINTBEGIN(bugprone-macro-parentheses)

// Helper macros
#undef MBO_STATUS_MACROS_IMPL_CONCAT_INNER_
#undef MBO_STATUS_MACROS_IMPL_CONCAT_
#define MBO_STATUS_MACROS_IMPL_CONCAT_INNER_(x, y) x##y
#define MBO_STATUS_MACROS_IMPL_CONCAT_(x, y) MBO_STATUS_MACROS_IMPL_CONCAT_INNER_(x, y)

// Macros for testing the results of functions that return absl::Status or
// absl::StatusOr<T> (for any type T).
#undef MBO_EXPECT_OK
#define MBO_EXPECT_OK(expression) EXPECT_THAT(expression, mbo::testing::IsOk())

#ifndef EXPECT_OK
#define EXPECT_OK(expr) MBO_EXPECT_OK(expr)
#elif __cplusplus >= 202'302L
#warning "EXPECT_OK already defined"
#endif

#undef MBO_ASSERT_OK
#define MBO_ASSERT_OK(expression) ASSERT_THAT(expression, mbo::testing::IsOk())

#ifndef ASSERT_OK
#define ASSERT_OK(expr) MBO_ASSERT_OK(expr)
#elif __cplusplus >= 202'302L
#warning "ASSERT_OK already defined"
#endif

#undef MBO_ASSERT_OK_AND_ASSIGN_IMPL_
#undef MBO_ASSERT_OK_AND_ASSIGN

#define MBO_ASSERT_OK_AND_ASSIGN_IMPL_(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                                   \
  ASSERT_TRUE(statusor.ok());                                \
  lhs = std::move(statusor.value())

#define MBO_ASSERT_OK_AND_ASSIGN(lhs, rexpr) \
  MBO_ASSERT_OK_AND_ASSIGN_IMPL_(MBO_STATUS_MACROS_IMPL_CONCAT_(_status_or_value, __LINE__), lhs, rexpr)

#ifndef ASSERT_OK_AND_ASSIGN
#define ASSERT_OK_AND_ASSIGN(lhs, rexpr) MBO_ASSERT_OK_AND_ASSIGN(lhs, rexpr)
#elif __cplusplus >= 202'302L
#warning "ASSERT_OK_AND_ASSIGN already defined"
#endif

// NOLINTEND(bugprone-macro-parentheses)
// NOLINTEND(cppcoreguidelines-macro-usage)

}  // namespace mbo::testing

#endif  // MBO_TESTING_STATUS_H_
