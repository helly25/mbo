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

#include "mbo/types/no_destruct.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/extend.h"
#include "mbo/types/traits.h"

namespace mbo::types {

using ::mbo::types::Extend;
using ::mbo::types::types_internal::kStructNameSupport;
using ::testing::Conditional;
using ::testing::Ge;

// NOLINTBEGIN(*-magic-numbers)
struct TestSimple : Extend<TestSimple> {
    int a = 25;
    int b = 42;
};
// NOLINTEND(*-magic-numbers)

struct TestString : Extend<TestString> {
    std::string a = "25";
    std::string b = "42";
};

static_assert(IsAggregate<TestSimple>, "Must be aggregate");
static_assert(IsAggregate<TestString>, "Must be aggregate");

static_assert(DecomposeCountV<TestSimple> == 2, "There are 2 fields, no?");
static_assert(DecomposeCountV<TestString> == 2, "There are 2 fields, no?");

static const NoDestruct<TestSimple> kTestSimple;
static const NoDestruct<TestString> kTestString;

namespace {

class NoDestructTest : public ::testing::Test {};

#ifdef _LIBCPP_STD_VER
TEST_F(NoDestructTest, ClangCheck) {
    const std::string k_lib_cpp_std_ver = std::to_string(_LIBCPP_STD_VER);
    EXPECT_THAT(k_lib_cpp_std_ver, Ge("20"));
}
#endif  // _LIBCPP_STD_VER

TEST_F(NoDestructTest, Test) {
    const auto expected_simple = Conditional(kStructNameSupport, "{a: 25, b: 42}", "{25, 42}");
    const auto expected_string = Conditional(kStructNameSupport, R"({a: "25", b: "42"})", R"({"25", "42"})");
    EXPECT_THAT(kTestSimple.Get().ToString(), expected_simple);
    EXPECT_THAT((*kTestSimple).ToString(), expected_simple);
    EXPECT_THAT(kTestSimple->ToString(), expected_simple);
    EXPECT_THAT(kTestString.Get().ToString(), expected_string);
    EXPECT_THAT((*kTestString).ToString(), expected_string);
    EXPECT_THAT(kTestString->ToString(), expected_string);
}

}  // namespace
}  // namespace mbo::types
