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

#ifndef MBO_STRINGS_INDENT_H_
#define MBO_STRINGS_INDENT_H_

#include <string_view>  // IWYU pragma: keep
#include <vector>

namespace mbo::strings {

// Converts a raw-string text block as if it had no indent.
//
// Performs these transitions:
// - removes the first line if empty (line break after 'R"(')
// - removes the indent of the second first line on all successive lines.
// - clears the last line if it has only whitespace.
//
// Whitespace: ' ' and '\t'.
std::string DropIndent(std::string_view text);

// Variant of `DropIndent` that returns the result as lines.
std::vector<std::string_view> DropIndentAndSplit(std::string_view text);

}  // namespace mbo::strings

#endif  // MBO_STRINGS_INDENT_H_
