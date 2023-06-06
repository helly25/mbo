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

#include "mbo/diff/unified_diff.h"

#include <list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {
namespace {
size_t AbsDiff(size_t lhs, size_t rhs) {
  if (lhs > rhs) {
    return lhs - rhs;
  } else {
    return rhs - lhs;
  }
}
}  // namespace

namespace diff_internal {

class Data final {
 public:
  Data() = delete;
  ~Data() = default;

  explicit Data(std::string_view text)
      : got_nl_(absl::ConsumeSuffix(&text, "\n")),
        last_line_no_nl_(LastLineIfNoNewLine(text, got_nl_)),
        text_(SplitAndAdaptLastLine(text, got_nl_, last_line_no_nl_)) {}

  Data(const Data&) = delete;
  Data& operator=(const Data&) = delete;
  Data(Data&&) = delete;
  Data& operator=(Data&&) = delete;

  std::string_view Next() { return Done() ? "" : text_[idx_++]; }

  std::string_view Line() const { return Done() ? "" : text_[idx_]; }

  std::string_view Line(size_t ofs) const {
    ofs += idx_;
    return ofs >= Size() ? "" : text_[ofs];
  }

  size_t Idx() const { return idx_; }

  size_t Size() const { return text_.size(); }

  bool Done() const { return idx_ >= Size(); }

  bool Done(size_t ofs) const { return idx_ + ofs >= Size(); }

  bool GotNl() const { return got_nl_; }

 private:
  static std::string LastLineIfNoNewLine(std::string_view text, bool got_nl) {
    if (got_nl) {
      return "";
    }
    auto pos = text.rfind('\n');
    if (pos == std::string_view::npos) {
      pos = 0;
    } else {
      pos += 1;  // skip '\n'
    }
    return absl::StrCat(text.substr(pos), "\n\\ No newline at end of file");
  }

  static std::vector<std::string_view>
  SplitAndAdaptLastLine(std::string_view text, bool got_nl, std::string_view last_line) {
    if (!got_nl && text.empty()) {
      // This means a zero-length input (not just a single new-line).
      // For that case `diff -du` does not show 'No newline at end of file'.
      return {};
    }
    std::vector<std::string_view> result = absl::StrSplit(text, '\n');
    if (!got_nl) {
      if (result.empty()) {
        result.push_back(last_line);
      } else {
        result.back() = last_line;
      }
    }
    return result;
  }

  static std::string_view LastLine(std::string_view text) {
    auto pos = text.rfind('\n');
    if (pos == std::string_view::npos) {
      return text;
    }
    return text.substr(pos);
  }

  static std::vector<std::string_view>
  UpdateLast(std::vector<std::string_view> lines, bool got_nl, std::string_view last_line) {
    if (!got_nl) {
      lines.back() = last_line;
    }
    return lines;
  }

  const bool got_nl_;
  const std::string last_line_no_nl_;
  const std::vector<std::string_view> text_;
  size_t idx_ = 0;
};

class Context final {
 public:
  Context() = delete;
  ~Context() = default;

  explicit Context(const UnifiedDiff::Options& options) : options_(options) {}

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(Context&&) = delete;

  bool Empty() const { return data_.empty(); }

  bool HalfFull() const { return Full(true); }

  bool Full(bool half = false) const { return data_.size() >= (half ? Max() : 2 * Max()); }

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

  std::string_view PopFront() {
    const std::string_view result = data_.front();
    data_.pop_front();
    return result;
  }

  size_t Max() const { return options_.context_size; }

  size_t Size() const { return data_.size(); }

  size_t HalfSsize() const { return HalfFull() ? Max() : data_.size(); }

  void Clear() { data_.clear(); }

 private:
  const UnifiedDiff::Options& options_;
  std::list<std::string_view> data_;
};

class Chunk {
 public:
  Chunk() = delete;
  ~Chunk() = default;

  Chunk(const file::Artefact& lhs, const file::Artefact& rhs, const UnifiedDiff::Options& options)
      : lhs_empty_(lhs.text.empty()), rhs_empty_(rhs.text.empty()), context_(options) {
    absl::StrAppendFormat(
        &output_, "--- %s %s\n", InputName(lhs.name), absl::FormatTime(options.time_format, lhs.time, lhs.tz));
    absl::StrAppendFormat(
        &output_, "+++ %s %s\n", InputName(rhs.name), absl::FormatTime(options.time_format, rhs.time, rhs.tz));
  }

  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&&) = delete;
  Chunk& operator=(Chunk&&) = delete;

