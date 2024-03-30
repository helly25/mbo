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

#include "mbo/file/ini/ini_file.h"

#include <filesystem>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/diff/unified_diff.h"
#include "mbo/file/file.h"
#include "mbo/testing/runfiles_dir.h"
#include "mbo/testing/status.h"

namespace mbo::file {
namespace {

using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;

namespace fs = std::filesystem;

struct IniFileTest : ::testing::Test {
  static std::string SrcDir(std::string_view src_rel) {
    return mbo::testing::RunfilesDirOrDie("com_helly25_mbo", src_rel);
  }

  static std::string TmpDir() { return ::testing::TempDir(); }
};

TEST_F(IniFileTest, TestGolden) {
  const std::string dir = SrcDir("mbo/file/ini/tests");
  std::map<std::string, std::pair<std::string, std::string>> tests;
  for (const auto& entry : fs::directory_iterator{dir}) {
    std::string_view filename = entry.path().native();
    if (entry.path().extension() == ".ini") {
      tests[entry.path().stem().native()].first = filename;
      continue;
    } else if (entry.path().extension() == ".golden") {
      tests[entry.path().stem().native()].second = filename;
      continue;
    }
  }
  for (const auto& [base, files] : tests) {
    const auto& [ini_fn, exp_fn] = files;
    SCOPED_TRACE(absl::StrCat("Test: ", base));
    ASSERT_FALSE(ini_fn.empty()) << "Missing '.ini' file for: " << base;
    ASSERT_FALSE(exp_fn.empty()) << "Missing '.golden' file for: " << base;
  }
  std::size_t file_count = 0;
  for (const auto& [base, files] : tests) {
    const auto& [ini_fn, exp_fn] = files;
    SCOPED_TRACE(absl::StrCat("Test: ", base));
    MBO_ASSERT_OK_AND_ASSIGN(const IniFile ini, IniFile::Read(ini_fn));
    const std::string dst_fn = JoinPaths(TmpDir(), absl::StrCat(base, ".ini"));
    EXPECT_OK(ini.Write(dst_fn));
    ++file_count;
    MBO_ASSERT_OK_AND_ASSIGN(const auto dst_artefact, Artefact::Read(dst_fn));
    MBO_ASSERT_OK_AND_ASSIGN(const auto exp_artefact, Artefact::Read(exp_fn));
    MBO_ASSERT_OK_AND_ASSIGN(const std::string diff, mbo::diff::UnifiedDiff::Diff(exp_artefact, dst_artefact));
    if (base == "empty") {
      EXPECT_THAT(ini, IsEmpty());
      EXPECT_THAT(ini, SizeIs(Eq(0)));
      EXPECT_THAT(dst_artefact.data, IsEmpty());
      EXPECT_THAT(exp_artefact.data, IsEmpty());
      EXPECT_THAT(diff, IsEmpty());
    } else {
      EXPECT_THAT(ini, Not(IsEmpty()));
      EXPECT_THAT(ini, SizeIs(Gt(0)));
      EXPECT_THAT(dst_artefact.data, Not(IsEmpty())) << dst_artefact.name;
      EXPECT_THAT(exp_artefact.data, Not(IsEmpty())) << exp_artefact.name;
      EXPECT_THAT(diff, IsEmpty()) << "\n" << diff;
    }
  }
  ASSERT_THAT(file_count, 2);
}

}  // namespace
}  // namespace mbo::file
