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

#include "mbo/types/extend.h"

#include <concepts>  // IWYU pragma: keep
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>        // IWYU pragma: keep
#include <type_traits>  // IWYU pragma: keep

#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash_testing.h"
#include "absl/log/absl_log.h"  // IWYU pragma: keep
#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/extender.h"
#include "mbo/types/internal/decompose_count.h"
#include "mbo/types/internal/extend.h"  // IWYU pragma: keep
#include "mbo/types/traits.h"           // IWYU pragma: keep

namespace std {
template<typename Sink>
void AbslStringify(Sink& sink, const std::pair<const int, std::string>& val) {  // NOLINT(cert-dcl58-cpp)
  absl::Format(&sink, R"({%d, "%s"})", val.first, val.second);
}
}  // namespace std

namespace mbo::types {
namespace {

using ::mbo::types::extender::AbslHashable;
using ::mbo::types::extender::AbslStringify;
using ::mbo::types::extender::Comparable;
using ::mbo::types::extender::Printable;
using ::mbo::types::extender::Streamable;
using ::mbo::types::types_internal::kStructNameSupport;
using ::testing::AnyOf;
using ::testing::Conditional;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Ne;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::WhenSorted;

static_assert(std::same_as<::mbo::types::extender::AbslStringify, ::mbo::extender::AbslStringify>);
static_assert(std::same_as<::mbo::types::extender::Default, ::mbo::extender::Default>);

static_assert(::mbo::types_internal::ExtenderListValid<::mbo::extender::Default>);
static_assert(::mbo::types_internal::ExtenderListValid<AbslHashable, AbslStringify, Comparable, Printable, Streamable>);

struct Empty {};

struct Base1 {
  int a = 0;
};

struct Base2 {
  int a = 0;
  int b = 0;
};

struct Base3 {
  int a = 0;
  int b = 0;
  int c = 0;
};

struct Extend0 : public Extend<Extend0> {};

struct Extend1 : public Extend<Extend1> {
  int a = 0;
};

struct Extend2 : public Extend<Extend2> {
  int a = 0;
  int b = 0;
};

struct Extend4 : public Extend<Extend4> {
  int a = 0;
  int b = 0;
  std::string c;
  const int* ptr = nullptr;
};

struct SimpleName {
  std::string first;
  std::string last;
};

struct SimplePerson : Empty {
  SimpleName name;
  unsigned int age = 0;
};

struct Name : Extend<Name> {
  std::string first;
  std::string last;
};

struct Person : Extend<Person> {
  Name name;
  unsigned age = 0;
};

class ExtendTest : public ::testing::Test {};

#if !defined(__clang__)
TEST_F(ExtendTest, TestDecomposeInfo) {
  using ::mbo::types::types_internal::DecomposeInfo;
# define DEBUG_AND_TEST(Type, kExpected)                            \
   ABSL_LOG(INFO) << #Type << ": " << DecomposeInfo<Type>::Debug(); \
   EXPECT_THAT(DecomposeInfo<Type>::kDecomposeCount, kExpected)

  DEBUG_AND_TEST(Extend4, 4);
  DEBUG_AND_TEST(SimpleName, 2);
  DEBUG_AND_TEST(SimplePerson, 2);
  DEBUG_AND_TEST(Name, 2);
  DEBUG_AND_TEST(Person, 2);
# undef DEBUG_AND_TEST
}
#endif

#if defined(__clang__)
static_assert(kStructNameSupport);
#endif

TEST_F(ExtendTest, Test) {
  ASSERT_THAT(std::is_aggregate_v<Extend2>, true);
  ASSERT_THAT(std::is_empty_v<Extend2>, false);
  ASSERT_THAT(std::is_default_constructible_v<Extend2>, true);
  ASSERT_THAT(types_internal::AggregateHasNonEmptyBase<Extend2>, false);
  ASSERT_THAT(DecomposeCountV<Extend2>, 2);
}

TEST_F(ExtendTest, Print) {
  {
    const Extend2 ext2{.a = 25, .b = 42};
    ASSERT_THAT(DecomposeCountV<decltype(ext2)>, 2);
    EXPECT_THAT(ext2.ToString(), Conditional(kStructNameSupport, "{.a: 25, .b: 42}", "{25, 42}"));
  }

  {
    const Extend4 ext4{.a = 25, .b = 42, .c = "Hello There!"};
    ASSERT_THAT(DecomposeCountV<decltype(ext4)>, 4);
    EXPECT_THAT(
        ext4.ToString(), Conditional(
                             kStructNameSupport, R"({.a: 25, .b: 42, .c: "Hello There!", .ptr: <nullptr>})",
                             R"({25, 42, "Hello There!", <nullptr>})"));
  }
  {
    constexpr int kVal = 1'337;
    const Extend4 ext4{.a = 25, .b = 42, .c = "Hello There!", .ptr = &kVal};
    ASSERT_THAT(DecomposeCountV<decltype(ext4)>, 4);
    EXPECT_THAT(
        ext4.ToString(), Conditional(
                             kStructNameSupport, R"({.a: 25, .b: 42, .c: "Hello There!", .ptr: *{1337}})",
                             R"({25, 42, "Hello There!", *{1337}})"));
  }
}

TEST_F(ExtendTest, NestedPrint) {
  const Person person{.name = {.first = "First", .last = "Last"}, .age = 42};
  static constexpr std::string_view kExpected =
      kStructNameSupport ? R"({.name: {.first: "First", .last: "Last"}, .age: 42})" : R"({{"First", "Last"}, 42})";
  EXPECT_THAT(person.ToString(), kExpected);
  EXPECT_THAT(absl::StrFormat("%v", person), kExpected);
}

TEST_F(ExtendTest, Streamable) {
  const Extend4 ext4{.a = 25, .b = 42};
  std::ostringstream ss4;
  ss4 << ext4;
  EXPECT_THAT(
      ss4.str(),
      Conditional(kStructNameSupport, R"({.a: 25, .b: 42, .c: "", .ptr: <nullptr>})", R"({25, 42, "", <nullptr>})"));
}

#if defined(__clang__)

namespace debug {

// NOLINTBEGIN(bugprone-easily-swappable-parameters,cert-dcl50-cpp)
int DumpStructVisitor(
    std::size_t /*field_index*/,
    std::string_view format,
    std::string_view indent = {},
    std::string_view type = {},
    std::string_view name = {},
    ...) {
  std::cout << "Format: '" << format << "'";
  std::cout << ", Indent: '" << indent << "'";
  std::cout << ", Type: '" << type << "'";
  std::cout << ", Name: '" << name << "'";
  std::cout << '\n';
  return 0;
}

struct StructVisitorElement {
  std::string format;
  std::string indent;
  std::string type;
  std::string name;
  std::string line;
};

void PrintStructVisitor(
    std::size_t /*field_index*/,
    std::vector<StructVisitorElement>& fields,
    std::string& longest_type,
    bool print,
    std::string_view format,
    std::string_view indent = {},
    std::string_view type = {},
    std::string_view name = {},
    ...) {
  const std::string line = [&]() -> std::string {
    if (!format.starts_with('%')) {
      return std::string(format);
    } else if (format == "%s" || format == "%s}\n") {
      return absl::StrCat(indent, format.substr(2));
    } else if (format == "%s%s") {
      return absl::StrFormat("%s%s", indent.data(), type.data());
    } else if (format == "%s%s %s =") {
      return absl::StrFormat("%s%s %s =", indent.data(), type.data(), name.data());
    } else if (format.starts_with("%s%s %s =")) {
      return absl::StrFormat("%s%s %s =\n", indent.data(), type.data(), name.data());
    } else {
      return absl::StrFormat("Unknown format: '%s'", format);
    }
  }();
  if (print) {
    std::cout << line;
  }
  if (type.length() > longest_type.length()) {
    longest_type = type;
  }
  if (format.starts_with("%s%s %s =") && indent == "  ") {
    fields.emplace_back(StructVisitorElement{.format{format}, .indent{indent}, .type{type}, .name{name}, .line = line});
  }
}

// NOLINTEND(bugprone-easily-swappable-parameters,cert-dcl50-cpp)

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
template<typename T>
std::string Print(const T* ptr, bool print = false) {
  std::string longest_type;
  std::size_t field_index{0};
  std::vector<StructVisitorElement> fields;
  __builtin_dump_struct(ptr, &PrintStructVisitor, field_index, fields, longest_type, print);
  if (print) {
    field_index = 0;
    __builtin_dump_struct(ptr, &DumpStructVisitor, field_index);
  }
  return longest_type;
}

// NOLINTEND(cppcoreguidelines-pro-type-vararg,hicpp-vararg)

}  // namespace debug

#endif  // __clang__

struct PersonData : Extend<PersonData> {
  int index = 0;
  Person person;
  const std::set<std::string>* data = nullptr;
};

TEST_F(ExtendTest, StreamableComplexFields) {
  const std::set<std::string> data{"foo", "bar"};
  PersonData person{
      .index = 25,  // NOLINT(*-magic-numbers)
      .person =
          {
              .name =
                  {
                      .first = "Hugo",
                      .last = "Meyer",
                  },
              .age = 42,  // NOLINT(*-magic-numbers)
          },
      .data = &data,
  };
  std::ostringstream out;
  out << person;
  // NOTE: For Clang the top struct's field names are not present because the generated type names are far too long.
  EXPECT_THAT(
      out.str(),
      Conditional(
          kStructNameSupport,
          R"({.index: 25, .person: {.name: {.first: "Hugo", .last: "Meyer"}, .age: 42}, .data: *{{"bar", "foo"}}})",
          R"({25, {{"Hugo", "Meyer"}, 42}, *{{"bar", "foo"}}})"));
#ifdef __clang__
  std::cout << "Person:\n";
  EXPECT_THAT(debug::Print(&person), SizeIs(Le(201)));
  std::cout << "Person::person.name:\n";
  EXPECT_THAT(debug::Print(&person.person.name), SizeIs(Le(195)));
#endif  // __clang__
}

struct WithUnion : mbo::types::Extend<WithUnion> {
  union U {
    int first{2};
    int second;

