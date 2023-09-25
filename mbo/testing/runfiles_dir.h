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

#ifndef MBO_TESTING_RUNFILES_DIR_H_
#define MBO_TESTING_RUNFILES_DIR_H_

#include <string>

#include "absl/status/statusor.h"

namespace mbo::testing {

// Returns bazel_tools's `RLocation`.
absl::StatusOr<std::string> RunfilesDir();
std::string RunfilesDirOrDie();

}  // namespace mbo::testing

#endif // MBO_TESTING_RUNFILES_DIR_H_
