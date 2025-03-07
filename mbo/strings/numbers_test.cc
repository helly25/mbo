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

#include "mbo/strings/numbers.h"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mbo::strings {
namespace {

// NOLINTBEGIN(*-magic-numbers,modernize-use-designated-initializers)

struct NumbersTest : ::testing::Test {
  template<typename T>
  static void BigNumberTest(T v, unsigned expected_len, std::string_view expected_str) {
    SCOPED_TRACE(absl::StrFormat("Number: %d", v));
    const unsigned len = BigNumberLen(v);
    const std::string str = BigNumber(v);
    EXPECT_THAT(str, expected_str);
    EXPECT_THAT(str.size(), len) << "  Number: '" << str << "'";
    EXPECT_THAT(len, expected_len) << "  Number: '" << str << "'";
  }
};

template<typename T>
struct TestData {
  T value;
  unsigned expected_len{0};
  std::string_view expected_str;
};

TEST_F(NumbersTest, BigNumberMinMax) {
  BigNumberTest<char>(-128, 4, "-128");
  BigNumberTest<char>(127, 3, "127");
  BigNumberTest<unsigned char>(255, 3, "255");
  BigNumberTest<int16_t>(std::numeric_limits<int16_t>::min(), 7, "-32'768");
  BigNumberTest<int16_t>(std::numeric_limits<int16_t>::max(), 6, "32'767");
  BigNumberTest<uint16_t>(std::numeric_limits<uint16_t>::max(), 6, "65'535");
  BigNumberTest<int32_t>(std::numeric_limits<int32_t>::min(), 14, "-2'147'483'648");
  BigNumberTest<int32_t>(std::numeric_limits<int32_t>::max(), 13, "2'147'483'647");
  BigNumberTest<uint32_t>(std::numeric_limits<uint32_t>::max(), 13, "4'294'967'295");
  BigNumberTest<int64_t>(std::numeric_limits<int64_t>::min(), 26, "-9'223'372'036'854'775'808");
  BigNumberTest<int64_t>(std::numeric_limits<int64_t>::max(), 25, "9'223'372'036'854'775'807");
  BigNumberTest<uint64_t>(std::numeric_limits<uint64_t>::max(), 26, "18'446'744'073'709'551'615");
}

TEST_F(NumbersTest, BigNumberInt32) {
  constexpr std::array<TestData<int32_t>, 39> kTests{{
      {0, 1, "0"},
      {1, 1, "1"},
      {9, 1, "9"},
      {10, 2, "10"},
      {99, 2, "99"},
      {100, 3, "100"},
      {999, 3, "999"},
      {1'000, 5, "1'000"},
      {9'999, 5, "9'999"},
      {10'000, 6, "10'000"},
      {99'999, 6, "99'999"},
      {100'000, 7, "100'000"},
      {999'999, 7, "999'999"},
      {1'000'000, 9, "1'000'000"},
      {9'999'999, 9, "9'999'999"},
      {10'000'000, 10, "10'000'000"},
      {99'999'999, 10, "99'999'999"},
      {100'000'000, 11, "100'000'000"},
      {999'999'999, 11, "999'999'999"},
      {1'000'000'000, 13, "1'000'000'000"},
      {-1, 2, "-1"},
      {-9, 2, "-9"},
      {-10, 3, "-10"},
      {-99, 3, "-99"},
      {-100, 4, "-100"},
      {-999, 4, "-999"},
      {-1'000, 6, "-1'000"},
      {-9'999, 6, "-9'999"},
      {-10'000, 7, "-10'000"},
      {-99'999, 7, "-99'999"},
      {-100'000, 8, "-100'000"},
      {-999'999, 8, "-999'999"},
      {-1'000'000, 10, "-1'000'000"},
      {-9'999'999, 10, "-9'999'999"},
      {-10'000'000, 11, "-10'000'000"},
      {-99'999'999, 11, "-99'999'999"},
      {-100'000'000, 12, "-100'000'000"},
      {-999'999'999, 12, "-999'999'999"},
      {-1'000'000'000, 14, "-1'000'000'000"},
  }};
  for (const auto& [value, expected_len, expected_str] : kTests) {
    BigNumberTest(value, expected_len, expected_str);
  }
}

TEST_F(NumbersTest, BigNumberInt64) {
  constexpr std::array<TestData<int64_t>, 38> kTests{{
      {0, 1, "0"},
      {1, 1, "1"},
      {9, 1, "9"},
      {10, 2, "10"},
      {99, 2, "99"},
      {100, 3, "100"},
      {999, 3, "999"},
      {1'000, 5, "1'000"},
      {9'999, 5, "9'999"},
      {10'000, 6, "10'000"},
      {99'999, 6, "99'999"},
      {100'000, 7, "100'000"},
      {999'999, 7, "999'999"},
      {1'000'000, 9, "1'000'000"},
      {9'999'999, 9, "9'999'999"},
      {10'000'000, 10, "10'000'000"},
      {99'999'999, 10, "99'999'999"},
      {100'000'000, 11, "100'000'000"},
      {999'999'999, 11, "999'999'999"},
      {1'000'000'000, 13, "1'000'000'000"},
      {9'999'999'999ULL, 13, "9'999'999'999"},
      {10'000'000'000ULL, 14, "10'000'000'000"},
      {99'999'999'999ULL, 14, "99'999'999'999"},
      {100'000'000'000ULL, 15, "100'000'000'000"},
      {999'999'999'999ULL, 15, "999'999'999'999"},
      {1'000'000'000'000ULL, 17, "1'000'000'000'000"},
      {9'999'999'999'999ULL, 17, "9'999'999'999'999"},
      {10'000'000'000'000ULL, 18, "10'000'000'000'000"},
      {99'999'999'999'999ULL, 18, "99'999'999'999'999"},
      {100'000'000'000'000ULL, 19, "100'000'000'000'000"},
      {999'999'999'999'999ULL, 19, "999'999'999'999'999"},
      {1'000'000'000'000'000ULL, 21, "1'000'000'000'000'000"},
      {9'999'999'999'999'999ULL, 21, "9'999'999'999'999'999"},
      {10'000'000'000'000'000ULL, 22, "10'000'000'000'000'000"},
      {99'999'999'999'999'999ULL, 22, "99'999'999'999'999'999"},
      {100'000'000'000'000'000ULL, 23, "100'000'000'000'000'000"},
      {999'999'999'999'999'999ULL, 23, "999'999'999'999'999'999"},
      {1'000'000'000'000'000'000ULL, 25, "1'000'000'000'000'000'000"},
  }};
  for (const auto& [value, expected_len, expected_str] : kTests) {
    BigNumberTest(value, expected_len, expected_str);
  }
}

// NOLINTEND(*-magic-numbers,modernize-use-designated-initializers)

}  // namespace
}  // namespace mbo::strings
