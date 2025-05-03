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

#include "mbo/diff/unified_diff.h"

#include <cstddef>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "absl/time/time.h"
#include "mbo/file/artefact.h"
#include "mbo/strings/strip.h"
#include "mbo/types/no_destruct.h"
#include "re2/re2.h"

namespace mbo::diff {

std::optional<UnifiedDiff::RegexReplace> UnifiedDiff::ParseRegexReplaceFlag(std::string_view flag) {
  if (flag.empty()) {
    return std::nullopt;
  }
  const char separator = flag[0];
  std::vector<std::string> parts = absl::StrSplit(flag, separator);
  ABSL_CHECK_EQ(parts.size(), 4U);
  ABSL_CHECK(parts[0].empty());
  ABSL_CHECK(parts[3].empty());
  return UnifiedDiff::RegexReplace{
      .regex = std::make_unique<RE2>(parts[1]),
      .replace{parts[2]},
  };
}

namespace {
std::size_t AbsDiff(std::size_t lhs, std::size_t rhs) {
  if (lhs > rhs) {
    return lhs - rhs;
  } else {
    return rhs - lhs;
  }
}
}  // namespace

namespace diff_internal {

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
      const UnifiedDiff::Options& options,
      const std::optional<UnifiedDiff::RegexReplace>& regex_replace,
      std::string_view text)
      : options_(options),
        regex_replace_(regex_replace),
        got_nl_(absl::ConsumeSuffix(&text, "\n")),
        last_line_no_nl_(LastLineIfNoNewLine(text, got_nl_)),
        text_(SplitAndAdaptLastLine(options_, regex_replace_, text, got_nl_, last_line_no_nl_)) {}

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

  static LineCache Process(
      const UnifiedDiff::Options& options,
      const std::optional<UnifiedDiff::RegexReplace>& regex_replace,
      std::string_view line);

  static std::vector<LineCache> SplitAndAdaptLastLine(
      const UnifiedDiff::Options& options,
      const std::optional<UnifiedDiff::RegexReplace>& regex_replace,
      std::string_view text,
      bool got_nl,
      std::string_view last_line) {
    if (!got_nl && text.empty()) {
      // This means a zero-length input (not just a single new-line).
      // For that case `diff -du` does not show 'No newline at end of file'.
      return {};
    }
    const std::size_t count = std::count_if(text.begin(), text.end(), [](char chr) { return chr == '\n'; });
    std::vector<LineCache> result;
    result.reserve(count > 0 ? count : 1);
    for (std::string_view line : absl::StrSplit(text, '\n')) {
      result.push_back(Process(options, regex_replace, line));
    }
    if (!got_nl) {
      if (!result.empty()) {
        result.pop_back();
      }
      result.push_back({
          .line = last_line,
          .processed = std::string{last_line},
          .matches_ignore = false,
      });
    }
    return result;
  }

  const UnifiedDiff::Options& options_;
  const std::optional<UnifiedDiff::RegexReplace>& regex_replace_;
  const bool got_nl_;
  const std::string last_line_no_nl_;
  const std::vector<LineCache> text_;
  std::size_t idx_ = 0;
};

template<class... Args>
struct Select : Args... {
  explicit Select(Args... args) : Args(args)... {}

  using Args::operator()...;
};

Data::LineCache Data::Process(
    const UnifiedDiff::Options& options,
    const std::optional<UnifiedDiff::RegexReplace>& regex_replace,
    std::string_view line) {
  std::string processed;
  if (options.ignore_all_space) {
    std::string stripped;
    stripped.reserve(line.length());
    for (const char chr : line) {
      if (!absl::ascii_isspace(chr)) {
        stripped.push_back(chr);
      }
    }
    processed = stripped;
  } else if (options.ignore_consecutive_space) {
    processed = line;
    absl::RemoveExtraAsciiWhitespace(&processed);
  } else if (options.ignore_trailing_space) {
    processed = absl::StripTrailingAsciiWhitespace(line);
  } else {
    processed = line;
  }
  std::visit(
      Select{
          [&](UnifiedDiff::NoCommentStripping) {},
          [&](const mbo::strings::StripCommentArgs& args) {
            processed = mbo::strings::StripLineComments(processed, args);
          },
          [&](const mbo::strings::StripParsedCommentArgs& args) {
            const auto line_or = mbo::strings::StripParsedLineComments(processed, args);
            if (line_or.ok()) {
              processed = *std::move(line_or);
            }
          }},
      options.strip_comments);
  if (regex_replace.has_value()) {
    RE2::Replace(&processed, *regex_replace->regex, regex_replace->replace);
  }
  return {
      .line = line,
      .processed = processed,
      .matches_ignore = options.ignore_matching_chunks && options.ignore_matching_lines.has_value()
                        && RE2::PartialMatch(processed, *options.ignore_matching_lines),
  };
}

