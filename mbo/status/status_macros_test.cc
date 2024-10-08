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

#include "mbo/status/status_macros.h"

#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/status.h"

namespace mbo::status {
namespace {

using ::mbo::testing::IsOk;
using ::mbo::testing::StatusIs;

struct StatusMacrosTest : ::testing::Test {
  template<typename T>
  static absl::Status TestReturnIfError(T&& v, bool expect_early_return) {
    bool early_return = true;
    absl::Cleanup done = [&] { EXPECT_THAT(early_return, expect_early_return); };
    MBO_RETURN_IF_ERROR(std::forward<T>(v));
    early_return = false;
    return absl::OkStatus();
  }

  template<typename T, typename V>
  static absl::Status TestAssignOrReturn(T&& status_or, const V& expected) {
    MBO_ASSIGN_OR_RETURN(const V got, std::forward<T>(status_or));
    EXPECT_THAT(got, expected);
    return absl::OkStatus();
  }
};

TEST_F(StatusMacrosTest, StatusToStatus) {
  {
    const absl::Status status;
    EXPECT_THAT(status, IsOk());
    EXPECT_THAT(TestReturnIfError(status, false), IsOk());
  }
  {
    const absl::Status status = absl::CancelledError();
    EXPECT_THAT(status, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(TestReturnIfError(status, true), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusMacrosTest, StatusMoveToStatus) {
  {
    absl::Status status;
    EXPECT_THAT(status, IsOk());
    EXPECT_THAT(TestReturnIfError(std::move(status), false), IsOk());
  }
  {
    absl::Status status = absl::CancelledError();
    EXPECT_THAT(status, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(TestReturnIfError(std::move(status), true), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusMacrosTest, StatusOrToStatus) {
  {
    const absl::StatusOr<int> status_or(1);
    EXPECT_THAT(status_or, IsOk());
    EXPECT_THAT(TestReturnIfError(status_or, false), IsOk());
  }
  {
    const absl::StatusOr<int> status_or = absl::CancelledError();
    EXPECT_THAT(status_or, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(TestReturnIfError(status_or, true), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusMacrosTest, StatusOrMoveToStatus) {
  {
    absl::StatusOr<int> status_or(1);
    EXPECT_THAT(status_or, IsOk());
    EXPECT_THAT(TestReturnIfError(std::move(status_or), false), IsOk());
  }
  {
    absl::StatusOr<int> status_or = absl::CancelledError();
    EXPECT_THAT(status_or, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(TestReturnIfError(std::move(status_or), true), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusMacrosTest, AssignOrReturn) {
  {
    absl::StatusOr<int> status_or(1);
    EXPECT_THAT(TestAssignOrReturn(status_or, 1), IsOk());
    EXPECT_THAT(TestAssignOrReturn(std::move(status_or), 1), IsOk());
  }
  {
    absl::StatusOr<int> status_or = absl::CancelledError();
    EXPECT_THAT(TestAssignOrReturn(status_or, 1), StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(TestAssignOrReturn(std::move(status_or), 1), StatusIs(absl::StatusCode::kCancelled));
  }
}

}  // namespace
}  // namespace mbo::status
