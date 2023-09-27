// Copyright 2023 M. Boerger (helly25.com)
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

#include "mbo/container/convert_container.h"

#include <initializer_list>
#include <set>
#include <string>       // IWYU pragma: keep
#include <string_view>  // IWYU pragma: keep
#include <unordered_set>
#include <vector>

#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/traits.h"

namespace mbo::types {

using ::mbo::container::ConvertContainer;
using ::testing::ElementsAre;
using ::testing::WhenSorted;

static_assert(ContainerIsForwardIteratable<std::vector<std::string>>);
static_assert(ContainerIsForwardIteratable<const std::vector<std::string>>);
static_assert(ContainerIsForwardIteratable<const std::vector<std::string>&>);

static_assert(!ContainerIsForwardIteratable<std::initializer_list<std::string>>);
static_assert(IsForwardIteratable<std::initializer_list<std::string>>);

static_assert(ContainerCopyConvertible<std::vector<std::string_view>, std::set<std::string>>);
static_assert(ContainerCopyConvertible<const std::vector<std::string_view>, std::set<std::string>>);
static_assert(ContainerCopyConvertible<const std::vector<std::string_view>&, std::set<std::string>>);

template<typename ContainerIn, typename ContainerOut>
using FromTo = std::pair<ContainerIn, ContainerOut>;

template<typename FromToTypes>
struct ConvertContainerTest : ::testing::Test {};

TYPED_TEST_SUITE_P(ConvertContainerTest);

TYPED_TEST_P(ConvertContainerTest, StringTest) {
  using ContainerIn = TypeParam::first_type;
  using ContainerTo = TypeParam::second_type;
  const ContainerIn input = {"foo", "bar", "baz"};
  const ContainerTo converted = ConvertContainer(input);
  EXPECT_THAT(converted, WhenSorted(ElementsAre("bar", "baz", "foo")));
}

REGISTER_TYPED_TEST_SUITE_P(ConvertContainerTest, StringTest);

using TestTypes = ::testing::Types<
    FromTo<std::vector<std::string>, std::vector<std::string>>,
    FromTo<std::vector<std::string_view>, std::set<std::string>>,
    FromTo<std::vector<const char*>, std::set<std::string>>,
    FromTo<std::set<std::string_view>, std::vector<std::string>>,
    FromTo<std::initializer_list<std::string_view>, std::unordered_set<std::string>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Tests, ConvertContainerTest, TestTypes);

struct ConversionTest : public ::testing::Test {};

TEST_F(ConversionTest, SameTypeFunc) {
  EXPECT_THAT(
      std::set<int>(ConvertContainer(std::vector<int>{1, 2, 3}, [](int v) { return v * v; })),
      ElementsAre(1, 4, 9));
}

TEST_F(ConversionTest, ConversionFunc) {
  EXPECT_THAT(
      std::set<std::string>(ConvertContainer(std::vector<int>{1, 2, 3}, [](int v) { return absl::StrCat(v); })),
      ElementsAre("1", "2", "3"));
}

class MoveOnly final {
 public:
  ~MoveOnly() = default;
  MoveOnly() = delete;
  explicit MoveOnly(int v) : v_(v) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;

  int Value() const { return v_; }

 private:
  int v_;
};

TEST_F(ConversionTest, MoveOnlyConversion) {
  std::initializer_list<MoveOnly> values{MoveOnly(1), MoveOnly(2), MoveOnly(3)};
  auto conv = [](const MoveOnly&& v) -> std::string { return absl::StrCat(std::move(v).Value()); };
  EXPECT_THAT(
      std::set<std::string>(ConvertContainer(values, conv)),
      ElementsAre("1", "2", "3"));
}



}  // namespace mbo::types