class Context final {
 public:
  Context() = delete;
  ~Context() = default;

  explicit Context(const UnifiedDiff::Options& options) : options_(options) {}

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
  const UnifiedDiff::Options& options_;
  std::list<std::string_view> data_;
};

class Chunk {
 public:
  Chunk() = delete;
  ~Chunk() = default;

  Chunk(const file::Artefact& lhs, const file::Artefact& rhs, const UnifiedDiff::Options& options)
      : options_(options), lhs_empty_(lhs.data.empty()), rhs_empty_(rhs.data.empty()), context_(options) {
    absl::StrAppend(&output_, "--- ", SelectFileHeader(lhs, lhs, rhs, options), "\n");
    absl::StrAppend(&output_, "+++ ", SelectFileHeader(rhs, lhs, rhs, options), "\n");
  }

  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&&) = delete;
  Chunk& operator=(Chunk&&) = delete;

  void PushBoth(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view ctx) {
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

  void PushLhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view lhs) {
    if (options_.skip_left_deletions) {
      return;
    }
    only_blank_lines_ &= lhs.empty();
    only_matching_lines_ &= options_.ignore_matching_lines && RE2::PartialMatch(lhs, *options_.ignore_matching_lines);
    CheckContext(lhs_idx, rhs_idx);
    lhs_.emplace_back(lhs);
    ++lhs_size_;
  }

  void PushRhs(std::size_t lhs_idx, std::size_t rhs_idx, std::string_view rhs) {
    only_blank_lines_ &= rhs.empty();
    only_matching_lines_ &= options_.ignore_matching_lines && RE2::PartialMatch(rhs, *options_.ignore_matching_lines);
    CheckContext(lhs_idx, rhs_idx);
    rhs_.emplace_back(rhs);
    ++rhs_size_;
  }

  std::string MoveOutput() {
    OutputChunk();
    if (diff_found_) {
      return std::move(output_);
    } else {
      return "";  // Not showing chunk header.
    }
  }

 private:
  static std::string FileHeader(const file::Artefact& info, const UnifiedDiff::Options& options) {
    std::string_view name;
    if (options.strip_file_header_prefix.find_first_of(".*?()[]|") == std::string::npos) {
      name = absl::StripPrefix(info.name, options.strip_file_header_prefix);
    } else {
      name = info.name;
      RE2::Consume(&name, options.strip_file_header_prefix);
    }
    return absl::StrCat(info.name.empty() ? "-" : name, " ", absl::FormatTime(options.time_format, info.time, info.tz));
  }

  static std::string SelectFileHeader(
      const file::Artefact& either,
      const file::Artefact& lhs,
      const file::Artefact& rhs,
      const UnifiedDiff::Options& options) {
    switch (options.file_header_use) {
      case UnifiedDiff::Options::FileHeaderUse::kBoth:
        break;  // Do not use default, but break for this case, so it becomes the function return.
      case UnifiedDiff::Options::FileHeaderUse::kLeft: return FileHeader(lhs, options);
      case UnifiedDiff::Options::FileHeaderUse::kRight: return FileHeader(rhs, options);
    }
    return FileHeader(either, options);
  }

  void CheckContext(std::size_t lhs_idx, std::size_t rhs_idx) {
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
    std::size_t ctx = last ? context_.HalfSsize() : context_.Size();
    while (ctx-- > 0) {
      data_.emplace_back(' ', context_.PopFront());
      ++lhs_size_;
      ++rhs_size_;
    }
  }

  static std::string ChunkPos(bool empty, std::size_t idx, std::size_t size) {
    if (empty) {
      return "0,0";
    } else if (size == 1) {
      return absl::StrCat(idx + 1);
    } else {
      return absl::StrCat(idx + 1, ",", size);
    }
  }

  void OutputChunk() {
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
    // Output position and length:
    // - If there is no content, then line is 0, otherwise use next line,
    //   whether ot not it has content.
    // - Do not show length 1
    absl::StrAppendFormat(
        &output_, "@@ -%s +%s @@\n",                // Format
        ChunkPos(lhs_empty_, lhs_idx_, lhs_size_),  // Format
        ChunkPos(rhs_empty_, rhs_idx_, rhs_size_));
    while (!data_.empty()) {
      absl::StrAppendFormat(&output_, "%c%s\n", data_.front().first, data_.front().second);
      data_.pop_front();
    }
  }

  void Clear() {
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

  const UnifiedDiff::Options& options_;
  const bool lhs_empty_ = false;
  const bool rhs_empty_ = false;
  std::string output_;
  Context context_;
  std::list<std::pair<char, std::string_view>> data_;
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

}  // namespace diff_internal

