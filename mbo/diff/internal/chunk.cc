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

#include "mbo/diff/internal/chunk.h"

#include <cstddef>
#include <string>
#include <string_view>

#include "absl/cleanup/cleanup.h"
#include "mbo/diff/diff_options.h"
#include "mbo/diff/internal/output.h"
#include "re2/re2.h"

namespace mbo::diff::diff_internal {

void Chunk::PushBoth(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view ctx) {
  MoveDiffs();
  if (!data_.empty() && context_.Full()) {
    // We have a finished chunk_.
    // We could check whether the next 'context_size' lines are equal and
    // continue, but that is unnecessarily complex.
    OutputChunk();
  }
  if (lhs_size_ == 0 && rhs_size_ == 0) {
    if (context_.Empty()) {
      lhs_idx_ = lhs_idx;
      rhs_idx_ = rhs_idx;
    } else if (context_.HalfFull()) {
      ++lhs_idx_;
      ++rhs_idx_;
    }
  }
  context_.Push(ctx, lhs_size_ == 0 && rhs_size_ == 0);
}

void Chunk::PushLhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view lhs) {
  if (options_.skip_left_deletions) {
    return;
  }
  only_blank_lines_ &= lhs.empty();
  only_matching_lines_ &= options_.ignore_matching_lines && RE2::PartialMatch(lhs, *options_.ignore_matching_lines);
  CheckContext(lhs_idx, rhs_idx);
  lhs_.emplace_back(lhs);
  ++lhs_size_;
}

void Chunk::PushRhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view rhs) {
  only_blank_lines_ &= rhs.empty();
  only_matching_lines_ &= options_.ignore_matching_lines && RE2::PartialMatch(rhs, *options_.ignore_matching_lines);
  CheckContext(lhs_idx, rhs_idx);
  rhs_.emplace_back(rhs);
  ++rhs_size_;
}

std::string Chunk::MoveOutput() {
  OutputChunk();
  if (diff_found_) {
    return std::move(output_);
  } else {
    return "";  // Not showing chunk header.
  }
}

void Chunk::CheckContext(std::size_t lhs_idx, std::size_t rhs_idx) {
  if (context_.Empty() && lhs_size_ == 0 && rhs_size_ == 0) {
    lhs_idx_ = lhs_idx;
    rhs_idx_ = rhs_idx;
  }
  MoveContext(false);
}

void Chunk::MoveDiffs() {
  while (!lhs_.empty()) {
    data_.push_back({.kind = '-', .text = lhs_.front()});
    lhs_.pop_front();
  }
  while (!rhs_.empty()) {
    data_.push_back({.kind = '+', .text = rhs_.front()});
    rhs_.pop_front();
  }
}

void Chunk::MoveContext(bool last) {
  std::size_t ctx = last ? context_.HalfSsize() : context_.Size();
  while (ctx-- > 0) {
    data_.push_back({.kind = ' ', .text = context_.PopFront()});
    ++lhs_size_;
    ++rhs_size_;
  }
}

void Chunk::OutputChunk() {
  const absl::Cleanup clear = [this] { Clear(); };
  if (lhs_size_ == 0 && rhs_size_ == 0) {
    return;
  }
  MoveContext(true);
  MoveDiffs();
  if (only_blank_lines_ && options_.ignore_blank_lines) {
    only_matching_lines_ = true;
    return;
  }
  if (only_matching_lines_ && options_.ignore_matching_chunks && options_.ignore_matching_lines) {
    only_blank_lines_ = true;
    return;
  }
  diff_found_ = true;
  AppendChunk(
      output_, options_,
      {
          .lhs_idx = lhs_idx_,
          .rhs_idx = rhs_idx_,
          .lhs_size = lhs_size_,
          .rhs_size = rhs_size_,
      },
      data_);
}

void Chunk::Clear() {
  lhs_.clear();
  rhs_.clear();
  data_.clear();
  // Don't clear context, we may need the remaining context. Instead note the
  // new index locations.
  lhs_idx_ += lhs_size_;
  rhs_idx_ += rhs_size_;
  lhs_size_ = 0;
  rhs_size_ = 0;
  only_blank_lines_ = true;
  only_matching_lines_ = true;
}

}  // namespace mbo::diff::diff_internal
