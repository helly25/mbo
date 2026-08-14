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

#include "mbo/log/demangle.h"

#include <string>
#include <typeinfo>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::log {
namespace {

using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;

struct DemangleTest : ::testing::Test {};

struct SomeType {};

template<typename T>
struct SomeTemplate {};

TEST_F(DemangleTest, DemanglesAClassName) {
  // The mangled spelling is compiler specific, so the assertion is on the readable
  // name coming back out - not on any particular mangling.
  EXPECT_THAT(Demangle(typeid(SomeType).name()), HasSubstr("SomeType"));
}

TEST_F(DemangleTest, DemanglesATemplateName) {
  const std::string name = Demangle(typeid(SomeTemplate<int>).name());
  EXPECT_THAT(name, HasSubstr("SomeTemplate"));
  EXPECT_THAT(name, HasSubstr("int")) << "the template argument survives demangling";
}

TEST_F(DemangleTest, DemanglesAStdType) {
  EXPECT_THAT(Demangle(typeid(std::vector<int>).name()), HasSubstr("vector"));
}

TEST_F(DemangleTest, ReturnsSomethingForAnUndemanglableName) {
  // A name that is not a valid mangling must not crash and must not come back empty:
  // this runs in logging paths, where losing the string is worse than an ugly one.
  const std::string name = Demangle("not a mangled name at all");
  EXPECT_THAT(name, Not(IsEmpty()));
}

TEST_F(DemangleTest, DemangleVUsesTheValuesType) {
  const SomeType value;
  EXPECT_THAT(DemangleV(value), HasSubstr("SomeType"));

  const SomeTemplate<double> templated;
  EXPECT_THAT(DemangleV(templated), HasSubstr("SomeTemplate"));
}

TEST_F(DemangleTest, DemangleVAcceptsAnRvalue) {
  // The parameter is a forwarding reference, so a temporary has to work too.
  EXPECT_THAT(DemangleV(SomeType{}), HasSubstr("SomeType"));
}

}  // namespace
}  // namespace mbo::log