  void PushBoth(size_t lhs_idx, size_t rhs_idx, std::string_view ctx) {
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

  void PushLhs(size_t lhs_idx, size_t rhs_idx, std::string_view lhs) {
    CheckContext(lhs_idx, rhs_idx);
    LOG(INFO) << "Push L " << lhs_idx << ": <" << lhs << ">";
    lhs_.emplace_back(lhs);
    ++lhs_size_;
  }

  void PushRhs(size_t lhs_idx, size_t rhs_idx, std::string_view rhs) {
    CheckContext(lhs_idx, rhs_idx);
    LOG(INFO) << "Push R " << rhs_idx << ": <" << rhs << ">";
    rhs_.emplace_back(rhs);
    ++rhs_size_;
  }

  std::string MoveOutput() {
    OutputChunk();
    return std::move(output_);
  }

 private:
  static std::string_view InputName(std::string_view name) { return name.empty() ? "-" : name; }

  void CheckContext(size_t lhs_idx, size_t rhs_idx) {
    if (context_.Empty() && lhs_size_ == 0 && rhs_size_ == 0) {
      lhs_idx_ = lhs_idx;
      rhs_idx_ = rhs_idx;
    }
    MoveContext(false);
  }

  void MoveDiffs() {
    while (!lhs_.empty()) {
      data_.emplace_back('-', lhs_.front());
      lhs_.pop_front();
    }
    while (!rhs_.empty()) {
      data_.emplace_back('+', rhs_.front());
      rhs_.pop_front();
    }
  }

  void MoveContext(bool last) {
    LOG(INFO) << "Move Context: cut half";
    size_t ctx = last ? context_.HalfSsize() : context_.Size();
    while (ctx-- > 0) {
      data_.emplace_back(' ', context_.PopFront());
      ++lhs_size_;
      ++rhs_size_;
    }
    LOG(INFO) << "Move Context: done";
  }

  static std::string ChunkPos(bool empty, size_t idx, size_t size) {
    if (empty) {
      return "0,0";
    } else if (size == 1) {
      return absl::StrCat(idx + 1);
    } else {
      return absl::StrCat(idx + 1, ",", size);
    }
  }

  void OutputChunk() {
    if (lhs_size_ == 0 && rhs_size_ == 0) {
      return;
    }
    MoveContext(true);
    MoveDiffs();
    // Output position and length:
    // - If there is no content, then line is 0, otherwise use next line,
    //   whether ot not it has content.
    // - Do not show length 1
    absl::StrAppendFormat(
        &output_, "@@ -%s +%s @@\n",  //
        ChunkPos(lhs_empty_, lhs_idx_, lhs_size_), ChunkPos(rhs_empty_, rhs_idx_, rhs_size_));
    while (!data_.empty()) {
      absl::StrAppendFormat(&output_, "%c%s\n", data_.front().first, data_.front().second);
      data_.pop_front();
    }
    lhs_.clear();
    rhs_.clear();
    data_.clear();
    // Don't clear context, we may need the remaining context. Instead note the
    // new index locations.
    lhs_idx_ += lhs_size_;
    rhs_idx_ += rhs_size_;
    lhs_size_ = 0;
    rhs_size_ = 0;
  }

  const bool lhs_empty_ = false;
  const bool rhs_empty_ = false;
  std::string output_;
  Context context_;
  std::list<std::pair<char, std::string_view>> data_;
  std::list<std::string_view> lhs_;
  std::list<std::string_view> rhs_;
  size_t lhs_idx_ = 0;
  size_t rhs_idx_ = 0;
  size_t lhs_size_ = 0;
  size_t rhs_size_ = 0;
};

}  // namespace diff_internal

class UnifiedDiff::Impl {
 public:
  Impl() = delete;

  Impl(const file::Artefact& lhs, const file::Artefact& rhs, const Options& options)
      : lhs_data_(lhs.text), rhs_data_(rhs.text), chunk_(lhs, rhs, options) {}

  ~Impl() = default;
  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  absl::StatusOr<std::string> Compute() {
    Loop();
    return Finalize();
  }

