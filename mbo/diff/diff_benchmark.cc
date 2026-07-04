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

// Compares the diff algorithms on typical input shapes:
//   equal:    identical inputs (early-out path).
//   edits:    scattered single line changes (the common case).
//   moved:    a block of lines moved to another position.
//   disjoint: no common lines at all (worst case).
//
//   bazel run -c opt //mbo/diff:diff_benchmark

#include <cstddef>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "benchmark/benchmark.h"
#include "mbo/diff/diff.h"
#include "mbo/file/artefact.h"

namespace mbo::diff {
namespace {

// NOLINTBEGIN(*-magic-numbers)

struct Case {
  std::string name;
  file::Artefact lhs;
  file::Artefact rhs;
  bool ignore_case = false;
  bool ignore_trailing_space = false;
  bool ignore_all_space = false;
};

std::string NumberedLines(std::size_t count, std::string_view tag) {
  std::string text;
  for (std::size_t i = 0; i < count; ++i) {
    absl::StrAppend(&text, tag, i % 97, "-", i, "\n");
  }
  return text;
}

std::string EditedLines(std::size_t count, std::string_view tag, std::size_t every) {
  std::string text;
  for (std::size_t i = 0; i < count; ++i) {
    if (i % every == 0) {
      absl::StrAppend(&text, "edited-", i, "\n");
    } else {
      absl::StrAppend(&text, tag, i % 97, "-", i, "\n");
    }
  }
  return text;
}

// Distinct long mixed-case lines; every `edit_every`-th line differs (none
// for 0). Stresses the tokenizer rather than the middle snake search.
std::string LongLines(
    std::size_t count,
    std::size_t width,
    std::size_t edit_every,
    std::string_view pattern = "aBcDeFgHiJkLmNoP",
    std::string_view suffix = "") {
  std::string pad;
  while (pad.size() < width) {
    pad += pattern;
  }
  pad.resize(width);
  std::string text;
  for (std::size_t i = 0; i < count; ++i) {
    if (edit_every != 0 && i % edit_every == 0) {
      absl::StrAppend(&text, "edited-", i, "\n");
    } else {
      absl::StrAppend(&text, "line-", i, "-", pad, suffix, "\n");
    }
  }
  return text;
}

std::string MovedBlock(std::size_t count, std::string_view tag, std::size_t from, std::size_t len, std::size_t to) {
  const std::string base = NumberedLines(count, tag);
  std::vector<std::string_view> lines = absl::StrSplit(base, '\n');
  lines.pop_back();
  std::vector<std::string_view> result;
  result.reserve(lines.size());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i >= from && i < from + len) {
      continue;
    }
    result.push_back(lines[i]);
    if (i == to) {
      for (std::size_t j = from; j < from + len; ++j) {
        result.push_back(lines[j]);
      }
    }
  }
  return absl::StrCat(absl::StrJoin(result, "\n"), "\n");
}

const std::vector<Case>& Cases() {
  static const auto* const kCases = new std::vector<Case>{
      {"equal_10k", {NumberedLines(10'000, "line-"), "lhs"}, {NumberedLines(10'000, "line-"), "rhs"}},
      {"edits_10k", {NumberedLines(10'000, "line-"), "lhs"}, {EditedLines(10'000, "line-", 200), "rhs"}},
      {"moved_10k", {NumberedLines(10'000, "line-"), "lhs"}, {MovedBlock(10'000, "line-", 2'000, 100, 7'000), "rhs"}},
      {"disjoint_2k", {NumberedLines(2'000, "left-"), "lhs"}, {NumberedLines(2'000, "right-"), "rhs"}},
      {"tokenize_20k_long", {LongLines(20'000, 120, 0), "lhs"}, {LongLines(20'000, 120, 500), "rhs"}},
      {"tokenize_20k_long_icase", {LongLines(20'000, 120, 0), "lhs"}, {LongLines(20'000, 120, 500), "rhs"}, true},
      // Lines differing only in trailing space: the strip is a pure suffix trim.
      {.name = "tokenize_20k_trail_space",
       .lhs = {LongLines(20'000, 120, 0, "aBcDeFgHiJkLmNoP", "  "), "lhs"},
       .rhs = {LongLines(20'000, 120, 500, "aBcDeFgHiJkLmNoP", " "), "rhs"},
       .ignore_trailing_space = true},
      // Lines differing only in internal spacing: every line gets rebuilt.
      // The width is a multiple of the pattern length so both sides strip to
      // identical text.
      {.name = "tokenize_20k_all_space",
       .lhs = {LongLines(20'000, 108, 0, "aB cD eF gH iJ kL "), "lhs"},
       .rhs = {LongLines(20'000, 108, 500, "aBcD eFg HiJ kL   "), "rhs"},
       .ignore_all_space = true},
  };
  return *kCases;
}

void BmDiff(benchmark::State& state, DiffOptions::Algorithm algorithm, std::size_t case_idx) {
  const Case& bm_case = Cases()[case_idx];
  const DiffOptions options{
      .algorithm = algorithm,
      .ignore_case = bm_case.ignore_case,
      .ignore_all_space = bm_case.ignore_all_space,
      .ignore_trailing_space = bm_case.ignore_trailing_space,
  };
  for (auto unused : state) {
    auto result = Diff::FileDiff(bm_case.lhs, bm_case.rhs, options);
    benchmark::DoNotOptimize(result);
  }
}

void RegisterAll() {
  for (std::size_t idx = 0; idx < Cases().size(); ++idx) {
    for (const auto& [algo_name, algorithm] :
         {std::pair{"myers", DiffOptions::Algorithm::kMyers}, std::pair{"naive", DiffOptions::Algorithm::kNaive}}) {
      benchmark::RegisterBenchmark(
          absl::StrCat("BmDiff<", algo_name, ">/", Cases()[idx].name),
          [algorithm, idx](benchmark::State& state) { BmDiff(state, algorithm, idx); })
          ->Unit(benchmark::kMillisecond);
    }
  }
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::diff

int main(int argc, char** argv) {
  mbo::diff::RegisterAll();
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
