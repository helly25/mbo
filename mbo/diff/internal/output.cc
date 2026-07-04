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

#include "mbo/diff/internal/output.h"

#include <cstddef>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "mbo/diff/diff_options.h"

namespace mbo::diff::diff_internal {
namespace {

// Unified range: 1-based `start,size`. A size of one drops the `,size` part.
// An empty range starts at the line preceding it (0 at file start): that is
// where `patch` applies the insertion.
std::string UnifiedPos(std::size_t idx, std::size_t size) {
  if (size == 0) {
    return absl::StrCat(idx, ",0");
  } else if (size == 1) {
    return absl::StrCat(idx + 1);
  } else {
    return absl::StrCat(idx + 1, ",", size);
  }
}

// Context/normal range: 1-based inclusive `first,last`. A range of one line
// shows just that line and an empty range shows the line before it (0 at file
// start), following GNU diff conventions.
std::string RangePos(std::size_t idx, std::size_t size) {
  const std::size_t first = idx + 1;
  const std::size_t last = idx + size;
  if (last <= first) {
    return absl::StrCat(last);
  }
  return absl::StrCat(first, ",", last);
}

void AppendUnified(
    std::string& output,
    const DiffOptions& options,
    const ChunkRange& range,
    const std::vector<ChunkEntry>& entries) {
  // Position and length:
  // - If there is no content, then line is 0, otherwise use next line,
  //   whether or not it has content.
  // - Do not show length 1.
  if (options.show_chunk_headers) {
    absl::StrAppendFormat(
        &output, "@@ -%s +%s @@\n",                 // Format
        UnifiedPos(range.lhs_idx, range.lhs_size),  // Format
        UnifiedPos(range.rhs_idx, range.rhs_size));
  }
  for (const ChunkEntry& entry : entries) {
    absl::StrAppendFormat(&output, "%c%s\n", entry.kind, entry.text);
  }
}

void AppendContext(
    std::string& output,
    const DiffOptions& options,
    const ChunkRange& range,
    const std::vector<ChunkEntry>& entries) {
  // Deletions directly followed by insertions form a change block whose lines
  // show as '!' on both sides; lone runs keep '-' respectively '+'.
  const std::size_t num = entries.size();
  std::vector<char> marks(num, ' ');
  bool lhs_changed = false;
  bool rhs_changed = false;
  for (std::size_t pos = 0; pos < num; ++pos) {
    switch (entries[pos].kind) {
      case ' ': break;
      case '-': {
        std::size_t end = pos;
        while (end < num && entries[end].kind == '-') {
          marks[end++] = '-';
        }
        lhs_changed = true;
        if (end < num && entries[end].kind == '+') {
          for (std::size_t idx = pos; idx < end; ++idx) {
            marks[idx] = '!';
          }
          while (end < num && entries[end].kind == '+') {
            marks[end++] = '!';
          }
          rhs_changed = true;
        }
        pos = end - 1;
        break;
      }
      default:  // '+' without preceding deletions.
        marks[pos] = '+';
        rhs_changed = true;
        break;
    }
  }
  if (options.show_chunk_headers) {
    absl::StrAppendFormat(&output, "***************\n*** %s ****\n", RangePos(range.lhs_idx, range.lhs_size));
  }
  if (lhs_changed) {
    for (std::size_t pos = 0; pos < num; ++pos) {
      if (entries[pos].kind != '+') {
        absl::StrAppendFormat(&output, "%c %s\n", marks[pos], entries[pos].text);
      }
    }
  }
  if (options.show_chunk_headers) {
    absl::StrAppendFormat(&output, "--- %s ----\n", RangePos(range.rhs_idx, range.rhs_size));
  }
  if (rhs_changed) {
    for (std::size_t pos = 0; pos < num; ++pos) {
      if (entries[pos].kind != '-') {
        absl::StrAppendFormat(&output, "%c %s\n", marks[pos], entries[pos].text);
      }
    }
  }
}

void AppendNormal(
    std::string& output,
    const DiffOptions& options,
    const ChunkRange& range,
    const std::vector<ChunkEntry>& entries) {
  // Normal format has no syntax for context lines: context entries only
  // advance the line counters. Every change block becomes one a/d/c command.
  std::size_t lhs_line = range.lhs_idx;
  std::size_t rhs_line = range.rhs_idx;
  const std::size_t num = entries.size();
  std::size_t pos = 0;
  while (pos < num) {
    if (entries[pos].kind == ' ') {
      ++lhs_line;
      ++rhs_line;
      ++pos;
      continue;
    }
    const std::size_t del_begin = pos;
    while (pos < num && entries[pos].kind == '-') {
      ++pos;
    }
    const std::size_t del_end = pos;
    while (pos < num && entries[pos].kind == '+') {
      ++pos;
    }
    const std::size_t add_end = pos;
    const std::size_t dels = del_end - del_begin;
    const std::size_t adds = add_end - del_end;
    if (options.show_chunk_headers) {
      const char letter = dels == 0 ? 'a' : adds == 0 ? 'd' : 'c';
      absl::StrAppendFormat(
          &output, "%s%c%s\n",       // Format
          RangePos(lhs_line, dels),  // Format
          letter,                    // Format
          RangePos(rhs_line, adds));
    }
    for (std::size_t idx = del_begin; idx < del_end; ++idx) {
      absl::StrAppendFormat(&output, "< %s\n", entries[idx].text);
    }
    if (dels > 0 && adds > 0) {
      absl::StrAppend(&output, "---\n");
    }
    for (std::size_t idx = del_end; idx < add_end; ++idx) {
      absl::StrAppendFormat(&output, "> %s\n", entries[idx].text);
    }
    lhs_line += dels;
    rhs_line += adds;
  }
}

}  // namespace

void AppendChunk(
    std::string& output,
    const DiffOptions& options,
    const ChunkRange& range,
    const std::vector<ChunkEntry>& entries) {
  switch (options.output_format) {
    case DiffOptions::OutputFormat::kUnified: AppendUnified(output, options, range, entries); return;
    case DiffOptions::OutputFormat::kContext: AppendContext(output, options, range, entries); return;
    case DiffOptions::OutputFormat::kNormal: AppendNormal(output, options, range, entries); return;
  }
}

}  // namespace mbo::diff::diff_internal