    template<typename Sink>
    friend void AbslStringify(Sink& sink, const U& val) {
      absl::Format(&sink, "%d", val.first);  // NOLINT(cppcoreguidelines-pro-type-union-access)
    }
  };

  int first{1};
  U second;
  int third{3};
};

static_assert(!HasUnionMember<int>);

static_assert(types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<0>::value);
static_assert(types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<1>::value);
static_assert(types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<2>::value);
static_assert(types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<3>::value);
static_assert(types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<4>::value);
static_assert(!types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<5>::value);
static_assert(!types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<6>::value);
static_assert(!types_internal::AggregateInitializeTest<WithUnion>::IsInitializable<7>::value);
static_assert(types_internal::DecomposeInfo<WithUnion>::kInitializerCount == 4);
static_assert(types_internal::DecomposeInfo<WithUnion>::kFieldCount == 4);
static_assert(types_internal::DecomposeInfo<WithUnion>::kCountBases == 0);
static_assert(types_internal::DecomposeInfo<WithUnion>::kCountEmptyBases == 1);
static_assert(types_internal::DecomposeCountImpl<WithUnion>::value == 3);

static_assert(HasUnionMember<WithUnion>);

struct WithAnonymousUnion {  // Cannot extend due to anonymous union
  int first{1};

  union {
    int second{2};
    int third;
  };

