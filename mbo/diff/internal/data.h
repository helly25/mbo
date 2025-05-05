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

#ifndef MBO_DIFF_INTERNAL_DATA_H_
#define MBO_DIFF_INTERNAL_DATA_H_

#include <cstddef>
#include <string>

#include "mbo/diff/diff_options.h"

namespace mbo::diff::diff_internal {

class Data {
 public:
  struct LineCache final {
    std::string_view line;
    std::string processed;
    bool matches_ignore = false;
  };

  Data() = delete;
  ~Data() = default;

  Data(
      const DiffOptions& options,
      const std::optional<DiffOptions::RegexReplace>& regex_replace,
      std::string_view text);

  Data(const Data&) = delete;
  Data& operator=(const Data&) = delete;
  Data(Data&&) = delete;
  Data& operator=(Data&&) = delete;

  std::string_view Next() noexcept { return Done() ? "" : text_[idx_++].line; }

  std::string_view Line() const noexcept { return Done() ? "" : text_[idx_].line; }

  const LineCache& GetCache(std::size_t ofs) const noexcept {
    ofs += idx_;
    ABSL_CHECK_LT(ofs, Size());
    return text_.at(ofs);
  }

  std::size_t Idx() const noexcept { return idx_; }

  std::size_t Size() const noexcept { return text_.size(); }

  bool Done() const noexcept { return idx_ >= Size(); }

  bool Done(std::size_t ofs) const noexcept { return idx_ + ofs >= Size(); }

 private:
  static std::string LastLineIfNoNewLine(std::string_view text, bool got_nl);

  static LineCache Process(
      const DiffOptions& options,
      const std::optional<DiffOptions::RegexReplace>& regex_replace,
      std::string_view line);

  static std::vector<LineCache> SplitAndAdaptLastLine(
      const DiffOptions& options,
      const std::optional<DiffOptions::RegexReplace>& regex_replace,
      std::string_view text,
      bool got_nl,
      std::string_view last_line);

  const DiffOptions& options_;
  const std::optional<DiffOptions::RegexReplace>& regex_replace_;
  const bool got_nl_ = true;
  const std::string last_line_no_nl_;
  const std::vector<LineCache> text_;
  std::size_t idx_ = 0;
};

}  // namespace mbo::diff::diff_internal

#endif  // MBO_DIFF_INTERNAL_DATA_H_