class UnifiedDiffImpl {
 public:
  UnifiedDiffImpl() = delete;

  UnifiedDiffImpl(const file::Artefact& lhs, const file::Artefact& rhs, const UnifiedDiff::Options& options)
      : options_(options),
        lhs_data_(options_, options_.regex_replace_lhs, lhs.data),
        rhs_data_(options_, options_.regex_replace_rhs, rhs.data),
        chunk_(lhs, rhs, options_) {}

  ~UnifiedDiffImpl() = default;
  UnifiedDiffImpl(const UnifiedDiffImpl&) = delete;
  UnifiedDiffImpl& operator=(const UnifiedDiffImpl&) = delete;
  UnifiedDiffImpl(UnifiedDiffImpl&&) = delete;
  UnifiedDiffImpl& operator=(UnifiedDiffImpl&&) = delete;

  absl::StatusOr<std::string> Compute() {
    Loop();
    return Finalize();
  }

 private:
  bool CompareEq(std::size_t lhs, std::size_t rhs) const;
  void Loop();
  void LoopBoth();
  bool FindNext();
  std::tuple<std::size_t, std::size_t, bool> FindNextRight();
  std::tuple<std::size_t, std::size_t, bool> FindNextLeft();
  bool PastMaxDiffChunkLength(std::size_t& loop);
  absl::StatusOr<std::string> Finalize();

  const UnifiedDiff::Options& options_;
  diff_internal::Data lhs_data_;
  diff_internal::Data rhs_data_;
  diff_internal::Chunk chunk_;
};

bool UnifiedDiffImpl::CompareEq(std::size_t lhs, std::size_t rhs) const {
  const auto& lhs_cache = lhs_data_.GetCache(lhs);
  const auto& rhs_cache = rhs_data_.GetCache(rhs);
  if (lhs_cache.matches_ignore && rhs_cache.matches_ignore) {
    return true;
  }
  if (options_.ignore_case) {
    return absl::EqualsIgnoreCase(lhs_cache.processed, rhs_cache.processed);
  } else {
    return lhs_cache.processed == rhs_cache.processed;
  }
}

void UnifiedDiffImpl::LoopBoth() {
  while (!lhs_data_.Done() && !rhs_data_.Done() && CompareEq(0, 0)) {
    chunk_.PushBoth(lhs_data_.Idx(), rhs_data_.Idx(), lhs_data_.Line());
    lhs_data_.Next();
    rhs_data_.Next();
  }
}

