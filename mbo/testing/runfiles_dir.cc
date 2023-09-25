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

#include "mbo/testing/runfiles_dir.h"

#include <string>

#include "tools/cpp/runfiles/runfiles.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace mbo::testing {

absl::StatusOr<std::string> RunfilesDir() {
  std::string error;
  std::unique_ptr<bazel::tools::cpp::runfiles::Runfiles> runfiles(bazel::tools::cpp::runfiles::Runfiles::CreateForTest(&error));
  if (runfiles == nullptr) {
    return absl::NotFoundError("Could not determine runfiles directory.");
  }
  return runfiles->Rlocation("");
}

std::string RunfilesDirOrDie() {
  const auto runfiles_dir = RunfilesDir();
  ABSL_QCHECK_OK(runfiles_dir);
  return *runfiles_dir;
}

}  // namespace mbo::testing
