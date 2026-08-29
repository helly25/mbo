// SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
// SPDX-License-Identifier: Apache-2.0

#include <compare>
#include <stdexcept>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/required.h"

namespace mbo::types {
namespace {

using ::testing::Eq;

struct ThrowingValue {
  explicit ThrowingValue(int new_value) : value(new_value) {
    if (throw_on_construction) {
      throw std::runtime_error("requested construction failure");
    }
  }

  ThrowingValue(const ThrowingValue&) = delete;
  ThrowingValue& operator=(const ThrowingValue&) = delete;
  ThrowingValue(ThrowingValue&&) noexcept = default;
  ThrowingValue& operator=(ThrowingValue&&) = delete;
  ~ThrowingValue() = default;

  static inline bool throw_on_construction = false;
  int value;
};

template<typename Func>
bool ThrowsRuntimeError(Func&& func) {
  try {
    std::forward<Func>(func)();
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

static_assert(!noexcept(std::declval<Required<ThrowingValue>&>().emplace(1)));

struct ThrowingComparison {
  bool operator==(const ThrowingComparison&) const noexcept = default;

  std::strong_ordering operator<=>(const ThrowingComparison& /*other*/) const {
    throw std::runtime_error("requested comparison failure");
  }
};

static_assert(!noexcept(std::declval<const Required<ThrowingComparison>&>() <=> ThrowingComparison{}));
static_assert(!noexcept(
    std::declval<const Required<ThrowingComparison>&>() <=> std::declval<const Required<ThrowingComparison>&>()));

struct RequiredExceptionTest : ::testing::Test {};

TEST_F(RequiredExceptionTest, FailedEmplacePreservesValue) {
  Required<ThrowingValue> req(1);
  ThrowingValue::throw_on_construction = true;

  const bool caught = ThrowsRuntimeError([&req] { req.emplace(2); });

  ThrowingValue::throw_on_construction = false;
  EXPECT_THAT(caught, true);
  EXPECT_THAT(req->value, Eq(1));
  req.emplace(3);
  EXPECT_THAT(req->value, Eq(3));
}

TEST_F(RequiredExceptionTest, ComparisonPropagatesException) {
  const Required<ThrowingComparison> req;

  EXPECT_THAT(ThrowsRuntimeError([&req] { static_cast<void>(req <=> ThrowingComparison{}); }), true);
}

}  // namespace
}  // namespace mbo::types
