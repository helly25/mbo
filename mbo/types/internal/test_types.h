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

#ifndef MBO_TYPES_INTERNAL_TEST_TYPES_H_
#define MBO_TYPES_INTERNAL_TEST_TYPES_H_

#include "gtest/gtest.h"
#include "mbo/types/internal/cases.h"

namespace mbo::types::types_internal::test_types {

struct Empty {
  enum { kFieldCount = 0 };
};

struct Base1 {
  enum { kFieldCount = 1 };

  int a;
};

struct Base2 {
  enum { kFieldCount = 2 };

  int a;
  int b;
};

struct Base3 {
  enum { kFieldCount = 3 };

  int a;
  int b;
  int c;
};

struct BaseOutOfRange final {
  BaseOutOfRange() = delete;
};

template<std::size_t base>
using ConstructBase = typename CasesImpl<
    IfThen<base == 0, Empty>,
    IfThen<base == 1, Base1>,
    IfThen<base == 2, Base1>,
    IfThen<base == 3, Base1>,
    IfElse<BaseOutOfRange>>::type;

template<typename Base>
struct Derived0 : Base {
  using BaseType = Base;

  enum { kFieldCount = 0 };
};

template<typename Base>
struct Derived1 : Base {
  using BaseType = Base;

  enum { kFieldCount = 1 };

  int a;
};

template<typename Base>
struct Derived2 : Base {
  using BaseType = Base;

  enum { kFieldCount = 2 };

  int a;
  int b;
};

template<typename Base>
struct Derived3 : Base {
  using BaseType = Base;

  enum { kFieldCount = 3 };

  int a;
  int b;
  int c;
};

struct DerivedOutOfRange final {
  DerivedOutOfRange() = delete;
};

template<std::size_t derived, std::size_t base>
using ConstructType = typename CasesImpl<
    IfThen<derived == 0, Derived0<ConstructBase<base>>>,
    IfThen<derived == 1, Derived1<ConstructBase<base>>>,
    IfThen<derived == 2, Derived2<ConstructBase<base>>>,
    IfThen<derived == 3, Derived3<ConstructBase<base>>>,
    IfElse<DerivedOutOfRange>>::type;

using AllConstructedTypes = ::testing::Types<
    ConstructType<0, 0>,
    ConstructType<0, 1>,
    ConstructType<0, 2>,
    ConstructType<0, 3>,
    ConstructType<1, 0>,
    ConstructType<1, 1>,
    ConstructType<1, 2>,
    ConstructType<1, 3>,
    ConstructType<2, 0>,
    ConstructType<2, 1>,
    ConstructType<2, 2>,
    ConstructType<2, 3>,
    ConstructType<3, 0>,
    ConstructType<3, 1>,
    ConstructType<3, 2>,
    ConstructType<3, 3>>;


template<typename BaseA, typename BaseB>
struct Multi0
    : BaseA
    , BaseB {
  using BaseAType = BaseA;
  using BaseBType = BaseB;

  enum { kFieldCount = 0 };
};

template<typename BaseA, typename BaseB>
struct Multi1
    : BaseA
    , BaseB {
  using BaseAType = BaseA;
  using BaseBType = BaseB;

  enum { kFieldCount = 1 };

  int a;
};

template<typename BaseA, typename BaseB>
struct Multi2
    : BaseA
    , BaseB {
  using BaseAType = BaseA;
  using BaseBType = BaseB;

  enum { kFieldCount = 2 };

  int a;
  int b;
};

struct MultiOutOfRange final {
  MultiOutOfRange() = delete;
};

struct EmptyB {
  enum { kFieldCount = 0 };
};
struct Base1B {
  enum { kFieldCount = 1 };
  int b_a;
};
struct Base2B {
  enum { kFieldCount = 2 };
  int b_a;
  int b_b;
};
struct Base3B {
  enum { kFieldCount = 3 };
  int b_a;
  int b_b;
  int b_c;
};

template<std::size_t base>
using ConstructBase2 = typename CasesImpl<
    IfThen<base == 0, EmptyB>,
    IfThen<base == 1, Base2B>,
    IfThen<base == 2, Base2B>,
    IfThen<base == 3, Base3B>,
    IfElse<BaseOutOfRange>>::type;

template<std::size_t derived, std::size_t a, std::size_t b>
using ConstructMultiType = typename CasesImpl<
    IfThen<derived == 0, Multi0<ConstructBase<a>, ConstructBase2<b>>>,
    IfThen<derived == 1, Multi1<ConstructBase<a>, ConstructBase2<b>>>,
    IfThen<derived == 2, Multi2<ConstructBase<a>, ConstructBase2<b>>>,
    IfElse<MultiOutOfRange>>::type;

}  // namespace mbo::types::types_internal::test_types

#endif  // MBO_TYPES_INTERNAL_TEST_TYPES_H_
