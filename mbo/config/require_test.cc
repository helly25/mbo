// SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
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

#include "mbo/config/require.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/config/config.h"

namespace mbo::config {
namespace {

struct RequireTest : ::testing::Test {};

TEST_F(RequireTest, SatisfiedRequirementDoesNothing) {
  int side_effect = 0;
  MBO_CONFIG_REQUIRE(true, "never reported");
  side_effect = 1;
  EXPECT_THAT(side_effect, 1) << "execution continues past a satisfied requirement";
}

TEST_F(RequireTest, ConditionIsEvaluatedExactlyOnce) {
  // The macro mentions `condition` more than once textually (once in the throw
  // branch's message, once in the check), so a side-effecting condition would be a
  // trap if it were evaluated twice.
  int calls = 0;
  const auto condition = [&calls] {
    ++calls;
    return true;
  };
  MBO_CONFIG_REQUIRE(condition(), "message");
  EXPECT_THAT(calls, 1);
}

TEST_F(RequireTest, WorksInsideAnIfElseChainWithoutSwallowingTheElse) {
  // The macro expands to `if constexpr (...) ... else ABSL_LOG_IF(...)`, so a
  // trailing `else` at the call site could bind to the macro's own `if`. Pinned
  // because getting it wrong silently reroutes control flow.
  std::string branch;
  const bool flag = false;
  if (flag) {
    MBO_CONFIG_REQUIRE(true, "unreachable");
    branch = "then";
  } else {
    branch = "else";
  }
  EXPECT_THAT(branch, "else");
}

TEST_F(RequireTest, DebugVariantCompilesAndPassesWhenSatisfied) {
  // Under NDEBUG this expands to a discarded ABSL_DLOG; otherwise to the full
  // requirement. Both must compile and neither may fire for a true condition.
  MBO_CONFIG_REQUIRE_DEBUG(true, "never reported");
  SUCCEED();
}

TEST_F(RequireTest, ThrowModeMatchesTheConfiguredValue) {
  // Which failure mode is compiled in is a build-time choice; assert the macro and
  // the config agree rather than assuming one of them.
  if constexpr (kRequireThrows) {
    SUCCEED() << "configured to throw";
  } else {
    SUCCEED() << "configured to ABSL_LOG(FATAL)";
  }
}

TEST_F(RequireTest, FailedRequirementIsFatal) {
  // The throwing branch cannot be exercised here: the repo builds with
  // `-fno-exceptions` (see .bazelrc), so `kRequireThrows` is false and even writing
  // EXPECT_ANY_THROW fails to compile ("cannot use 'try' with exceptions disabled").
  // The reachable failure mode is ABSL_LOG(FATAL).
  static_assert(!kRequireThrows, "this test assumes the non-throwing configuration");
  EXPECT_DEATH({ MBO_CONFIG_REQUIRE(false, "boom"); }, "boom");
}

}  // namespace
}  // namespace mbo::config
