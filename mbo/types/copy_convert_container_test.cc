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

#include "mbo/types/copy_convert_container.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/traits.h"

namespace mbo::types {

using ::mbo::types::CopyConvertContainer;
using ::testing::ElementsAre;
using ::testing::WhenSorted;

static_assert(ContainerIsForwardIteratable<std::vector<std::string>>);
static_assert(ContainerIsForwardIteratable<const std::vector<std::string>>);
static_assert(ContainerIsForwardIteratable<const std::vector<std::string>&>);
static_assert(ContainerCopyConvertible<std::vector<std::string_view>, std::set<std::string>>);
static_assert(ContainerCopyConvertible<const std::vector<std::string_view>, std::set<std::string>>);
static_assert(ContainerCopyConvertible<const std::vector<std::string_view>&, std::set<std::string>>);

template<typename ContainerIn, typename ContainerOut>
using FromTo = std::pair<ContainerIn, ContainerOut>;

template<typename FromToTypes>
struct CopyConvertContainerTest : ::testing::Test {};

TYPED_TEST_SUITE_P(CopyConvertContainerTest);

TYPED_TEST_P(CopyConvertContainerTest, StringTest) {
  using ContainerIn = TypeParam::first_type;
  using ContainerTo = TypeParam::second_type;
  const ContainerIn input = {"foo", "bar", "baz"};
  const ContainerTo converted = CopyConvertContainer(input);
  EXPECT_THAT(converted, WhenSorted(ElementsAre("bar", "baz", "foo")));
}

REGISTER_TYPED_TEST_SUITE_P(CopyConvertContainerTest, StringTest);

using TestTypes = ::testing::Types<
  FromTo<std::vector<std::string>, std::vector<std::string>>,
  FromTo<std::vector<std::string_view>, std::set<std::string>>,
  FromTo<std::vector<const char*>, std::set<std::string>>,
  FromTo<std::set<std::string_view>, std::vector<std::string>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Tests, CopyConvertContainerTest, TestTypes);

}  // namespace mbo::types
