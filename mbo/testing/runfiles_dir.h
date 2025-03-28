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

#ifndef MBO_TESTING_RUNFILES_DIR_H_
#define MBO_TESTING_RUNFILES_DIR_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"

namespace mbo::testing {

// Returns bazel_tools's `RLocation`.
// If environment variable `TEST_WORKSPACE` is present then `workspace` will be ignored.
absl::StatusOr<std::string> RunfilesDir(std::string_view workspace, std::string_view source_rel);
std::string RunfilesDirOrDie(std::string_view workspace, std::string_view source_rel);

// The single parameter variants understand relative path, but also bazel labels.
// If a source starts with '@' then it is assumed to be an absolute label. The function will split
// the source at the first '//' to separate workspace and relative source.
// If a source starts with '//', then it is assumed to be curren workspace rooted source.
// In either case above the first ':' in the resulting source will be replaced with '/'.
// Otherwise the source is assumed to be a plain file path.
absl::StatusOr<std::string> RunfilesDir(std::string_view source);
std::string RunfilesDirOrDie(std::string_view source);

}  // namespace mbo::testing

#endif  // MBO_TESTING_RUNFILES_DIR_H_
