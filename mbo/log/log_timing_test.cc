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

#include "mbo/log/log_timing.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::log::log_internal {
namespace {

struct StripFunctionNameTest : ::testing::Test {};

TEST_F(StripFunctionNameTest, Empty) {
  EXPECT_THAT(StripFunctionName(""), "");
}

TEST_F(StripFunctionNameTest, Simple) {
  EXPECT_THAT(StripFunctionName("auto foo()"), "foo");
  EXPECT_THAT(StripFunctionName("auto foo(int, int)"), "foo");
}

TEST_F(StripFunctionNameTest, Operator) {
  EXPECT_THAT(StripFunctionName("auto Foo::operator()()"), "Foo::operator()");
  EXPECT_THAT(
      StripFunctionName("std::function<void(bool, int)> Foo::operator()(std::function<void(bool, int))"),
      "Foo::operator()");
}

}  // namespace
}  // namespace mbo::log::log_internal
