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

#include "mbo/types/stringify_ostream.h"

#include <ostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/types/extender.h"

namespace mbo_other {  // Not using namespace mbo::types

namespace {

// NOLINTBEGIN(*-magic-numbers,*-named-parameter)

using ::mbo::types::types_internal::kStructNameSupport;
using ::testing::Conditional;

struct StringifyOstreamTest : ::testing::Test {};

TEST_F(StringifyOstreamTest, OStream) {
  struct TestStruct {
    int one = 11;
    int two = 25;

    using MboTypesStringifySupport = void;
  };

  std::stringstream os;
  os << TestStruct{};
  EXPECT_THAT(os.str(), Conditional(kStructNameSupport, R"({.one: 11, .two: 25})", R"({11, 25})"));
}

TEST_F(StringifyOstreamTest, Nested) {
  struct TestSub {
    int sub = 77;
    // This does not need an type annotation, as it is being picked up automatically.
  };

  struct TestStruct {
    int one = 11;
    TestSub two;

    using MboTypesStringifySupport = void;
  };

  std::stringstream os;
  os << TestStruct{};
  EXPECT_THAT(os.str(), Conditional(kStructNameSupport, R"({.one: 11, .two: {.sub: 77}})", R"({11, {77}})"));
}

namespace existing_o_stream_operator {
struct TestSub {
  int sub = 77;
};

struct TestStruct {
  int one = 11;
  TestSub two;

  friend std::ostream& operator<<(std::ostream& os, const TestStruct& v) {
    return os << "TestStruct{one=" << v.one << "}";
  }

  using MboTypesStringifySupport = void;
};

TEST_F(StringifyOstreamTest, ExistingOStreamOperator) {
  std::stringstream os;
  os << TestStruct{};
  EXPECT_THAT(os.str(), R"(TestStruct{one=11})") << "  Note: No `TestSub` at all.";
}
}  // namespace existing_o_stream_operator

namespace existing_absl_stringify {
struct TestSub {
  int sub = 77;

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const TestSub& v) {
    absl::Format(&sink, "TestSub{sub=%d}", v.sub);
  }
};

struct TestStruct {
  int one = 11;
  TestSub two;

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const TestStruct& /*v*/) {
    sink.Append("NOT USED");
  }

  using MboTypesStringifySupport = void;
};

TEST_F(StringifyOstreamTest, ExistingAbslStringify) {
  std::stringstream os;
  os << TestStruct{};
  EXPECT_THAT(
      os.str(), Conditional(kStructNameSupport, R"({.one: 11, .two: TestSub{sub=77}})", R"({11, TestSub{sub=77}})"))
      << "  Note: AbslStringify in TestStruct ignored since Stringify has higher precendence.";
}
}  // namespace existing_absl_stringify

// NOLINTEND(*-magic-numbers,*-named-parameter)

}  // namespace
}  // namespace mbo_other

// Not using namespace mbo::types
