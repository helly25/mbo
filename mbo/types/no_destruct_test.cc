#include "mbo/types/internal/decompose_count.h"
#include "mbo/types/no_destruct.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/extend.h"
#include "mbo/types/traits.h"

namespace mbo::types {

using ::mbo::types::Extend;
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
    const std::string kLibCppStdVer = std::to_string(_LIBCPP_STD_VER);
    EXPECT_THAT(kLibCppStdVer, Ge("20"));
}
#endif  // _LIBCPP_STD_VER

TEST_F(NoDestructTest, Test) {
    EXPECT_THAT(kTestSimple.Get().Print(), "{25, 42}");
    EXPECT_THAT((*kTestSimple).Print(), "{25, 42}");
    EXPECT_THAT(kTestSimple->Print(), "{25, 42}");
    EXPECT_THAT(kTestString.Get().Print(), R"({"25", "42"})");
    EXPECT_THAT((*kTestString).Print(), R"({"25", "42"})");
    EXPECT_THAT(kTestString->Print(), R"({"25", "42"})");
}

}  // namespace
}  // namespace mbo::types
