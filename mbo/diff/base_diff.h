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

#ifndef MBO_DIFF_BASE_DIFF_H_
#define MBO_DIFF_BASE_DIFF_H_

#include <cstddef>
#include <string>

#include "mbo/diff/diff_options.h"
#include "mbo/diff/internal/data.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {

class BaseDiff {
 public:
  static std::string FileHeaders(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options);

  static std::string FileHeader(const file::Artefact& info, const DiffOptions& options);

  static std::string SelectFileHeader(
      const file::Artefact& either,
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const DiffOptions& options);

  BaseDiff() = delete;

  BaseDiff(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& opts)
      : options_(opts),
        header_(FileHeaders(lhs, rhs, options_)),
        lhs_data_(opts, opts.regex_replace_lhs, lhs.data),
        rhs_data_(opts, opts.regex_replace_rhs, rhs.data) {}

  ~BaseDiff() = default;
  BaseDiff(const BaseDiff&) = delete;
  BaseDiff& operator=(const BaseDiff&) = delete;
  BaseDiff(BaseDiff&&) = delete;
  BaseDiff& operator=(BaseDiff&&) = delete;

  bool CompareEq(std::size_t lhs, std::size_t rhs) const;

  const DiffOptions& Options() const noexcept { return options_; }

  const std::string& Header() const noexcept { return header_; }

 protected:
  diff_internal::Data& LhsData() noexcept { return lhs_data_; }

  const diff_internal::Data& LhsData() const noexcept { return lhs_data_; }

  diff_internal::Data& RhsData() noexcept { return rhs_data_; }

  const diff_internal::Data& RhsData() const noexcept { return rhs_data_; }

 private:
  const DiffOptions& options_;
  const std::string header_;
  diff_internal::Data lhs_data_;
  diff_internal::Data rhs_data_;
};

}  // namespace mbo::diff

#endif  // MBO_DIFF_BASE_DIFF_H_
