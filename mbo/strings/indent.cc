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

#include "mbo/strings/indent.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"

namespace mbo::strings {

std::vector<std::string_view> DropIndentAndSplit(std::string_view text) {
  // If the first line is empty, simply remove it and the new first line will
  // determine the indent to remove. Otherwise the first line will be left alone
  // and the second line will determine the indent to remove.
  // On the last line we remove any indent unconditionally.
  const std::size_t start = absl::ConsumePrefix(&text, "\n") ? 0 : 1;

  std::vector<std::string_view> lines = absl::StrSplit(text, '\n');
  if (lines.size() > start) {
    std::size_t pos = lines[start].find_first_not_of(" \t");
    if (pos == std::string_view::npos) {
      pos = lines[start].size();
    }
    const std::string prefix(lines[start], 0, pos);
    // std::cout << "First(" << start << "): <" << lines[start] << ">\n";
    // std::cout << "Prefix: <" << prefix << ">\n";

    if (lines.back().find_first_not_of(" \t") == std::string::npos) {
      lines.back() = "";
    }
    for (std::size_t i = start; i < lines.size(); ++i) {
      absl::ConsumePrefix(&lines[i], prefix);
    }
  }
  return lines;
}

std::string DropIndent(std::string_view text) {
  if (text.empty() || text == "\n") {
    return std::string(text);
  }
  return absl::StrJoin(DropIndentAndSplit(text), "\n");
}

}  // namespace mbo::strings
