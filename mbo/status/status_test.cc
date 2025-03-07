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

#include "mbo/status/status.h"

#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/status.h"

namespace mbo::status {
namespace {

using ::mbo::testing::IsOk;
using ::mbo::testing::StatusIs;

struct StatusTest : ::testing::Test {};

TEST_F(StatusTest, StatusToStatus) {
  {
    const absl::Status status;
    EXPECT_THAT(status, IsOk());
    EXPECT_THAT(GetStatus(status), IsOk());
  }
  {
    const absl::Status status = absl::CancelledError();
    EXPECT_THAT(status, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(GetStatus(status), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusTest, StatusMoveToStatus) {
  {
    absl::Status status;
    EXPECT_THAT(status, IsOk());
    EXPECT_THAT(GetStatus(std::move(status)), IsOk());
  }
  {
    absl::Status status = absl::CancelledError();
    EXPECT_THAT(status, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(GetStatus(std::move(status)), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusTest, StatusOrToStatus) {
  {
    const absl::StatusOr<int> status_or(1);
    EXPECT_THAT(status_or, IsOk());
    EXPECT_THAT(GetStatus(status_or), IsOk());
  }
  {
    const absl::StatusOr<int> status_or = absl::CancelledError();
    EXPECT_THAT(status_or, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(GetStatus(status_or), StatusIs(absl::StatusCode::kCancelled));
  }
}

TEST_F(StatusTest, StatusOrMoveToStatus) {
  {
    absl::StatusOr<int> status_or(1);
    EXPECT_THAT(status_or, IsOk());
    EXPECT_THAT(GetStatus(std::move(status_or)), IsOk());
  }
  {
    absl::StatusOr<int> status_or = absl::CancelledError();
    EXPECT_THAT(status_or, StatusIs(absl::StatusCode::kCancelled));
    EXPECT_THAT(GetStatus(std::move(status_or)), StatusIs(absl::StatusCode::kCancelled));
  }
}
}  // namespace
}  // namespace mbo::status
