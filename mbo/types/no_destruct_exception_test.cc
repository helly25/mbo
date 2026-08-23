// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/no_destruct.h"

namespace mbo::types {
namespace {

struct ThrowingValue {
  explicit ThrowingValue(int /*value*/) { throw std::runtime_error("requested construction failure"); }
};

static_assert(!noexcept(NoDestruct<ThrowingValue>(1)));

struct NoDestructExceptionTest : ::testing::Test {};

TEST_F(NoDestructExceptionTest, ConstructionPropagatesException) {
  bool caught = false;
  try {
    const NoDestruct<ThrowingValue> value(1);
  } catch (const std::runtime_error&) {
    caught = true;
  }

  EXPECT_THAT(caught, true);
}

}  // namespace
}  // namespace mbo::types