 private:
  void Loop();
  void LoopBoth();
  bool FindNext();
  std::tuple<size_t, size_t, bool> FindNextRight();
  std::tuple<size_t, size_t, bool> FindNextLeft();
  bool PastMaxDiffChunkLength(size_t& loop);
  absl::StatusOr<std::string> Finalize();

  diff_internal::Data lhs_data_;
  diff_internal::Data rhs_data_;
  diff_internal::Chunk chunk_;
};

void UnifiedDiff::Impl::LoopBoth() {
  while (!lhs_data_.Done() && !rhs_data_.Done() && lhs_data_.Line() == rhs_data_.Line()) {
    chunk_.PushBoth(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Line());
    lhs_data_.Next();
    rhs_data_.Next();
  }
}

std::tuple<size_t, size_t, bool> UnifiedDiff::Impl::FindNextRight() {
  size_t lhs = 1;  // L+0 != R+0 -> start at lhs = 1, R1 = 0
  size_t rhs = 0;
  bool equal = false;
  while (!rhs_data_.Done(rhs)) {
    while (!lhs_data_.Done(lhs)) {
      if (lhs_data_.Line(lhs) == rhs_data_.Line(rhs)) {
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

std::tuple<size_t, size_t, bool> UnifiedDiff::Impl::FindNextLeft() {
  size_t lhs = 0;
  size_t rhs = 1;  // L+0 != R+0 -> start at L2 = 0, rhs = 1
  bool equal = false;
  while (!lhs_data_.Done(lhs)) {
    while (!rhs_data_.Done(rhs)) {
      if (lhs_data_.Line(lhs) == rhs_data_.Line(rhs)) {
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

bool UnifiedDiff::Impl::PastMaxDiffChunkLength(size_t& loop) {
  static constexpr size_t kMaxDiffChunkLength = 1'337;
  if (++loop > kMaxDiffChunkLength) {
    static constexpr std::string_view kMsg = "Maximum loop count reached";
    LOG(ERROR) << kMsg;
    chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), kMsg);
    return true;
  }
  return false;
}

bool UnifiedDiff::Impl::FindNext() {
  auto [lhs1, rhs1, eq1] = FindNextRight();
  auto [lhs2, rhs2, eq2] = FindNextLeft();
  if (eq1 && (!eq2 || AbsDiff(lhs1, rhs1) < AbsDiff(lhs2, rhs2))) {
    for (size_t i = 0; i < lhs1; ++i) {
      chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Next());
    }
    for (size_t i = 0; i < rhs1; ++i) {
      chunk_.PushRhs(lhs_data_.Idx(), rhs_data_.Idx(), rhs_data_.Next());
    }
    return true;
  } else if (eq2) {
    for (size_t i = 0; i < lhs2; ++i) {
      chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Next());
    }
    for (size_t i = 0; i < rhs2; ++i) {
      chunk_.PushRhs(lhs_data_.Idx(), rhs_data_.Idx(), rhs_data_.Next());
    }
    return true;
  } else {
    if (!lhs_data_.Done()) {
      chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Next());
    }
    if (!rhs_data_.Done()) {
      chunk_.PushRhs(lhs_data_.Idx(), rhs_data_.Idx(), rhs_data_.Next());
    }
  }
  return false;
}

void UnifiedDiff::Impl::Loop() {
  while (!lhs_data_.Done() && !rhs_data_.Done()) {
    LoopBoth();
    size_t loop = 0;
    while (!lhs_data_.Done() && !rhs_data_.Done() && !PastMaxDiffChunkLength(loop) && !FindNext()) {}
  }
}

absl::StatusOr<std::string> UnifiedDiff::Impl::Finalize() {
  while (!lhs_data_.Done()) {
    chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Next());
  }
  while (!rhs_data_.Done()) {
    chunk_.PushRhs(lhs_data_.Idx(), rhs_data_.Idx(), rhs_data_.Next());
  }
  return chunk_.MoveOutput();
}

absl::StatusOr<std::string>
UnifiedDiff::Diff(const file::Artefact& lhs, const file::Artefact& rhs, const Options& options) {
  if (lhs.text == rhs.text) {
    return std::string();
  }
  Impl diff(lhs, rhs, options);
  return diff.Compute();
}

}  // namespace mbo::diff
