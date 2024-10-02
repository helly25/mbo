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

#include "mbo/types/traits.h"

#include <type_traits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/internal/test_types.h"

namespace mbo::types {
namespace {

// NOLINTBEGIN(*-runtime-int)

using ::mbo::types::types_internal::AnyBaseType;
using ::mbo::types::types_internal::AnyType;
using ::testing::AnyOf;
using ::testing::Ne;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::mbo::types::types_internal::test_types;

struct TraitsTest : ::testing::Test {
  template<typename T>
  static std::string DecomposeInfo() {
    return absl::StrCat("  DecomposeInfo: {\n", mbo::types::types_internal::DecomposeInfo<T>::Debug("\n    "), "  \n}");
  }
};

template<IsSameAsAnyOfRaw<int, short> T>
constexpr bool Tester() noexcept {
  return 1;
}

template<NotSameAsAnyOfRaw<int, short> T>
constexpr bool Tester() noexcept {
  return 0;
}

TEST_F(TraitsTest, Concepts) {
  static_assert(Tester<int>());
  static_assert(Tester<short>());  // NOLINT(google-runtime-int)
  static_assert(!Tester<uint16_t>());
  static_assert(!Tester<unsigned>());
  static_assert(!Tester<void>());

  static_assert(!Tester<int*>());
  static_assert(Tester<int&>());
}

template<typename TestType>
struct GenTraitsTest : public ::testing::Test {
  static constexpr std::size_t kDerivedFieldCount = static_cast<std::size_t>(TestType::kFieldCount);
  static constexpr std::size_t kBaseFieldCount = static_cast<std::size_t>(TestType::BaseType::kFieldCount);
};

TYPED_TEST_SUITE(GenTraitsTest, AllConstructedTypes);

// NOLINTBEGIN(*-special-member-functions)
struct Virtual {
  virtual ~Virtual() = default;
};

// NOLINTEND(*-special-member-functions)

struct CtorDefault {
  CtorDefault() = default;
};

struct CtorUser {
  CtorUser(){};  // NOLINT(*-use-equals-default)
};

struct CtorBase : CtorUser {};

TEST_F(TraitsTest, DecomposeCountV) {
  ASSERT_THAT(NotDecomposableV, Ne(0));

  ASSERT_THAT(IsAggregate<void>, false);
  EXPECT_THAT(IsDecomposable<void>, false);

  ASSERT_THAT(IsAggregate<void>, false);
  EXPECT_THAT(IsDecomposable<void>, false);
  EXPECT_THAT(DecomposeCountV<void>, NotDecomposableV);

  ASSERT_THAT(IsAggregate<Virtual>, false);
  EXPECT_THAT(IsDecomposable<Virtual>, false);
  EXPECT_THAT(DecomposeCountV<Virtual>, NotDecomposableV);

  ASSERT_THAT(IsAggregate<CtorDefault>, false);
  EXPECT_THAT(IsDecomposable<CtorDefault>, false);
  EXPECT_THAT(DecomposeCountV<CtorDefault>, NotDecomposableV);

  ASSERT_THAT(IsAggregate<CtorUser>, false);
  EXPECT_THAT(IsDecomposable<CtorUser>, false);
  EXPECT_THAT(DecomposeCountV<CtorUser>, NotDecomposableV);

  ASSERT_THAT(types_internal::AggregateHasNonEmptyBase<CtorBase>, false);
  ASSERT_THAT(IsAggregate<CtorBase>, true);
  EXPECT_THAT(IsDecomposable<CtorBase>, false);
  EXPECT_THAT(DecomposeCountV<CtorBase>, NotDecomposableV);

  ASSERT_THAT(types_internal::AggregateHasNonEmptyBase<CtorBase>, false);
  EXPECT_THAT(IsDecomposable<Empty>, false);
  EXPECT_THAT(DecomposeCountV<Empty>, NotDecomposableV);

  ASSERT_THAT(types_internal::AggregateHasNonEmptyBase<CtorBase>, false);
  EXPECT_THAT(IsDecomposable<Base1>, true);
  EXPECT_THAT(DecomposeCountV<Base1>, 1);

  ASSERT_THAT(types_internal::AggregateHasNonEmptyBase<CtorBase>, false);
  EXPECT_THAT(IsDecomposable<Base2>, true);
  EXPECT_THAT(DecomposeCountV<Base2>, 2);
}

struct MadMix {
  int a = 0;
  std::string b;
  char c[5]{};  // NOLINT(*-magic-numbers,*-avoid-c-arrays)

