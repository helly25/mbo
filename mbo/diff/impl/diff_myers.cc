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

#include "mbo/diff/impl/diff_myers.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "mbo/diff/diff_options.h"
#include "mbo/diff/internal/data.h"
#include "mbo/file/artefact.h"
#include "mbo/hash/hash.h"

namespace mbo::diff {
namespace {

// Sentinel written just outside the current diagonal window: smaller than any
// reachable x, so the boundary comparisons pick the interior neighbor.
constexpr std::ptrdiff_t kOutside = -1;

// Hash for the interning keys (`std::string_view`s, see `Tokenize`).
struct TokenHash {
  std::size_t operator()(std::string_view text) const noexcept { return mbo::hash::GetHash64(text); }
};

std::size_t ISqrt(std::size_t value) {
  return static_cast<std::size_t>(std::sqrt(static_cast<double>(value)));
}

// Clamps the diagonal window [-d, d] to the edit grid [lo, hi], keeping the
// parity of d (valid diagonals at cost d satisfy k == d (mod 2)).
void ClampWindow(std::ptrdiff_t d, std::ptrdiff_t lo, std::ptrdiff_t hi, std::ptrdiff_t& kmin, std::ptrdiff_t& kmax) {
  kmin = -d < lo ? lo + ((d + lo) & 1 ? 1 : 0) : -d;
  kmax = d > hi ? hi - ((d + hi) & 1 ? 1 : 0) : d;
}

}  // namespace

absl::StatusOr<std::string> DiffMyers::FileDiff(
    const file::Artefact& lhs,
    const file::Artefact& rhs,
    const DiffOptions& options) {
  if (lhs.data == rhs.data) {
    return std::string();
  }
  return DiffMyers(lhs, rhs, options).Compute();
}

DiffMyers::DiffMyers(const file::Artefact& lhs, const file::Artefact& rhs, const DiffOptions& options)
    : ChunkedDiff(lhs, rhs, options) {
  const std::size_t lhs_size = LhsData().Size();
  const std::size_t rhs_size = RhsData().Size();
  max_cost_ = std::max<std::size_t>(64, ISqrt(lhs_size + rhs_size));
  const std::size_t v_size = 2 * (std::max(lhs_size, rhs_size) + 2) + 1;
  fwd_.assign(v_size, kOutside);
  bwd_.assign(v_size, kOutside);
}

absl::StatusOr<std::string> DiffMyers::Compute() {
  Tokenize();
  Loop();
  return Finalize();
}

void DiffMyers::Tokenize() {
  // Interning follows `BaseDiff::CompareEq`: lines matching
  // `ignore_matching_lines` are all mutually equal (they share one token),
  // all other lines compare by their preprocessed text, case folded for
  // `ignore_case`.
  //
  // Keys are `std::string_view`s into the preprocessed line cache (which
  // outlives the tokens), so no line is copied. Under `ignore_case` each
  // line is folded once into a reused buffer (fast word-wise hashing and
  // exact equality beat fold-aware per-byte functors, measured in
  // `diff_benchmark`); only distinct folded lines get stable arena storage.
  constexpr Token kIgnoreToken = 0;
  const bool fold = Options().ignore_case;
  absl::flat_hash_map<std::string_view, Token, TokenHash> ids;
  std::string fold_buffer;
  std::deque<std::string> fold_arena;
  const auto tokenize = [&](const diff_internal::Data& data, std::vector<Token>& tokens) {
    tokens.reserve(data.Size());
    for (std::size_t pos = 0; pos < data.Size(); ++pos) {
      const diff_internal::Data::LineCache& cache = data.GetCache(pos);
      if (cache.matches_ignore) {
        tokens.push_back(kIgnoreToken);
        continue;
      }
      std::string_view key = cache.processed;
      if (fold) {
        fold_buffer.assign(key);
        absl::AsciiStrToLower(&fold_buffer);
        key = fold_buffer;
      }
      auto it = ids.find(key);
      if (it == ids.end()) {
        if (fold) {
          key = fold_arena.emplace_back(fold_buffer);
        }
        it = ids.emplace(key, static_cast<Token>(ids.size() + 1)).first;
      }
      tokens.push_back(it->second);
    }
  };
  tokenize(LhsData(), lhs_tokens_);
  tokenize(RhsData(), rhs_tokens_);
}

void DiffMyers::Loop() {
  // Work stack, processed leftmost first: a range gets split at its middle
  // snake into (left half, snake equals, right half) pushed in reverse.
  std::vector<Span> stack;
  stack.push_back({.lhs_end = lhs_tokens_.size(), .rhs_end = rhs_tokens_.size()});
  while (!stack.empty()) {
    Span span = stack.back();
    stack.pop_back();
    if (span.equals > 0) {
      for (std::size_t i = 0; i < span.equals; ++i) {
        PushEqual();
      }
      continue;
    }
    // Emit the common prefix right away.
    while (span.lhs_begin < span.lhs_end && span.rhs_begin < span.rhs_end
           && lhs_tokens_[span.lhs_begin] == rhs_tokens_[span.rhs_begin]) {
      PushEqual();
      ++span.lhs_begin;
      ++span.rhs_begin;
    }
    // The common suffix is emitted (LIFO) after everything in between.
    std::size_t suffix = 0;
    while (span.lhs_end > span.lhs_begin && span.rhs_end > span.rhs_begin
           && lhs_tokens_[span.lhs_end - 1] == rhs_tokens_[span.rhs_end - 1]) {
      --span.lhs_end;
      --span.rhs_end;
      ++suffix;
    }
    if (suffix > 0) {
      stack.push_back({.equals = suffix});
    }
    if (span.lhs_begin == span.lhs_end) {
      for (std::size_t i = span.rhs_begin; i < span.rhs_end; ++i) {
        PushRhs();
      }
      continue;
    }
    if (span.rhs_begin == span.rhs_end) {
      for (std::size_t i = span.lhs_begin; i < span.lhs_end; ++i) {
        PushLhs();
      }
      continue;
    }
    const Snake snake = FindMiddleSnake(span);
    stack.push_back({snake.lhs_begin + snake.length, span.lhs_end, snake.rhs_begin + snake.length, span.rhs_end, 0});
    if (snake.length > 0) {
      stack.push_back({.equals = snake.length});
    }
    stack.push_back({span.lhs_begin, snake.lhs_begin, span.rhs_begin, snake.rhs_begin, 0});
  }
}

DiffMyers::Snake DiffMyers::FindMiddleSnake(const Span& span) {
  // The span was trimmed: both sides are non-empty and neither the first nor
  // the last lines match, so the minimal cost is >= 2 and the first overlap
  // of the forward and backward searches yields a valid middle snake.
  const std::ptrdiff_t n = static_cast<std::ptrdiff_t>(span.lhs_end - span.lhs_begin);
  const std::ptrdiff_t m = static_cast<std::ptrdiff_t>(span.rhs_end - span.rhs_begin);
  const std::ptrdiff_t delta = n - m;
  const bool odd = (delta & 1) != 0;
  const Token* lhs = lhs_tokens_.data() + span.lhs_begin;
  const Token* rhs = rhs_tokens_.data() + span.rhs_begin;
  const std::ptrdiff_t center = static_cast<std::ptrdiff_t>(fwd_.size() / 2);
  // Cost 0: neither search extends (the corner lines differ).
  fwd_[center] = 0;
  bwd_[center] = 0;
  std::ptrdiff_t bwd_kmin = 0;  // Window written by the last backward pass.
  std::ptrdiff_t bwd_kmax = 0;
  // Furthest reaching points seen, for the cost cap fallback split.
  std::ptrdiff_t best_fwd_x = 0;
  std::ptrdiff_t best_fwd_y = 0;
  std::ptrdiff_t best_bwd_x = 0;
  std::ptrdiff_t best_bwd_y = 0;
  const std::ptrdiff_t d_max = (n + m + 1) / 2 + 1;
  for (std::ptrdiff_t d = 1; d <= d_max; ++d) {
    std::ptrdiff_t kmin = 0;
    std::ptrdiff_t kmax = 0;
    // Forward pass.
    ClampWindow(d, -m, n, kmin, kmax);
    fwd_[center + kmin - 1] = kOutside;
    fwd_[center + kmax + 1] = kOutside;
    for (std::ptrdiff_t k = kmin; k <= kmax; k += 2) {
      std::ptrdiff_t x = k == -d || (k != d && fwd_[center + k - 1] < fwd_[center + k + 1]) ? fwd_[center + k + 1]
                                                                                            : fwd_[center + k - 1] + 1;
      // A path hitting a grid edge continues on other diagonals: the furthest
      // reach on this diagonal is the edge point itself.
      x = std::min({x, m + k, n});
      std::ptrdiff_t y = x - k;
      const std::ptrdiff_t x_begin = x;
      while (x < n && y < m && lhs[x] == rhs[y]) {
        ++x;
        ++y;
      }
      fwd_[center + k] = x;
      if (x + y > best_fwd_x + best_fwd_y) {
        best_fwd_x = x;
        best_fwd_y = y;
      }
      if (odd) {
        // Overlap with the backward (d-1)-path on the same real diagonal?
        const std::ptrdiff_t kr = delta - k;
        if (kr >= bwd_kmin && kr <= bwd_kmax && x + bwd_[center + kr] >= n) {
          return {
              .lhs_begin = span.lhs_begin + static_cast<std::size_t>(x_begin),
              .rhs_begin = span.rhs_begin + static_cast<std::size_t>(x_begin - k),
              .length = static_cast<std::size_t>(x - x_begin),
          };
        }
      }
    }
    const std::ptrdiff_t fwd_kmin = kmin;
    const std::ptrdiff_t fwd_kmax = kmax;
    // Backward pass: a forward search over both sequences reversed.
    ClampWindow(d, -m, n, kmin, kmax);
    bwd_[center + kmin - 1] = kOutside;
    bwd_[center + kmax + 1] = kOutside;
    for (std::ptrdiff_t k = kmin; k <= kmax; k += 2) {
      std::ptrdiff_t x = k == -d || (k != d && bwd_[center + k - 1] < bwd_[center + k + 1]) ? bwd_[center + k + 1]
                                                                                            : bwd_[center + k - 1] + 1;
      x = std::min({x, m + k, n});
      std::ptrdiff_t y = x - k;
      const std::ptrdiff_t x_begin = x;
      while (x < n && y < m && lhs[n - 1 - x] == rhs[m - 1 - y]) {
        ++x;
        ++y;
      }
      bwd_[center + k] = x;
      if (x + y > best_bwd_x + best_bwd_y) {
        best_bwd_x = x;
        best_bwd_y = y;
      }
      if (!odd) {
        // Overlap with the forward d-path on the same real diagonal?
        const std::ptrdiff_t kf = delta - k;
        if (kf >= fwd_kmin && kf <= fwd_kmax && fwd_[center + kf] + x >= n) {
          return {
              .lhs_begin = span.lhs_begin + static_cast<std::size_t>(n - x),
              .rhs_begin = span.rhs_begin + static_cast<std::size_t>(m - y),
              .length = static_cast<std::size_t>(x - x_begin),
          };
        }
      }
    }
    bwd_kmin = kmin;
    bwd_kmax = kmax;
    if (static_cast<std::size_t>(d) < max_cost_) {
      continue;
    }
    // Too expensive (like git): split at the furthest reaching point. Both
    // searches took at least one step, so the split is strictly inside the
    // span and both halves shrink; the result stays a valid edit script, it
    // just may no longer be minimal.
    std::ptrdiff_t split_x = best_fwd_x;
    std::ptrdiff_t split_y = best_fwd_y;
    if (best_bwd_x + best_bwd_y > best_fwd_x + best_fwd_y) {
      split_x = n - best_bwd_x;
      split_y = m - best_bwd_y;
    }
    while (split_x + split_y >= n + m) {  // Keep the split off the far corner.
      split_x > 0 ? --split_x : --split_y;
    }
    return {
        .lhs_begin = span.lhs_begin + static_cast<std::size_t>(split_x),
        .rhs_begin = span.rhs_begin + static_cast<std::size_t>(split_y),
        .length = 0,
    };
  }
  // Unreachable: the searches must overlap within d_max. Split in a valid
  // spot anyway rather than misbehaving.
  return {
      .lhs_begin = span.lhs_begin + static_cast<std::size_t>(best_fwd_x),
      .rhs_begin = span.rhs_begin + static_cast<std::size_t>(best_fwd_y),
      .length = 0,
  };
}

}  // namespace mbo::diff
