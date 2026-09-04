// SPDX-FileCopyrightText: Copyright (c) M. Boerger, The MBO Works Authors
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/config/config.h"
#include "mbo/container/limited_map.h"

namespace mbo::container {
namespace {

using ::testing::HasSubstr;
using ::testing::ThrowsMessage;

struct LimitedMapRequireExceptionsTest : ::testing::Test {};

TEST_F(LimitedMapRequireExceptionsTest, ReportsMissingKey) {
  if constexpr (!config::kRequireThrows) {
    GTEST_SKIP() << "requires --//mbo/config:require_throws=true";
  }
  LimitedMap<int, int, 1> map{{1, 2}};
  EXPECT_THAT([&map] { static_cast<void>(map.at(2)); }, ThrowsMessage<std::runtime_error>(HasSubstr("Out of range")));
}

}  // namespace
}  // namespace mbo::container
