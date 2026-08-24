// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/json/json.h"

namespace mbo::json {
namespace {

struct ThrowingString {
  explicit operator std::string() const { throw std::runtime_error("requested string conversion failure"); }
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

static_assert(ConvertibleToJson<ThrowingString>);
static_assert(!noexcept(Json(ThrowingString{})));
static_assert(!noexcept(Json(std::string_view{})));
static_assert(!noexcept(std::declval<Json&>()[std::string_view{}]));        // NOLINT(*-avoid-unchecked-*)
static_assert(!noexcept(std::declval<const Json&>()[std::string_view{}]));  // NOLINT(*-avoid-unchecked-*)

struct JsonExceptionTest : ::testing::Test {};

TEST_F(JsonExceptionTest, ConstructionPropagatesStringConversionFailure) {
  EXPECT_THAT(ThrowsRuntimeError([] { static_cast<void>(Json(ThrowingString{})); }), true);
}

}  // namespace
}  // namespace mbo::json
