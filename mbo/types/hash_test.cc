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

#include "mbo/types/hash.h"

#include <array>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

namespace mbo::types {
namespace {

struct HashTest : ::testing::Test {};

TEST_F(HashTest, TestEmty) {
  constexpr std::size_t kHashEmpty = GetHash("");
  constexpr std::size_t kHashNull = GetHash(std::string_view());
  EXPECT_THAT(kHashEmpty, kHashNull);
  // By providing non const string_view variables to `GetHash` the non constexpr version will
  // be chosen.
  std::string_view sv_empty{""};  // NOLINT(*-redundant-string-init)
  std::string_view sv_null{};
  const std::size_t hash_empty = GetHash(sv_empty);
  const std::size_t hash_null = GetHash(sv_null);
  EXPECT_THAT(kHashEmpty, hash_empty);
  EXPECT_THAT(kHashEmpty, hash_null);
}

TEST_F(HashTest, Test) {
  constexpr std::array<std::string_view, 10> kData{
      "1", "12", "123", "1234", "12345", "123456", "1234567", "12345678", "123456789", "1234567890",
  };
  // Generate the constexpr hashes.
  constexpr std::array<uint64_t, kData.size()> kHashes{
      GetHash(kData[0]), GetHash(kData[1]), GetHash(kData[2]), GetHash(kData[3]), GetHash(kData[4]),
      GetHash(kData[5]), GetHash(kData[6]), GetHash(kData[7]), GetHash(kData[8]), GetHash(kData[9]),
  };
  std::string_view sv;
  for (std::size_t n = 0; n < kData.size(); ++n) {
    sv = kData[n];
    EXPECT_THAT(GetHash(sv), kHashes[n]) << "Using the non const `string_view sv` to compute a hash should result the "
                                            "same value as the compile-time computed hash.";
  }
}

}  // namespace
}  // namespace mbo::types

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
