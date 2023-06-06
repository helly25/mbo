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

#include "mbo/file/file.h"

#include <filesystem>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/status.h"

namespace mbo::file {
namespace {

namespace fs = ::std::filesystem;

using ::mbo::testing::IsOkAndHolds;
using ::mbo::testing::StatusIs;
using ::testing::HasSubstr;

struct FileTest : public ::testing::Test {
  static std::string TestDir() {
    const auto* const test =
        ::testing::UnitTest::GetInstance()->current_test_info();
    return JoinPaths(::testing::TempDir(), test->name());
  }

  FileTest() : tmp_dir(TestDir()) {}

  void SetUp() override { fs::create_directory(tmp_dir); }

  void TearDown() override { fs::remove_all(tmp_dir); }

  const fs::path tmp_dir;
};

TEST_F(FileTest, SetContents) {
  const fs::path tmp_file = JoinPaths(tmp_dir, "foo.txt");
  EXPECT_OK(SetContents(tmp_file.string(), "foo"));
}

TEST_F(FileTest, Readable) {
  EXPECT_THAT(
      Readable(tmp_dir), StatusIs(
                             absl::StatusCode::kFailedPrecondition,
                             HasSubstr("Cannot open directory for reading")));
  const fs::path tmp_file = JoinPaths(tmp_dir, "foo.txt");
  ASSERT_OK(SetContents(tmp_file, "foo"));
  EXPECT_OK(Readable(tmp_file.string()));
}

TEST_F(FileTest, SetAndGetContents) {
  const fs::path tmp_file = JoinPaths(tmp_dir, "foo.txt");
  EXPECT_OK(SetContents(tmp_file, "foo"));
  EXPECT_THAT(GetContents(tmp_file), IsOkAndHolds("foo"));
}

TEST_F(FileTest, IsAbsolutePath) {
  EXPECT_TRUE(IsAbsolutePath(tmp_dir));
}

TEST_F(FileTest, NormalizePath) {
  EXPECT_THAT(NormalizePath(""), "");
  EXPECT_THAT(NormalizePath("/"), "/");
  EXPECT_THAT(NormalizePath("//"), "/");
  EXPECT_THAT(NormalizePath("///"), "/");
  EXPECT_THAT(NormalizePath("////"), "/");
  EXPECT_THAT(NormalizePath("//a/"), "/a");
  EXPECT_THAT(NormalizePath("//a//b"), "/a/b");
  EXPECT_THAT(NormalizePath("//a//b/"), "/a/b");
  EXPECT_THAT(NormalizePath("//a//b//"), "/a/b");
  EXPECT_THAT(NormalizePath("//a////b"), "/a/b");
  EXPECT_THAT(NormalizePath("a/"), "a");
}

TEST_F(FileTest, JoinPath) {
  EXPECT_THAT(JoinPaths(""), "");
  EXPECT_THAT(JoinPaths("a"), "a");
  EXPECT_THAT(JoinPaths("", ""), "");
  EXPECT_THAT(JoinPaths("a", ""), "a");
  EXPECT_THAT(JoinPaths("", "b"), "b");
  EXPECT_THAT(JoinPaths("a", "b"), "a/b");
  EXPECT_THAT(JoinPaths("", "", ""), "");
  EXPECT_THAT(JoinPaths("a", "", ""), "a");
  EXPECT_THAT(JoinPaths("", "b", ""), "b");
  EXPECT_THAT(JoinPaths("a", "b", ""), "a/b");
  EXPECT_THAT(JoinPaths("", "", "c"), "c");
  EXPECT_THAT(JoinPaths("a", "", "c"), "a/c");
  EXPECT_THAT(JoinPaths("a", "b", ""), "a/b");
  EXPECT_THAT(JoinPaths("a", "b", "c"), "a/b/c");
  EXPECT_THAT(JoinPaths("", "/", ""), "/");
  EXPECT_THAT(JoinPaths("", "/", "/"), "/");
  EXPECT_THAT(JoinPaths("", "/", "/", "a"), "/a");
  EXPECT_THAT(JoinPaths("a/"), "a");
}

}  // namespace
}  // namespace mbo::file
