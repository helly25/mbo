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

#include "mbo/types/internal/traits.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace mbo::types::types_internal {
namespace {

struct TraitsInternalTest : ::testing::Test {};

struct Empty {};

struct EmptyDerived : Empty {};

struct WithData {
  int value;
};

struct WithPrivate {
  // The field is deliberately never read: its presence alone is what makes the type
  // non-aggregate and non-empty, which is exactly what is being asserted.
  int Value() const { return value_; }

 private:
  int value_ = 0;
};

struct WithVirtual {
  WithVirtual() = default;
  WithVirtual(const WithVirtual&) = default;
  WithVirtual& operator=(const WithVirtual&) = default;
  WithVirtual(WithVirtual&&) noexcept = default;
  WithVirtual& operator=(WithVirtual&&) noexcept = default;
  virtual ~WithVirtual() = default;
};

struct WithConstructor {
  WithConstructor() {}  // NOLINT(*-use-equals-default): a user-provided ctor is the point.
};

TEST_F(TraitsInternalTest, IsAggregateFollowsTheLanguageRule) {
  static_assert(IsAggregate<Empty>);
  static_assert(IsAggregate<WithData>);
  static_assert(IsAggregate<int[3]>);  // NOLINT(*-avoid-c-arrays): arrays are aggregates.

  // A user-provided constructor, private data, or a virtual member each disqualify.
  static_assert(!IsAggregate<WithConstructor>);
  static_assert(!IsAggregate<WithPrivate>);
  static_assert(!IsAggregate<WithVirtual>);
  static_assert(!IsAggregate<int>);
  static_assert(!IsAggregate<std::string>);
  static_assert(!IsAggregate<std::vector<int>>);
}

TEST_F(TraitsInternalTest, IsEmptyTypeMeansNoStorage) {
  static_assert(IsEmptyType<Empty>);
  // Inheriting from an empty type keeps it empty - the base occupies no storage.
  static_assert(IsEmptyType<EmptyDerived>);
  // "Empty" is about storage, not about being an aggregate: a private-but-unused
  // member makes it non-empty.
  static_assert(!IsEmptyType<WithData>);
  static_assert(!IsEmptyType<WithPrivate>);
  // A virtual destructor adds a vtable pointer, so it is not empty either.
  static_assert(!IsEmptyType<WithVirtual>);
  static_assert(!IsEmptyType<int>);
  static_assert(!IsEmptyType<std::vector<int>>);
}

TEST_F(TraitsInternalTest, TheTwoConceptsAreIndependent) {
  // Empty and aggregate are orthogonal, which is why both exist.
  static_assert(IsAggregate<Empty> && IsEmptyType<Empty>);
  static_assert(IsAggregate<WithData> && !IsEmptyType<WithData>);
  static_assert(!IsAggregate<WithConstructor> && IsEmptyType<WithConstructor>);
  static_assert(!IsAggregate<WithVirtual> && !IsEmptyType<WithVirtual>);
}

}  // namespace
}  // namespace mbo::types::types_internal
