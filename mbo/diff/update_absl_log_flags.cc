// Copyright 2023 M.Boerger
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

#include <string>

#include "absl/flags/commandlineflag.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"

// NOLINTBEGIN(abseil-no-namespace)
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

ABSL_DECLARE_FLAG(int, stderrthreshold);
ABSL_DECLARE_FLAG(int, minloglevel);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTEND(abseil-no-namespace)

namespace mbo {

void UpdateAbslLogFlags() {
  // Unfortunately we cannot call `absl::InitializeLog()` as `IsIntialized()`
  // cannot be called to prevent duplicate initalization which triggers
  // `absl::log_internal::SetTimeZone() has already been called`.
  {
    absl::CommandLineFlag* flag = absl::FindCommandLineFlag("minloglevel");
    if (flag->CurrentValue() == flag->DefaultValue()) {
      absl::SetFlag(&FLAGS_minloglevel, 1);
    }
  }
  {
    absl::CommandLineFlag* flag = absl::FindCommandLineFlag("stderrthreshold");
    if (flag->CurrentValue() == flag->DefaultValue()) {
      absl::SetFlag(&FLAGS_stderrthreshold, 1);
    }
  }
}

}  // namespace mbo