  int fourth{3};
};

// cannot decompose class type '...WithAnonymousUnion' because it has an anonymous union member
//    auto& [a1, a2, a3] = data;
// static_assert(::mbo::types::HasUnionMember<WithAnonymousUnion>);

TEST_F(ExtendTest, StreamableWithUnion) {
  constexpr WithUnion kTest{
      .first = 25,
      .second{.second = 42},
      .third = 99,
  };

  EXPECT_THAT(kTest.ToString(), R"({25, 42, 99})");
}

struct SuppressFieldNames : ::mbo::types::Extend<SuppressFieldNames> {
  using MboExtendDoNotPrintFieldNames = void;

  int first{1};
  int second{2};
};

TEST_F(ExtendTest, SuppressFieldNames) {
  constexpr SuppressFieldNames kTest{
      .first = 25,
      .second = 42,
  };
  EXPECT_THAT(kTest.ToString(), R"({25, 42})");
}

TEST_F(ExtendTest, Comparable) {
  struct TestComparable : public ExtendNoDefault<TestComparable, Comparable> {
    int a = 0;
    int b = 0;
  };

  constexpr TestComparable kTest1{.a = 25, .b = 42};
  constexpr TestComparable kTest2{.a = 25, .b = 43};
  constexpr TestComparable kTest3{.a = 26, .b = 42};
  constexpr TestComparable kTest4{.a = 25, .b = 42};
  {
    EXPECT_THAT(kTest1 == kTest1, true);
    EXPECT_THAT(kTest1 != kTest1, false);
    EXPECT_THAT(kTest1 < kTest1, false);
    EXPECT_THAT(kTest1 <= kTest1, true);
    EXPECT_THAT(kTest1 > kTest1, false);
    EXPECT_THAT(kTest1 >= kTest1, true);
    EXPECT_THAT(kTest1, kTest1);  // sortcut for Eq, see below
    EXPECT_THAT(kTest1, Eq(kTest1));
    EXPECT_THAT(kTest1, Not(Lt(kTest1)));
    EXPECT_THAT(kTest1, Le(kTest1));
    EXPECT_THAT(kTest1, Not(Gt(kTest1)));
    EXPECT_THAT(kTest1, Ge(kTest1));
  }
  {
    EXPECT_THAT(kTest1 == kTest2, false);
    EXPECT_THAT(kTest1 != kTest2, true);
    EXPECT_THAT(kTest1 < kTest2, true);
    EXPECT_THAT(kTest1 <= kTest2, true);
    EXPECT_THAT(kTest1 > kTest2, false);
    EXPECT_THAT(kTest1 >= kTest2, false);
    EXPECT_THAT(kTest1, Not(kTest2));
    EXPECT_THAT(kTest1, Not(Eq(kTest2)));
    EXPECT_THAT(kTest1, Lt(kTest2));
    EXPECT_THAT(kTest1, Le(kTest2));
    EXPECT_THAT(kTest1, Not(Gt(kTest2)));
    EXPECT_THAT(kTest1, Not(Ge(kTest2)));
  }
  {
    EXPECT_THAT(kTest1 == kTest3, false);
    EXPECT_THAT(kTest1 != kTest3, true);
    EXPECT_THAT(kTest1 < kTest3, true);
    EXPECT_THAT(kTest1 <= kTest3, true);
    EXPECT_THAT(kTest1 > kTest3, false);
    EXPECT_THAT(kTest1 >= kTest3, false);
    EXPECT_THAT(kTest1, Not(kTest3));
    EXPECT_THAT(kTest1, Not(Eq(kTest3)));
    EXPECT_THAT(kTest1, Lt(kTest3));
    EXPECT_THAT(kTest1, Le(kTest3));
    EXPECT_THAT(kTest1, Not(Gt(kTest3)));
    EXPECT_THAT(kTest1, Not(Ge(kTest3)));
  }
  {
    EXPECT_THAT(kTest1 == kTest4, true);
    EXPECT_THAT(kTest1 != kTest4, false);
    EXPECT_THAT(kTest1 < kTest4, false);
    EXPECT_THAT(kTest1 <= kTest4, true);
    EXPECT_THAT(kTest1 > kTest4, false);
    EXPECT_THAT(kTest1 >= kTest4, true);
    EXPECT_THAT(kTest1, kTest4);  // sortcut for Eq, see below
    EXPECT_THAT(kTest1, Eq(kTest4));
    EXPECT_THAT(kTest1, Not(Lt(kTest4)));
    EXPECT_THAT(kTest1, Le(kTest4));
    EXPECT_THAT(kTest1, Not(Gt(kTest4)));
    EXPECT_THAT(kTest1, Ge(kTest4));
  }
  {
    EXPECT_THAT(kTest2 == kTest3, false);
    EXPECT_THAT(kTest2 != kTest3, true);
    EXPECT_THAT(kTest2 < kTest3, true);
    EXPECT_THAT(kTest2 <= kTest3, true);
    EXPECT_THAT(kTest2 > kTest3, false);
    EXPECT_THAT(kTest2 >= kTest3, false);
    EXPECT_THAT(kTest2, Not(kTest3));
    EXPECT_THAT(kTest2, Not(Eq(kTest3)));
    EXPECT_THAT(kTest2, Lt(kTest3));
    EXPECT_THAT(kTest2, Le(kTest3));
    EXPECT_THAT(kTest2, Not(Gt(kTest3)));
    EXPECT_THAT(kTest2, Not(Ge(kTest3)));
  }
  {
    EXPECT_THAT(kTest3 == kTest2, false);
    EXPECT_THAT(kTest3 != kTest2, true);
    EXPECT_THAT(kTest3 < kTest2, false);
    EXPECT_THAT(kTest3 <= kTest2, false);
    EXPECT_THAT(kTest3 > kTest2, true);
    EXPECT_THAT(kTest3 >= kTest2, true);
    EXPECT_THAT(kTest3, Not(kTest2));
    EXPECT_THAT(kTest3, Not(Eq(kTest2)));
    EXPECT_THAT(kTest3, Not(Lt(kTest2)));
    EXPECT_THAT(kTest3, Not(Le(kTest2)));
    EXPECT_THAT(kTest3, Gt(kTest2));
    EXPECT_THAT(kTest3, Ge(kTest2));
  }
}

struct PlainName {
  std::string first;
  std::string last;

