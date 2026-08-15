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

#include "mbo/mope/mope.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mbo/mope/ini.h"

// Unit tests for the mope Template engine and its ini loader. The golden
// `*_diff_test`s exercise the binary end to end; these pin the LIBRARY contract:
// what SetValue/AddSection/Expand and ReadIniToTemlate do, one behaviour each.

namespace mbo::mope {
namespace {

using ::testing::IsFalse;
using ::testing::IsTrue;

namespace fs = std::filesystem;

struct MopeTest : ::testing::Test {};

TEST_F(MopeTest, ExpandReplacesASetValue) {
  Template tpl;
  ASSERT_THAT(tpl.SetValue("foo", "bar"), absl::OkStatus());
  std::string output = "x {{foo}} y";
  ASSERT_THAT(tpl.Expand(output), absl::OkStatus());
  EXPECT_THAT(output, "x bar y");
}

TEST_F(MopeTest, SetValueRefusesToOverwriteWithoutAllowUpdate) {
  Template tpl;
  ASSERT_THAT(tpl.SetValue("foo", "bar"), absl::OkStatus());
  EXPECT_THAT(tpl.SetValue("foo", "baz").ok(), IsFalse()) << "silent overwrite would hide a template bug";
  ASSERT_THAT(tpl.SetValue("foo", "baz", /*allow_update=*/true), absl::OkStatus());
  std::string output = "{{foo}}";
  ASSERT_THAT(tpl.Expand(output), absl::OkStatus());
  EXPECT_THAT(output, "baz");
}

TEST_F(MopeTest, InlineAssignmentSetsAndExpands) {
  // The template language's own assignment: `{{name=value}}` emits nothing and
  // defines `name` for the rest of the expansion (see tests/simply.mope).
  const Template tpl;
  std::string output = "{{v=x}}{{v}}";
  ASSERT_THAT(tpl.Expand(output), absl::OkStatus());
  EXPECT_THAT(output, "x");
}

TEST_F(MopeTest, IsValidNameRejectsWhatSetValueWouldChokeOn) {
  EXPECT_THAT(Template::IsValidName("foo"), IsTrue());
  EXPECT_THAT(Template::IsValidName("foo_bar"), IsTrue());
  EXPECT_THAT(Template::IsValidName(""), IsFalse());
  EXPECT_THAT(Template::IsValidName("a b"), IsFalse());
}

TEST_F(MopeTest, ReadIniToTemlateLoadsRootValues) {
  const fs::path ini_path = fs::temp_directory_path() / "mope_test_root.ini";
  {
    std::ofstream out(ini_path);
    out << "foo=bar\n";
  }
  Template tpl;
  ASSERT_THAT(ReadIniToTemlate(ini_path.string(), tpl), absl::OkStatus());
  std::string output = "{{foo}}";
  ASSERT_THAT(tpl.Expand(output), absl::OkStatus());
  EXPECT_THAT(output, "bar");
  fs::remove(ini_path);
}

TEST_F(MopeTest, ReadIniToTemlateFailsForAMissingFile) {
  Template tpl;
  EXPECT_THAT(ReadIniToTemlate("/nonexistent/no.ini", tpl).ok(), IsFalse());
}

}  // namespace
}  // namespace mbo::mope
