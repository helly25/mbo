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

#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/status/status.h"
#include "absl/strings/str_split.h"
#include "mbo/file/artefact.h"
#include "mbo/mope/mope.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,abseil-no-namespace)
ABSL_FLAG(
    std::vector<std::string>,
    set,
    {},
    "Acomma separated list of `name=value` pairs, used to seed the "
    "template config.");

namespace mbo {

absl::Status Process(std::string_view input_name) {
  auto input = mbo::file::Artefact::Read(input_name);
  mbo::mope::Template mope_template;
  for (const auto& set_kv : absl::GetFlag(FLAGS_set)) {
    const std::pair<std::string_view, std::string_view> kv = absl::StrSplit(set_kv, '=');
    std::cerr << "Set '" << kv.first << "' = '" << kv.second << "'" << std::endl;
    mope_template.SetValue(kv.first, kv.second);
  }
  auto result = mope_template.Expand(input->data);
  if (!result.ok()) {
    return result;
  }
  std::cout << input->data;
  return absl::OkStatus();
}

}  // namespace mbo

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  if (args.size() != 2) {
    std::cerr << "Exactly 1 argument, the input filename is required.";
    return 1;
  }
  args.erase(args.begin());
  const auto result = mbo::Process(args[0]);
  if (result.ok()) {
    return 0;
  }
  std::cerr << result << std::endl;
  return 1;
}