  template<typename H>
  friend H AbslHashValue(H hash, const PlainName& obj) noexcept {
    return H::combine(std::move(hash), obj.first, obj.last);
  }
};

struct PlainPerson {
  PlainName name;
  std::size_t age = 0;

  template<typename H>
  friend H AbslHashValue(H hash, const PlainPerson& obj) noexcept {
    return H::combine(std::move(hash), obj.name, obj.age);
  }
};

TEST_F(ExtendTest, Hashable) {
  const Person person{.name = {.first = "First", .last = "Last"}, .age = 42};
  const PlainPerson plain_person{.name = {.first = "First", .last = "Last"}, .age = 42};

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({person, Person{}}));

  ASSERT_THAT((::mbo::types_internal::IsExtended<Name>), true);
  ASSERT_THAT((::mbo::types_internal::IsExtended<Person>), true);
  ASSERT_THAT((::mbo::types_internal::IsExtended<PlainName>), false);
  ASSERT_THAT((::mbo::types_internal::IsExtended<PlainPerson>), false);
  ASSERT_THAT(Person::RegisteredExtenderNames(), Contains("AbslHashable"));
  ASSERT_THAT(Person::RegisteredExtenderNames().size(), std::tuple_size_v<Person::RegisteredExtenders>);
  ASSERT_THAT((::mbo::types_internal::HasExtender<Person, AbslHashable>), true);

