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

#ifndef MBO_DIFF_INTERNAL_CHUNK_H_
#define MBO_DIFF_INTERNAL_CHUNK_H_

#include <cstddef>
#include <list>
#include <string>
#include <vector>

#include "mbo/diff/diff_options.h"
#include "mbo/diff/internal/context.h"
#include "mbo/diff/internal/output.h"

namespace mbo::diff::diff_internal {

class Chunk {
 public:
  Chunk() = delete;
  ~Chunk() = default;

  Chunk(std::string header, const DiffOptions& options)
      : options_(options), output_(std::move(header)), context_(options) {}

  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&&) = delete;
  Chunk& operator=(Chunk&&) = delete;

  void PushBoth(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view ctx);
  void PushLhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view lhs);
  void PushRhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view rhs);
  void MoveDiffs();
  std::string MoveOutput();

 private:
  void CheckContext(std::size_t lhs_idx, std::size_t rhs_idx);

  void MoveContext(bool last);

  void OutputChunk();

  void Clear();

  const DiffOptions& options_;
  std::string output_;
  Context context_;
  std::vector<ChunkEntry> data_;
  std::list<std::string_view> lhs_;
  std::list<std::string_view> rhs_;
  std::size_t lhs_idx_ = 0;
  std::size_t rhs_idx_ = 0;
  std::size_t lhs_size_ = 0;
  std::size_t rhs_size_ = 0;
  bool diff_found_ = false;
  bool only_blank_lines_ = true;
  bool only_matching_lines_ = true;
};

}  // namespace mbo::diff::diff_internal

#endif  // MBO_DIFF_INTERNAL_CHUNK_H_