std::tuple<std::size_t, std::size_t, bool> UnifiedDiffImpl::FindNextRight() {
  std::size_t lhs = 1;  // L+0 != R+0 -> start at lhs = 1, R1 = 0
  std::size_t rhs = 0;
  bool equal = false;
  while (!rhs_data_.Done(rhs)) {
    while (!lhs_data_.Done(lhs)) {
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

std::tuple<std::size_t, std::size_t, bool> UnifiedDiffImpl::FindNextLeft() {
  std::size_t lhs = 0;
  std::size_t rhs = 1;  // L+0 != R+0 -> start at L2 = 0, rhs = 1
  bool equal = false;
  while (!lhs_data_.Done(lhs)) {
    while (!rhs_data_.Done(rhs)) {
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

bool UnifiedDiffImpl::PastMaxDiffChunkLength(std::size_t& loop) {
  if (++loop > options_.max_diff_chunk_length) {
    static constexpr std::string_view kMsg = "Maximum loop count reached";
    ABSL_LOG(ERROR) << kMsg;
    chunk_.PushLhs(lhs_data_.Idx(), rhs_data_.Idx(), kMsg);
    return true;
  }
  return false;
}

bool UnifiedDiffImpl::FindNext() {
  auto [lhs1, rhs1, eq1] = FindNextRight();
  auto [lhs2, rhs2, eq2] = FindNextLeft();
  if (eq1 && (!eq2 || AbsDiff(lhs1, rhs1) < AbsDiff(lhs2, rhs2))) {
    for (std::size_t i = 0; i < lhs1; ++i) {
      const std::size_t l_idx = lhs_data_.Idx();  // Compiler might execute `Next` first.
      chunk_.PushLhs(l_idx, rhs_data_.Idx(), lhs_data_.Next());
    }
    for (std::size_t i = 0; i < rhs1; ++i) {
      const std::size_t r_idx = rhs_data_.Idx();
      chunk_.PushRhs(lhs_data_.Idx(), r_idx, rhs_data_.Next());
    }
    return true;
  } else if (eq2) {
    for (std::size_t i = 0; i < lhs2; ++i) {
      const std::size_t l_idx = lhs_data_.Idx();
      chunk_.PushLhs(l_idx, rhs_data_.Idx(), lhs_data_.Next());
    }
    for (std::size_t i = 0; i < rhs2; ++i) {
      const std::size_t r_idx = rhs_data_.Idx();
      chunk_.PushRhs(lhs_data_.Idx(), r_idx, rhs_data_.Next());
    }
    return true;
  } else {
    if (!lhs_data_.Done()) {
      const std::size_t l_idx = lhs_data_.Idx();
      chunk_.PushLhs(l_idx, rhs_data_.Idx(), lhs_data_.Next());
    }
    if (!rhs_data_.Done()) {
      const std::size_t r_idx = rhs_data_.Idx();
      chunk_.PushRhs(lhs_data_.Idx(), r_idx, rhs_data_.Next());
    }
  }
  return false;
}

void UnifiedDiffImpl::Loop() {
  while (!lhs_data_.Done() && !rhs_data_.Done()) {
    LoopBoth();
    std::size_t loop = 0;
    while (!lhs_data_.Done() && !rhs_data_.Done() && !PastMaxDiffChunkLength(loop) && !FindNext()) {}
  }
}

absl::StatusOr<std::string> UnifiedDiffImpl::Finalize() {
  while (!lhs_data_.Done()) {
    const std::size_t l_idx = lhs_data_.Idx();
    chunk_.PushLhs(l_idx, rhs_data_.Idx(), lhs_data_.Next());
  }
  while (!rhs_data_.Done()) {
    const std::size_t r_idx = rhs_data_.Idx();
    chunk_.PushRhs(lhs_data_.Idx(), r_idx, rhs_data_.Next());
  }
  return chunk_.MoveOutput();
}

absl::StatusOr<std::string> UnifiedDiff::Diff(
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const Options& options) {
  if (lhs.data == rhs.data) {
    return std::string();
  }
  UnifiedDiffImpl diff(lhs, rhs, options);
  return diff.Compute();
}

const UnifiedDiff::Options& UnifiedDiff::Options::Default() noexcept {
  static const mbo::types::NoDestruct<UnifiedDiff::Options> kDefaults;
  return *kDefaults;
}

}  // namespace mbo::diff
