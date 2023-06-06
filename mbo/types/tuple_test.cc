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

#include "mbo/types/tuple.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/internal/test_types.h"

namespace mbo::types {
namespace {

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace types_internal::test_types;

class TupleTest : public ::testing::Test {};

template<typename T>
class GenTupleTest : public ::testing::Test {
 public:
  static constexpr size_t kDerivedFieldCount =
      static_cast<size_t>(T::kFieldCount);
  static constexpr size_t kBaseFieldCount =
      static_cast<size_t>(T::BaseType::kFieldCount);
};

TYPED_TEST_SUITE(GenTupleTest, AllConstructedTypes);

TEST_F(TupleTest, StructCtorArgCountMax) {
  EXPECT_THAT(types_internal::StructCtorArgCountMaxV<Empty>, 0);
  EXPECT_THAT(types_internal::StructCtorArgCountMaxV<Base1>, 1);
  EXPECT_THAT(types_internal::StructCtorArgCountMaxV<Base2>, 2);
  EXPECT_THAT(types_internal::StructCtorArgCountMaxV<Base3>, 3);
}

TYPED_TEST(GenTupleTest, StructCtorArgCountMax) {
  EXPECT_THAT(
      types_internal::StructCtorArgCountMaxV<TypeParam>,
      TestFixture::kDerivedFieldCount + 1)
      << "The field count sums all derived fields and counts each base as 1.";
}

TEST_F(TupleTest, AggregateHasNonEmptyBase) {
  EXPECT_THAT(types_internal::AggregateHasNonEmptyBase<Empty>, false);
  EXPECT_THAT(types_internal::AggregateHasNonEmptyBase<Base1>, false);
  EXPECT_THAT(types_internal::AggregateHasNonEmptyBase<Base2>, false);
}

TYPED_TEST(GenTupleTest, AggregateHasNonEmptyBase) {
  EXPECT_THAT(
      types_internal::AggregateHasNonEmptyBase<TypeParam>,
      TestFixture::kBaseFieldCount != 0);
}

TEST_F(TupleTest, StructToTupleNoDerived) {
  constexpr std::tuple<> kTest0 = StructToTuple(Empty{});
  EXPECT_THAT(kTest0, std::tuple());
  constexpr std::tuple<int> kTest1 = StructToTuple(Base1{1});
  EXPECT_THAT(kTest1, std::tuple(1));
  constexpr std::tuple<int, int> kTest2 = StructToTuple(Base2{1, 2});
  EXPECT_THAT(kTest2, std::tuple(1, 2));
  constexpr std::tuple<int, int, int> kTest3 = StructToTuple(Base3{1, 2, 3});
  EXPECT_THAT(kTest3, std::tuple(1, 2, 3));
}

TEST_F(TupleTest, Mixed) {
  struct Mixed {
    int a = 0;
    double b = 0.0;
    std::string c;
    std::string_view d;
  };

  const std::tuple<int, double, std::string, std::string_view> test0 =
      StructToTuple(Mixed{1, 2.2, "3", "4"});
  EXPECT_THAT(test0, std::tuple(1, 2.2, "3", "4"));
}

TEST_F(TupleTest, DerivedNoBaseFields) {
  {
    using Type = ConstructType<0, 0>;
    constexpr std::tuple<> kTest0 = StructToTuple(Type{{}});
    EXPECT_THAT(kTest0, std::tuple());
  }
  {
    using Type = ConstructType<1, 0>;
    constexpr std::tuple<int> kTest1 = StructToTuple(Type{{}, 1});
    EXPECT_THAT(kTest1, std::tuple(1));
  }
  {
    using Type = ConstructType<2, 0>;
    constexpr std::tuple<int, int> kTest2 = StructToTuple(Type{{}, 1, 2});
    EXPECT_THAT(kTest2, std::tuple(1, 2));
  }
}

TEST_F(TupleTest, MultipleBaseClasses) {
  {
    using Type = ConstructMultiType<0, 0, 0>;
    constexpr std::tuple<> kTest0 = StructToTuple(Type{{}, {}});
    EXPECT_THAT(kTest0, std::tuple());
  }
  {
    using Type = ConstructMultiType<1, 0, 0>;
    constexpr std::tuple<int> kTest1 = StructToTuple(Type{{}, {}, 1});
    EXPECT_THAT(kTest1, std::tuple(1));
  }
  {
    using Type = ConstructMultiType<2, 0, 0>;
    constexpr std::tuple<int, int> kTest2 = StructToTuple(Type{{}, {}, 1, 2});
    EXPECT_THAT(kTest2, std::tuple(1, 2));
  }
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
struct BaseWithCtor {
  BaseWithCtor() { ++counter; }

  static int counter;
};

int BaseWithCtor::counter = 0;

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

TEST_F(TupleTest, BaseConstructor) {
  struct Derived : BaseWithCtor {
    int a = -1;
  };

  const int before = BaseWithCtor::counter;
  constexpr int kTestNum = 22;
  const Derived derived{{}, kTestNum};
  ASSERT_THAT(derived.a, kTestNum) << "Init did not happen";
  ASSERT_THAT(derived.counter, before + 1) << "Base constructor not executed";
  const std::tuple<int> test = StructToTuple(derived);
  EXPECT_THAT(test, std::tuple(kTestNum));
}

}  // namespace
}  // namespace mbo::types
