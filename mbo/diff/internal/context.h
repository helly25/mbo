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

#ifndef MBO_DIFF_INTERNAL_CONTEXT_H_
#define MBO_DIFF_INTERNAL_CONTEXT_H_

#include <cstddef>
#include <list>

#include "mbo/diff/diff_options.h"

namespace mbo::diff::diff_internal {

class Context final {
 public:
  Context() = delete;
  ~Context() = default;

  explicit Context(const DiffOptions& options) : options_(options) {}

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(Context&&) = delete;

  bool Empty() const noexcept { return data_.empty(); }

  bool HalfFull() const noexcept { return Full(true); }

  bool Full(bool half = false) const noexcept { return data_.size() >= (half ? Max() : 2 * Max()); }

  bool Push(std::string_view line, bool half = false) {
    if (Max() == 0) {
      return true;
    }
    while (Full(half)) {
      data_.pop_front();
    }
    data_.emplace_back(line);
    return Full(half);
  }

  std::string_view PopFront() noexcept {
    const std::string_view result = data_.front();
    data_.pop_front();
    return result;
  }

  std::size_t Max() const noexcept { return options_.context_size; }

  std::size_t Size() const noexcept { return data_.size(); }

  std::size_t HalfSsize() const noexcept { return HalfFull() ? Max() : data_.size(); }

  void Clear() noexcept { data_.clear(); }

 private:
  const DiffOptions& options_;
  std::list<std::string_view> data_;
};

}  // namespace mbo::diff::diff_internal

#endif  // MBO_DIFF_INTERNAL_CONTEXT_H_