  EXPECT_THAT(absl::HashOf(person), Ne(0));
  EXPECT_THAT(absl::HashOf(person), absl::HashOf(plain_person));
  EXPECT_THAT(absl::HashOf(person), std::hash<Person>{}(person));

  struct NameNoDefault : ExtendNoDefault<NameNoDefault> {
    std::string first;
    std::string last;
  };

  ASSERT_THAT((::mbo::types_internal::IsExtended<NameNoDefault>), true);
  ASSERT_THAT((::mbo::types_internal::HasExtender<NameNoDefault, AbslHashable>), false);
}

TEST_F(ExtendTest, ExtenderNames) {
  struct T0 : ExtendNoDefault<T0> {};

  struct T1 : ExtendNoDefault<T1, AbslStringify, Printable> {};

  struct T2 : ExtendNoDefault<T2, AbslStringify, Streamable> {};

  struct T3a : ExtendNoDefault<T3a, AbslStringify, Comparable, Printable, Streamable> {};

  struct T3b : ExtendNoDefault<T3b, AbslStringify, Streamable, Printable, Comparable> {};

  struct T4 : Extend<T4> {};

  EXPECT_THAT(  // No defaults, no extras.
      T0::RegisteredExtenderNames(), IsEmpty());
  EXPECT_THAT(  // No default, only the specified extra.
      T1::RegisteredExtenderNames(), WhenSorted(ElementsAre("AbslStringify", "Printable")));
  EXPECT_THAT(  // No defaults, two extra.
      T2::RegisteredExtenderNames(), WhenSorted(ElementsAre("AbslStringify", "Streamable")));
  EXPECT_THAT(  // All defaults, no extra.
      T3a::RegisteredExtenderNames(),
      WhenSorted(ElementsAre("AbslStringify", "Comparable", "Printable", "Streamable")));
  EXPECT_THAT(  // All defaults, no extra.
      T3b::RegisteredExtenderNames(),
      WhenSorted(ElementsAre("AbslStringify", "Comparable", "Printable", "Streamable")));
  EXPECT_THAT(  // All defaults, and as extra.
      T4::RegisteredExtenderNames(),
      WhenSorted(ElementsAre("AbslHashable", "AbslStringify", "Comparable", "Default", "Printable", "Streamable")));
}

template<typename T>
struct Crtp {};  // Technically `Crtp` is not needed for the test, but we want to ensure this works with CRTP types.

struct Crtp1 {  //}: Crtp<Crtp1> {
  Crtp1() = delete;

  explicit Crtp1(int v) : value(v) {}

