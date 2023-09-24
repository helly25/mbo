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

#include "mbo/strings/strip.h"

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

namespace mbo::strings {

std::string_view StripLineComments(std::string_view line, StripCommentArgs args) {
  const auto pos = line.find(args.comment_start);
  if (pos != std::string_view::npos) {
    line = line.substr(0, pos);
    if (args.strip_trailing_whitespace) {
      line = absl::StripTrailingAsciiWhitespace(line);
    }
  }
  return line;
}

std::string StripComments(std::string_view input, StripCommentArgs args) {
  return absl::StrJoin(absl::StrSplit(input, '\n'), "\n", [&](std::string* out, std::string_view line) {
    absl::StrAppend(out, StripLineComments(line, args));
  });
}

absl::StatusOr<std::string> StripParsedLineComments(std::string_view line, StripParsedCommentArgs args) {
  auto line_or = ParseString(args.parse, line);
  if (!line_or.ok()) {
    return absl::InvalidArgumentError(absl::StrCat("Cannot parse input."));
  }
  line = *line_or;
  if (args.strip_trailing_whitespace) {
    line = absl::StripTrailingAsciiWhitespace(line);
  }
  return std::string(line);
}

absl::StatusOr<std::string> StripParsedComments(std::string_view input, StripParsedCommentArgs args) {
  absl::Status error = absl::OkStatus();
  std::string result_str =
      absl::StrJoin(absl::StrSplit(input, '\n'), "\n", [&](std::string* out, std::string_view line) {
        if (!error.ok()) {
          return;
        }
        auto line_or = StripParsedLineComments(line, args);
        if (!line_or.ok()) {
          error = line_or.status();
          return;
        }
        absl::StrAppend(out, *line_or);
      });
  if (error.ok()) {
    return result_str;
  } else {
    return error;
  }
}

}  // namespace mbo::strings
