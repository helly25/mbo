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

#include "mbo/testing/runfiles_dir.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/testing/status.h"

// Runs under bazel, so real runfiles exist: the test's own data dependency is the
// fixture. The one-argument label forms are the interesting surface - they parse
// '@workspace//path:file' apart before resolving.

namespace mbo::testing {
namespace {

using ::testing::IsTrue;

namespace fs = std::filesystem;

struct RunfilesDirTest : ::testing::Test {};

TEST_F(RunfilesDirTest, ResolvesAPlainPath) {
  MBO_ASSERT_OK_AND_ASSIGN(const std::string dir, RunfilesDir("mbo/testing/runfiles_dir.h"));
  EXPECT_THAT(fs::exists(dir), IsTrue()) << dir;
}

TEST_F(RunfilesDirTest, ResolvesAWorkspaceRootedLabel) {
  MBO_ASSERT_OK_AND_ASSIGN(const std::string dir, RunfilesDir("//mbo/testing:runfiles_dir.h"));
  EXPECT_THAT(fs::exists(dir), IsTrue()) << dir;
}

TEST_F(RunfilesDirTest, LabelColonBecomesASlash) {
  MBO_ASSERT_OK_AND_ASSIGN(const std::string by_label, RunfilesDir("//mbo/testing:runfiles_dir.h"));
  MBO_ASSERT_OK_AND_ASSIGN(const std::string by_path, RunfilesDir("mbo/testing/runfiles_dir.h"));
  EXPECT_THAT(by_label, by_path);
}

TEST_F(RunfilesDirTest, OrDieVariantAgreesWithTheStatusVariant) {
  MBO_ASSERT_OK_AND_ASSIGN(const std::string dir, RunfilesDir("mbo/testing/runfiles_dir.h"));
  EXPECT_THAT(RunfilesDirOrDie("mbo/testing/runfiles_dir.h"), dir);
}

TEST_F(RunfilesDirTest, TwoArgumentFormMatchesTheOneArgumentForm) {
  // NOTE: the header says the workspace argument is ignored when TEST_WORKSPACE is
  // set, but a bogus workspace does NOT resolve - the argument still participates
  // in the runfiles lookup. Pinned with the real workspace instead: the two forms
  // must agree.
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* workspace = std::getenv("TEST_WORKSPACE");
  ASSERT_THAT(workspace != nullptr, IsTrue()) << "bazel always sets TEST_WORKSPACE for tests";
  MBO_ASSERT_OK_AND_ASSIGN(const std::string two_arg, RunfilesDir(workspace, "mbo/testing/runfiles_dir.h"));
  MBO_ASSERT_OK_AND_ASSIGN(const std::string one_arg, RunfilesDir("mbo/testing/runfiles_dir.h"));
  EXPECT_THAT(two_arg, one_arg);
}

}  // namespace
}  // namespace mbo::testing
