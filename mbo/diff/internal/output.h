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

#ifndef MBO_DIFF_INTERNAL_OUTPUT_H_
#define MBO_DIFF_INTERNAL_OUTPUT_H_

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "mbo/diff/diff_options.h"

namespace mbo::diff::diff_internal {

// One line of a finished chunk: `kind` is '-' (lhs only), '+' (rhs only) or
// ' ' (context, present on both sides).
struct ChunkEntry {
  char kind = ' ';
  std::string_view text;
};

// Line ranges of a finished chunk: `*_idx` is the 0-based index of the first
// line, `*_size` the number of lines on that side (context lines count on
// both sides).
struct ChunkRange {
  std::size_t lhs_idx = 0;
  std::size_t rhs_idx = 0;
  std::size_t lhs_size = 0;
  std::size_t rhs_size = 0;
};

// Renders `entries` as one chunk in `options.output_format` onto `output`.
void AppendChunk(
    std::string& output,
    const DiffOptions& options,
    const ChunkRange& range,
    const std::vector<ChunkEntry>& entries);

}  // namespace mbo::diff::diff_internal

#endif  // MBO_DIFF_INTERNAL_OUTPUT_H_
