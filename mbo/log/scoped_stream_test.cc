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

#include "mbo/log/scoped_stream.h"

#include <source_location>
#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::log {
namespace log_internal {

struct ScopedStreamTestAccess {
  template<typename ScopedStream>
  static std::string GetStr(const ScopedStream& str) {  // NOLINT(*-anonymous-namespace)
    return str.TestGetStr();
  }
};

}  // namespace log_internal

namespace {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::MatchesRegex;
using ::testing::Not;

struct ScopedStreamTest
    : ::testing::Test
    , log_internal::ScopedStreamTestAccess {};

TEST_F(ScopedStreamTest, TestOut) {
  {
    auto str = ScopedStreamOut();
    str << "Here" << "We" << "Go!";
    EXPECT_THAT(GetStr(str), "HereWeGo!");
  }
  std::stringstream out;
  {
    auto str = log_internal::ScopedStream(std::source_location::current(), out);
    str << "Here" << "We" << "Go!";
    EXPECT_THAT(GetStr(str), "HereWeGo!");
  }
  EXPECT_THAT(
      out.str(),
      MatchesRegex(
          R"rx(^\[[^\]]*/scoped_stream_test.cc:[0-9]+\] @.*void mbo::log.*::ScopedStreamTest_TestOut_Test::TestBody\(\) : HereWeGo!\n$)rx"));
}

TEST_F(ScopedStreamTest, TestVoid) {
  {
    auto str = ScopedStreamVoid();
    str << "Ignored!";
    EXPECT_THAT(GetStr(str), IsEmpty());
  }
  std::stringstream out;
  {
    auto str = log_internal::ScopedStream<ScopedStreamMode::kContinue, log_internal::VoidStream>(
        std::source_location::current(), out, "Still Here");
    str << "Ignored!";
    EXPECT_THAT(GetStr(str), IsEmpty());
  }
  EXPECT_THAT(
      out.str(),
      AllOf(
          MatchesRegex(
              R"rx(^\[[^\]]*/scoped_stream_test.cc:[0-9]+\] @.*void mbo::log.*::ScopedStreamTest_TestVoid_Test::TestBody\(\) : Still Here\n$)rx"),
          Not(HasSubstr("Ignored!"))));
}

void TestFail() {
  { ScopedStreamErr<ScopedStreamMode::kQuickExit>() << "YouAreDead"; }
  EXPECT_TRUE(false) << "Should not reach here";
}

TEST_F(ScopedStreamTest, TestFail) {
  EXPECT_DEATH(
      TestFail(), R"rx(^\[[^\]]*/scoped_stream_test.cc:[0-9]+\] @.*void mbo::log.*::TestFail\(\) : YouAreDead\n$)rx");
}

}  // namespace
}  // namespace mbo::log
