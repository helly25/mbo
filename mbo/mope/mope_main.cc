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

#include "absl/container/btree_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/status/status.h"
#include "absl/strings/str_split.h"
#include "mbo/file/artefact.h"
#include "mbo/file/file.h"
#include "mbo/mope/ini.h"
#include "mbo/mope/mope.h"
#include "mbo/status/status_macros.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,abseil-no-namespace)
ABSL_FLAG(std::string, template, "", "The templte input file (.tpl, .mope).");
ABSL_FLAG(std::string, generate, "-", "The generated out file ('-' for stdout)");
ABSL_FLAG(std::string, ini, "-", "An INI file that can be used to initialize section data.");

ABSL_FLAG(
    std::vector<std::string>,
    set,
    {},
    "A comma separated list of `name=value` pairs, used to seed the template config.");

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,abseil-no-namespace)

namespace mbo {

struct Options {
  std::string template_name;
  std::string generate_name;
};

absl::Status Process(const Options& opts) {
  auto input = mbo::file::Artefact::Read(opts.template_name);
  mbo::mope::Template mope_template;
  // Add `--set` flag values.
  for (const auto& set_kv : absl::GetFlag(FLAGS_set)) {
    const std::pair<std::string_view, std::string_view> kv = absl::StrSplit(set_kv, '=');
    MBO_STATUS_RETURN_IF_ERROR(mope_template.SetValue(kv.first, kv.second));
  }
  // Read `--ini` file if present.
  const std::string ini_filename = absl::GetFlag(FLAGS_ini);
  if (!ini_filename.empty()) {
    MBO_STATUS_RETURN_IF_ERROR(mope::ReadIniToTemlate(ini_filename, mope_template));
  }
  // Expand the template.
  MBO_STATUS_RETURN_IF_ERROR(mope_template.Expand(input->data));
  if (opts.generate_name.empty() || opts.generate_name == "-") {
    std::cout << input->data;
    return absl::OkStatus();
  }
  return mbo::file::SetContents(opts.generate_name, input->data);
}

}  // namespace mbo

static constexpr std::string_view kHelp = R"help(mope --template=<file.mope> [ --generate=<output> ] [ args ]

MOPE: Mope Over Pump Ends - Is a simple templating system.

Args:
  --template=<file.mope>  Path to the mope template.
  --generate=<file.out>   Path to output file. This defaults to '-'- which
                          results in using stdout as output.
  --set=(key=val,)+       List of comma separated `key`, `value` pairs used to
                          set simple values.

Background: Pump.py (Pretty Useful for Meta Programming) is a templating system
that allows to expand generic code mostly using simple for-loops and conditions.
Its drawback is that it is written in Python and that it does not support
structureed/hierarchical configuration. While moping over possible solutions,
the idea came up to implement just the necessary dynamic pices combined with a
structural templating system.

While more dynamic features might be added in the future, it is expressily not a
goal to become turing complete. There are many good choices available if that is
necessary.

MOPE understands single values and sections which are hierarchical dictionaries
that are made up of sections and values.

1) A single value is identified by: '{{' <name> '}}'. The value can be set by
calling `SetValue`.

```c++
mbo::mope::Templaye mope;
mope.SetValue("foo", "bar");
std::string output("My {{foo}}.");
mope.Expand(output);
CHECK_EQ(output, "My bar.");
```

2) A section dictionary can be build by calling `AddSubDictionary` multiple
times for the same `name` which becomes the section name. The section starts
with '{{#' <name> '}}' and ends with '{{/' <name> '}};.

```c++
mbo::mope::Templaye mope;
mope.AddSectionDictinary("section")->SetValue("foo", "bar");
mope.AddSectionDictinary("section")->SetValue("foo", "-");
mope.AddSectionDictinary("section")->SetValue("foo", "baz");
mope.AddSectionDictinary("other")->SetValue("many", "more");
std::string output("My {{#section}}{{foo}}{{/section}}.");
mope.Expand(output);
CHECK_EQ(output, "My bar-baz.");
```

3) The template supports for-loops:

'{{#'<name>'='<start>';'<end>(';'<step>(';'<join> )? )? '}}'...'{{/'<name>'}}'

* The values <start>, <end> and <step> can either be a number or a name of
  an existing section dictionary value.
* <step>:     Is the optional step-difference between iterations and defaults
              to 1. It canot be set to zero.
* <step> > 0: Iteration ends when the current value > <end>.
* <step> < 0: Iteration ends when the current value < <end>.
* <join>:     Optional value that functions as a joiner. The value can be a
              reference or a string in single (') or double quotes (").

This creates an automatic 'section' with a dynamic value under <name> which
can be accessed by '{{' <name> '}}'.

```c++
mbo::mope::Templaye mope;
mope.SetValue("max", "5");
std::string output("My {{#foo=1;max;2;"."}}{{foo}}{{/foor}}");
mope.Expand(output);
CHECK_EQ(output, "My 1.3.5.");
```

4) The template allows to set tags from within the template. This allows to
provide centralized configuration values for instance to for-loops.

These are value tags with a configuration: '{{' <<name> '=' <value> '}}'

```c++
mbo::mope::Templaye mope;
std::string output(R"(
   {{foo_start=1}}
   {{foo_end=8}}
   {{foo_step=2}}
   My {{#foo=foo_start;foo_end;foo_step;'.'}}{{foo}}{{/foor}})"R);
mope.Expand(output);
CHECK_EQ(output, "My 1.3.5.7.");
```

5) The template supports lists:

'{{#' <name> '=[' <values> '] (';' <join>)? }}'...'{{/'<name>'}}'

<values>:     Is a comma separated list which supports (limited, simple only)
              C++ escaping. In addition to the standard C++ escapes, the comma
              ',' curly braces '{', '}' and square braces '[', ']' can also be
              escaped to simplify template writing.
* <join>:     Optional value that functions as a joiner. The value can be a
              reference or a string in single (') or double quotes (").

```c++
mbo::mope::Templaye mope;
std::string output(R"({{#foo=['1,2',3\,4]}}[{{foo}}]{{/foo}})"R);
mope.Expand(output);
CHECK_EQ(output, "[1,2][3,4]");
```

)help";

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetProgramUsageMessage(kHelp);
  std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  if (args.size() != 1 || absl::GetFlag(FLAGS_template).empty()) {
    std::cerr << absl::ProgramUsageMessage();
    return 1;
  }
  args.erase(args.begin());
  const auto result = mbo::Process({
      .template_name = absl::GetFlag(FLAGS_template),
      .generate_name = absl::GetFlag(FLAGS_generate),
  });
  if (result.ok()) {
    return 0;
  }
  std::cerr << result << std::endl;
  return 1;
}
