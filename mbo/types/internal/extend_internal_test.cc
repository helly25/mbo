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

#include <cstddef>
#include <type_traits>

#include "gtest/gtest.h"
#include "mbo/types/internal/extend.h"
#include "mbo/types/internal/extender.h"
#include "mbo/types/internal/test_types.h"

// Compile-level tests for the Extend machinery's internals and the shared
// test-type fixtures. Everything here is a static_assert; the TEST_F bodies
// only group them.

namespace mbo::types::types_internal {
namespace {

struct ExtendInternalTest : ::testing::Test {};

TEST_F(ExtendInternalTest, TestTypesDeclareTheirFieldCounts) {
  // The fixtures state their own decompose counts; the rest of the test suite
  // trusts these, so they must match the actual field lists.
  static_assert(test_types::Empty::kFieldCount == 0);
  static_assert(test_types::Base1::kFieldCount == 1);
  static_assert(test_types::Base2::kFieldCount == 2);
  static_assert(test_types::Base3::kFieldCount == 3);
  static_assert(std::is_aggregate_v<test_types::Base3>);
  static_assert(std::is_empty_v<test_types::Empty>);
}

TEST_F(ExtendInternalTest, BaseOutOfRangeIsUnconstructible) {
  static_assert(!std::is_default_constructible_v<test_types::BaseOutOfRange>);
}

TEST_F(ExtendInternalTest, ConstructBaseSelectsByCount) {
  static_assert(std::is_same_v<test_types::ConstructBase<0>, test_types::Empty>);
  static_assert(std::is_same_v<test_types::ConstructBase<1>, test_types::Base1>);
}

TEST_F(ExtendInternalTest, TypeInListMatchesMembership) {
  static_assert(TypeInList<int, char, int, double>);
  static_assert(!TypeInList<float, char, int, double>);
  static_assert(!TypeInList<int>);
}

TEST_F(ExtendInternalTest, IsExtenderRejectsPlainTypes) {
  static_assert(!IsExtender<int>);
  static_assert(!IsExtender<test_types::Base1>);
}

TEST_F(ExtendInternalTest, IsExtendedRejectsPlainTypes) {
  // From extender.h: only types built by Extend carry the marker requirements.
  static_assert(!IsExtended<int>);
  static_assert(!IsExtended<test_types::Base1>);
  static_assert(!IsExtended<test_types::Empty>);
}

}  // namespace
}  // namespace mbo::types::types_internal
