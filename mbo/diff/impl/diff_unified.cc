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

#include "mbo/diff/impl/diff_unified.h"

#include <cstddef>
#include <string_view>
#include <tuple>

#include "absl/log/absl_log.h"
#include "mbo/diff/diff_options.h"

namespace mbo::diff {
namespace {
std::size_t AbsDiff(std::size_t lhs, std::size_t rhs) {
  if (lhs > rhs) {
    return lhs - rhs;
  } else {
    return rhs - lhs;
  }
}
}  // namespace

absl::StatusOr<std::string> DiffUnified::FileDiff(
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const DiffOptions& options) {
  if (lhs.data == rhs.data) {
    return std::string();
  }
  return DiffUnified(lhs, rhs, options).Compute();
}

void DiffUnified::LoopBoth() {
  while (More() && CompareEq(0, 0)) {
    PushEqual();
  }
}

std::tuple<std::size_t, std::size_t, bool> DiffUnified::FindNextRight() {
  std::size_t lhs = 1;  // L+0 != R+0 -> start at lhs = 1, R1 = 0
  std::size_t rhs = 0;
  bool equal = false;
  while (!RhsData().Done(rhs)) {
    while (!LhsData().Done(lhs)) {
      if (CompareEq(lhs, rhs)) {
        equal = true;
        break;
      }
      ++lhs;
    }
    if (equal) {
      break;
    }
    ++rhs;
    lhs = 0;  // Not 1
  }
  return {lhs, rhs, equal};
}

std::tuple<std::size_t, std::size_t, bool> DiffUnified::FindNextLeft() {
  std::size_t lhs = 0;
  std::size_t rhs = 1;  // L+0 != R+0 -> start at L2 = 0, rhs = 1
  bool equal = false;
  while (!LhsData().Done(lhs)) {
    while (!RhsData().Done(rhs)) {
      if (CompareEq(lhs, rhs)) {
        equal = true;
        break;
      }
      ++rhs;
    }
    if (equal) {
      break;
    }
    ++lhs;
    rhs = 0;  // Not 1
  }
  return {lhs, rhs, equal};
}

bool DiffUnified::PastMaxDiffChunkLength(std::size_t& loop) {
  if (++loop > Options().max_diff_chunk_length) {
    static constexpr std::string_view kMsg = "Maximum loop count reached";
    ABSL_LOG(ERROR) << kMsg;
    Chunk().PushLhs(LhsData().Idx(), RhsData().Idx(), kMsg);
    return true;
  }
  return false;
}

bool DiffUnified::FindNext() {
  auto [lhs1, rhs1, eq1] = FindNextRight();
  auto [lhs2, rhs2, eq2] = FindNextLeft();
  if (eq1 && (!eq2 || AbsDiff(lhs1, rhs1) < AbsDiff(lhs2, rhs2))) {
    for (std::size_t i = 0; i < lhs1; ++i) {
      PushLhs();
    }
    for (std::size_t i = 0; i < rhs1; ++i) {
      PushRhs();
    }
    return true;
  } else if (eq2) {
    for (std::size_t i = 0; i < lhs2; ++i) {
      PushLhs();
    }
    for (std::size_t i = 0; i < rhs2; ++i) {
      PushRhs();
    }
    return true;
  } else {
    if (!LhsData().Done()) {
      PushLhs();
    }
    if (!RhsData().Done()) {
      PushRhs();
    }
  }
  return false;
}

void DiffUnified::Loop() {
  while (More()) {
    LoopBoth();
    std::size_t loop = 0;
    while (More() && !PastMaxDiffChunkLength(loop) && !FindNext()) {}
  }
}

}  // namespace mbo::diff
