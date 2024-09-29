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

#include "mbo/types/stringify_ostream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo_other {  // Not using namespace mbo::types

namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::types::types_internal::kStructNameSupport;
using ::testing::Conditional;

struct StringifyOstreamTest : ::testing::Test {};

TEST_F(StringifyOstreamTest, OStream) {
  struct TestStruct {
    int one = 11;
    int two = 25;

    using MboTypesStringifySupport = void;
  };

  std::stringstream os;
  os << TestStruct{};
  EXPECT_THAT(os.str(), Conditional(kStructNameSupport, R"({.one: 11, .two: 25})", R"({11, 25})"));
}

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace
}  // namespace mbo_other

// Not using namespace mbo::types
