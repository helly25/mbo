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

#include "mbo/container/limited_vector.h"

#include "absl/log/initialize.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::container {
namespace {

// NOLINTBEGIN(*-magic-numbers)

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;

MATCHER_P(Capacity, cap, "Capacity is") {
  return arg.capacity() == static_cast<std::size_t>(cap);
}

struct LimitedVectorTest : ::testing::Test {
  static void SetUpTestSuite() { absl::InitializeLog(); }
};

TEST_F(LimitedVectorTest, MakeNoArg) {
  auto test = MakeLimitedVector<int>();
  EXPECT_THAT(test, IsEmpty());
  EXPECT_THAT(test, SizeIs(0));
  EXPECT_THAT(test, Capacity(0));
  EXPECT_THAT(test, ElementsAre());
}

TEST_F(LimitedVectorTest, MakeOneArg) {
  auto test1 = MakeLimitedVector(42);
  EXPECT_THAT(test1, Not(IsEmpty()));
  EXPECT_THAT(test1, SizeIs(1));
  EXPECT_THAT(test1, Capacity(1));
  EXPECT_THAT(test1, ElementsAre(42));
}

TEST_F(LimitedVectorTest, MakeInitArg) {
  auto test3 = MakeLimitedVector<3>({0, 1, 2});
  EXPECT_THAT(test3, Not(IsEmpty()));
  EXPECT_THAT(test3, SizeIs(3));
  EXPECT_THAT(test3, Capacity(3));
  EXPECT_THAT(test3, ElementsAre(0, 1, 2));
  auto test5 = MakeLimitedVector<5>({0, 1, 2});
  EXPECT_THAT(test5, Not(IsEmpty()));
  EXPECT_THAT(test5, SizeIs(3));
  EXPECT_THAT(test5, ElementsAre(0, 1, 2));
  test5.emplace_back(3);
  test5.push_back(4);
  EXPECT_THAT(test5, Not(IsEmpty()));
  EXPECT_THAT(test5, SizeIs(5));
  EXPECT_THAT(test5, Capacity(5));
  EXPECT_THAT(test5, ElementsAre(0, 1, 2, 3, 4));
}

TEST_F(LimitedVectorTest, MakeMultiArg) {
  auto test4 = MakeLimitedVector(0, 1, 2, 3);
  EXPECT_THAT(test4, Not(IsEmpty()));
  EXPECT_THAT(test4, SizeIs(4));
  EXPECT_THAT(test4, Capacity(4));
  EXPECT_THAT(test4, ElementsAre(0, 1, 2, 3));
}

TEST_F(LimitedVectorTest, MakeNumArg) {
  //auto test0 = MakeLimitedVector<0>(42);
  //EXPECT_THAT(test0, IsEmpty());
  //EXPECT_THAT(test0, SizeIs(0));
  //EXPECT_THAT(test0, ElementsAre());
  auto test1 = MakeLimitedVector<1>(42);
  EXPECT_THAT(test1, Not(IsEmpty()));
  EXPECT_THAT(test1, SizeIs(1));
  EXPECT_THAT(test1, Capacity(1));
  EXPECT_THAT(test1, ElementsAre(42));
  auto test2 = MakeLimitedVector<2>(42);
  EXPECT_THAT(test2, Not(IsEmpty()));
  EXPECT_THAT(test2, SizeIs(2));
  EXPECT_THAT(test2, Capacity(2));
  EXPECT_THAT(test2, ElementsAre(42, 42));
}

TEST_F(LimitedVectorTest, MakeIteratorArg) {
  std::vector<int> vec{0, 1, 2, 3};
  auto test = MakeLimitedVector<5>(vec.begin(), vec.end());
  EXPECT_THAT(test, Not(IsEmpty()));
  EXPECT_THAT(test, SizeIs(4));
  EXPECT_THAT(test, Capacity(5));
  EXPECT_THAT(test, ElementsAre(0, 1, 2, 3));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::container
