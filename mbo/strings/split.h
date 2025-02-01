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

#ifndef MBO_STRINGS_SPLIT_H_
#define MBO_STRINGS_SPLIT_H_

#include <cstddef>
#include <string_view>

namespace mbo::strings {

// Modifier for `absl::StrSplit` that creates at most two parts separated by 'sep'.
class AtLast {
 public:
  explicit AtLast(char sep) : sep_(sep){};

  std::string_view Find(std::string_view text, std::size_t pos) const {
    std::size_t next_pos = text.substr(pos).rfind(sep_);
    if (next_pos == std::string_view::npos) {
      return std::string_view{text.data() + text.size(), 0};
    }
    return text.substr(next_pos, 1);
  }

 private:
  const char sep_;
};

}  // namespace mbo::strings

#endif  // MBO_STRINGS_SPLIT_H_