  int value{0};
};

struct Crtp2 {  //}: Crtp<Crtp2> {
  Crtp2() = delete;

  explicit Crtp2(int v) : value(v) {}

  int value{0};
};

struct UseCrtp1 : Extend<UseCrtp1> {
  Crtp1 crtp;
};

static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<0>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<1>::value);
static_assert(types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<2>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<3>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<4>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<5>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<6>::value);
static_assert(!types_internal::AggregateInitializeTest<UseCrtp1>::IsInitializable<7>::value);

static_assert(types_internal::AggregateInitializerCount<UseCrtp1>::value == 2);
static_assert(types_internal::DecomposeInfo<UseCrtp1>::kInitializerCount == 2);

static_assert(types_internal::DecomposeInfo<UseCrtp1>::kFieldCount == 2);
static_assert(types_internal::DecomposeInfo<UseCrtp1>::kDecomposeCount == 1);
static_assert(!types_internal::DecomposeInfo<UseCrtp1>::kBadFieldCount);
static_assert(types_internal::DecomposeInfo<UseCrtp1>::kIsAggregate);
static_assert(!types_internal::DecomposeInfo<UseCrtp1>::kIsEmpty);
static_assert(!types_internal::DecomposeInfo<UseCrtp1>::kOneNonEmptyBase);
static_assert(types_internal::DecomposeInfo<UseCrtp1>::kOnlyEmptyBases);
static_assert(!types_internal::DecomposeInfo<UseCrtp1>::kOneNonEmptyBasePlusFields);

static_assert(types_internal::DecomposeInfo<UseCrtp1>::kDecomposeCount == 1);

struct UseCrtp2 : Extend<UseCrtp2> {
  Crtp2 crtp;
};

struct UseBoth : Extend<UseBoth> {
  Crtp1 crtp1;
  Crtp2 crtp2;
};

static_assert(IsAggregate<UseBoth>);

static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<0>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<1>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<2>::value);
static_assert(types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<3>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<4>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<5>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<6>::value);
static_assert(!types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<7>::value);

static_assert(types_internal::IsAggregateInitializableWithNumArgs<UseBoth, 3>);
static_assert(types_internal::AggregateInitializeTest<UseBoth>::IsInitializable<3>::value);
static_assert(types_internal::AggregateInitializerCount<UseBoth>::value == 3);
static_assert(types_internal::DecomposeInfo<UseBoth>::kInitializerCount == 3);

static_assert(types_internal::DecomposeInfo<UseBoth>::kFieldCount == 3);
static_assert(!types_internal::DecomposeInfo<UseBoth>::kBadFieldCount);
static_assert(types_internal::DecomposeInfo<UseBoth>::kIsAggregate);
static_assert(!types_internal::DecomposeInfo<UseBoth>::kIsEmpty);
static_assert(!types_internal::DecomposeInfo<UseBoth>::kOneNonEmptyBase);
static_assert(types_internal::DecomposeInfo<UseBoth>::kOnlyEmptyBases);
static_assert(!types_internal::DecomposeInfo<UseBoth>::kOneNonEmptyBasePlusFields);

static_assert(types_internal::DecomposeInfo<UseBoth>::kDecomposeCount == 2);

static_assert(!IsDecomposable<Crtp1>);
static_assert(!IsDecomposable<Crtp2>);
static_assert(IsDecomposable<UseCrtp1>);
static_assert(IsDecomposable<UseCrtp2>);
static_assert(IsDecomposable<UseBoth>);

TEST_F(ExtendTest, NoDefaultConstructor) {
  ABSL_LOG(ERROR) << types_internal::DecomposeInfo<UseBoth>::Debug();
}

struct AbslFlatHashMapUser : Extend<AbslFlatHashMapUser> {
  using MboExtendDoNotPrintFieldNames = void;
  absl::flat_hash_map<int, std::string> flat_hash_map;
};

TEST_F(ExtendTest, AbseilFlatHashMapMember) {
  const AbslFlatHashMapUser data = {
      .flat_hash_map = {{25, "25"}, {42, "42"}},
  };
  EXPECT_THAT(
      data.ToString(), AnyOf(EndsWith(R"({{25, "25"}, {42, "42"}}})"), EndsWith(R"({{42, "42"}, {25, "25"}}})")));
}

static_assert(DecomposeCountV<AbslFlatHashMapUser> == 1);

}  // namespace
}  // namespace mbo::types
