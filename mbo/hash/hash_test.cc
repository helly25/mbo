// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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

#include "mbo/hash/hash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

namespace mbo::hash::simple {
namespace {

using ::testing::Le;
using ::testing::Ne;

struct HashTest : ::testing::Test {};

TEST_F(HashTest, TestEmty) {
  constexpr std::size_t kHashEmpty = GetHash("");
  constexpr std::size_t kHashNull = GetHash(std::string_view());
  EXPECT_THAT(kHashEmpty, kHashNull);
  // Force using non constexpr `GetHash` by providing a non const string_view variable.
  const std::string_view sv_empty{""};  // NOLINT(*-redundant-string-init)
  const std::string_view sv_null{};
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
    // Force using non constexpr `GetHash` by providing a non const string_view variable.
    sv = kData[n];
    EXPECT_THAT(GetHash(sv), kHashes[n]) << "Using the non const `string_view sv` to compute a hash should result in "
                                            "the same value as the compile-time computed hash.";
    EXPECT_THAT(GetHash(sv), Ne(0));
    EXPECT_THAT(GetHash(sv), Ne(-1));
    EXPECT_THAT(GetHash(sv), Ne(~0ULL));
  }
}

class RandomStringGenerator {
 public:
  static inline constexpr std::size_t kDefaultMaxLen = 80;

  explicit RandomStringGenerator(int seed) : mt_(seed), char_dist_(0, kMaxChar) {}

  std::string GetRandomString(std::size_t max_len = kDefaultMaxLen) const {
    std::uniform_int_distribution<> str_len_dist(0, static_cast<int>(max_len));

    size_t length{0};
    while ((length = str_len_dist(mt_)) > max_len) {};
    std::string str;
    str.resize(length);
    for (size_t n = 0; n < length; ++n) {
      str[n] = static_cast<char>(char_dist_(mt_));
    }
    return str;
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  std::set<std::string> GetRandomStringSet(std::size_t num_strings, std::size_t max_len = kDefaultMaxLen) const {
    std::set<std::string> result;
    while (result.size() < num_strings) {
      result.insert(GetRandomString(max_len));
    }
    return result;
  }

 private:
  static inline constexpr int kMaxChar = 255;  // Could exclude 255 (invalid UTF-8).
  mutable std::mt19937 mt_;
  mutable std::uniform_int_distribution<int> char_dist_;
};

#undef SANITIZER
// Clang
#if defined(__has_feature)
# if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer)
#  define SANITIZER
# endif
// GCC
#elif defined(__SANITIZE_ADDRESS__)
# define SANITIZER
#endif

TEST_F(HashTest, Collision) {
  const RandomStringGenerator rsg(::testing::UnitTest::GetInstance()->random_seed());
#ifdef SANITIZER
  constexpr std::size_t kNumStrings = 200'000;
#else   // SANITIZER
  constexpr std::size_t kNumStrings = 2'000'000;
#endif  // SANITIZER
  const std::set<std::string> strings = rsg.GetRandomStringSet(kNumStrings, 30);
  std::set<uint64_t> hashes;
  for (const auto& str : strings) {
    hashes.insert(GetHash(str));
  }
  constexpr int kNumMaxCollisions = kNumStrings / 2'000'000;
  EXPECT_THAT(strings.size() - hashes.size(), Le(kNumMaxCollisions));
}

}  // namespace
}  // namespace mbo::hash::simple

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