  std::string Print() const {
    std::string_view str(static_cast<const char*>(&c[0]), sizeof(c));
    return absl::StrCat("{.a=", a, ", .b=\"", b, "\", .c={", str, "}}");
  }
};

TEST_F(TraitsTest, DecomposeMadMix) {
  ASSERT_THAT(IsAggregate<MadMix>, true);
  EXPECT_THAT(IsDecomposable<MadMix>, true);
#if CURRENTLY_UNSUPPORTED  // TODO(helly25): Currently does not work because DecomposeCount does not consider arrays
                           // correctly.
  EXPECT_THAT(DecomposeCountV<MadMix>, 3)
      << "  Note: With some restrictions the type can be xreated with up to 7 initializers.\n"
      << "        However C++ does not actually like it and the example results in an error:\n"
      << "        Bad example:\n"
      << "          MadMix{42, \"hallo\", '1', '2', '3', '4', '5'};\n"
      << "          error: suggest braces around initialization of subobject [-Werror,-Wmissing-braces]";
  EXPECT_THAT((MadMix{42, "hallo", {'1', '2', '3', '4', '5'}}).Print(), R"({.a=42, .b="hallo", .c={12345}})");
#endif
}

struct StructWithStrings {
  std::string_view one = "1";
  std::string_view two = "22";
  std::string_view three = "333";
  std::string_view four = "4444";
  std::string_view five;
};

TEST_F(TraitsTest, StructWithStrings) {
  using namespace ::mbo::types::types_internal;  // NOLINT(*-build-using-namespace)
  using T = StructWithStrings;
  EXPECT_THAT(DecomposeCountV<T>, 5) << DecomposeInfo<T>();
#ifndef MBO_TYPES_DECOMPOSE_COUNT_USE_OVERLOADSET
  EXPECT_THAT((AggregateFieldInitializerCount<T, 0>::value), 2);
  EXPECT_THAT((AggregateFieldInitializerCount<T, 1>::value), 2);
  EXPECT_THAT((AggregateFieldInitializerCount<T, 2>::value), 2);
  EXPECT_THAT((DetectSpecial<AggregateFieldInitializerCount<T, 0>::value, 5 - 0>::value), 2);
  EXPECT_THAT((DetectSpecial<AggregateFieldInitializerCount<T, 1>::value, 5 - 1>::value), 2);
#endif  // MBO_TYPES_DECOMPOSE_COUNT_USE_OVERLOADSET
}

TYPED_TEST(GenTraitsTest, DecomposeCountV) {
  using Type = TypeParam;
  constexpr std::size_t kBase = TestFixture::kBaseFieldCount;
  constexpr std::size_t kDerived = TestFixture::kDerivedFieldCount;
  EXPECT_THAT(
      DecomposeCountV<Type>, (kBase && kDerived) || (kBase + kDerived == 0) ? NotDecomposableV : kBase + kDerived);
}

TEST_F(TraitsTest, IsBracesContructible) {
  EXPECT_THAT((IsBracesConstructibleV<Empty>), true);
  EXPECT_THAT((IsBracesConstructibleV<Empty, AnyType>), false);

  EXPECT_THAT((IsBracesConstructibleV<Base1>), true) << "Default contruction";
  EXPECT_THAT((IsBracesConstructibleV<Base1, AnyType>), true);
  EXPECT_THAT((IsBracesConstructibleV<Base1, AnyBaseType<void>>), false);
  EXPECT_THAT((IsBracesConstructibleV<Base1, AnyBaseType<Base1>>), false);
  EXPECT_THAT((IsBracesConstructibleV<Base1, int>), true);
  EXPECT_THAT((IsBracesConstructibleV<Base1, void>), false);
  EXPECT_THAT((IsBracesConstructibleV<Base1, std::string>), false);
  EXPECT_THAT((IsBracesConstructibleV<Base1, int, void>), false);

  EXPECT_THAT((IsBracesConstructibleV<Base2>), true) << "Default contruction";
  EXPECT_THAT((IsBracesConstructibleV<Base2, AnyType>), true);
  EXPECT_THAT((IsBracesConstructibleV<Base2, int>), true);
  EXPECT_THAT((IsBracesConstructibleV<Base2, int, int>), true);
  EXPECT_THAT((IsBracesConstructibleV<Base2, std::string>), false);
  EXPECT_THAT((IsBracesConstructibleV<Base2, std::string, std::string>), false);
}

TEST_F(TraitsTest, IsBracesContructibleDerived) {
  static_assert(std::is_base_of_v<Empty, ConstructType<0, 0>>, "Bad type generation");
  EXPECT_THAT((IsBracesConstructibleV<ConstructType<0, 0>, Empty>), true);
}

TYPED_TEST(GenTraitsTest, IsAggregate) {
  EXPECT_THAT(std::is_aggregate_v<TypeParam>, true);
}

TYPED_TEST(GenTraitsTest, IsBracesContructibleGenerateDerived) {
  using Type = TypeParam;
  constexpr std::size_t kBase = TestFixture::kBaseFieldCount;
  constexpr std::size_t kDerived = TestFixture::kDerivedFieldCount;

  EXPECT_THAT(IsBracesConstructibleV<Type>, true);
  EXPECT_THAT((IsBracesConstructibleV<Type, void>), false);
  EXPECT_THAT((IsBracesConstructibleV<Type, int>), kBase >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, Empty>), kBase == 0);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base1>), kBase == 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base2>), kBase == 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base3>), kBase == 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType>), true);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>>), true);

  EXPECT_THAT((IsBracesConstructibleV<Type, Empty, int>), kBase == 0 && kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base1, int>), kBase == 1 && kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base2, int>), kBase == 2 && kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base3, int>), kBase == 3 && kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, int>), kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType>), kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, int>), kDerived >= 1);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType>), kDerived > 0);

  EXPECT_THAT((IsBracesConstructibleV<Type, Empty, int, int>), kBase == 0 && kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base1, int, int>), kBase == 1 && kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base2, int, int>), kBase == 2 && kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base3, int, int>), kBase == 3 && kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, int, int>), kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType, int>), kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType, AnyType>), kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, int, int>), kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType, int>), kDerived >= 2);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType, AnyType>), kDerived >= 2);

  EXPECT_THAT((IsBracesConstructibleV<Type, Empty, int, int, int>), kBase == 0 && kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base1, int, int, int>), kBase == 1 && kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base2, int, int, int>), kBase == 2 && kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, Base3, int, int, int>), kBase == 3 && kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, int, int, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType, int, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType, AnyType, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyType, AnyType, AnyType, AnyType>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, int, int, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType, int, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType, AnyType, int>), kDerived >= 3);
  EXPECT_THAT((IsBracesConstructibleV<Type, AnyBaseType<Type>, AnyType, AnyType, AnyType>), kDerived >= 3);
}

// NOLINTEND(*-runtime-int)

}  // namespace
}  // namespace mbo::types
