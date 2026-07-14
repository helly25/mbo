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

// Length-bucketed throughput benchmark comparing the hash algorithms (see
// hash_test_util.h). Run with: bazel run -c opt //mbo/hash:hash_benchmark
//
// Two size modes (see mbo/hash/measurements/): the default FAST set is the
// small, README-published set - cheap enough for CI. Setting the environment
// variable MBO_HASH_BENCHMARK_FULL=1 selects the dense FULL set that straddles
// every dispatch-tier boundary and SSO cutoff, for the complete dataset and
// the ns-vs-length graph.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "benchmark/benchmark.h"
#include "mbo/hash/hash_test_util.h"

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-array-index,*-magic-numbers)

constexpr uint64_t kSeed = 5'381;

// FAST set (default, CI, and the README tables): a dense set straddling every
// dispatch-tier boundary and SSO cutoff so the small-key cliffs are visible -
// 7/8 the fully-unrolled <=8 path, 15/16 the <=16 path (15 = libstdc++ SSO
// cap), 22 the libc++ SSO cap, 38/47/48 and 63/64 the short-chain steps, 127/128
// bracketing the chain->bulk 128-byte-window edge, with 3/5/11/19/27 filling the
// small range and 256/1024/4096 the bulk.
constexpr std::array<int, 22> kReadmeSizes = {
    1, 3, 5, 7, 8, 11, 15, 16, 19, 22, 27, 32, 38, 47, 48, 63, 64, 127, 128, 256, 1'024, 4'096,
};

// FULL set (MBO_HASH_BENCHMARK_FULL=1): ~3x denser, a slow exponential (ratio
// ~1.2) from 1..4096 unioned with the boundary set above, so the ns-vs-length
// curve is smooth and the tier edges stay sampled. For the complete dataset /
// graph, not for the README tables.
// Tiny             9
// GCC/MSVC SSO     7
// Clang SSO        6
// Extended/AVX2    9
// Cache            4
// Bins             5
// Medium          14
// Large           10
// TOTAL           64
constexpr std::array<int, 64> kFullSizes = {
    // --- Tiny Keys & Word Alignments ---
    1, 2, 3,
    4,  // 32-bit register (optimized fast-paths for integers)
    5, 6, 7,
    8,  // 64-bit word (standard 64-bit register boundary)
    9,

    // --- Register Alignments & GCC/MSVC SSO limits ---
    11,
    12,  // 3x 4-byte words / 3D float vectors
    13,
    15,  // GCC (libstdc++) & MSVC SSO capacity limit (15 chars + null)
    16,  // 128-bit vector register boundary (SSE/AVX)
    18, 19,

    // --- Clang SSO limit & Object sizes ---
    22,  // Clang (libc++) SSO capacity limit (22 chars + null)
    23,  // Clang heap spill boundary (first byte to trigger dynamic allocation); Folly fbstring SSO capacity
    24,  // LLVM's libc++: sizeof(std::string) 3x 8-byte; Folly fbstring SSO capacity
    25, 27, 29,

    // --- Extended SSO & 256-bit Vector Limits ---
    31,  // jemalloc 32-byte class boundary
    32,  // AVX2: 256-bit vector register boundary; MSVC: sizeof(std::string) 4x 8-byte
    38,
    40,          // Common loop unrolling (5x 8-byte words)
    46, 47, 48,  // Alignments around 48 bytes (tcmalloc size class / MSVC heap spill)
    49,          // Just over 48 bytes (forces allocator to bump to 64-byte class)
    55,

    // --- CPU Cache Line Boundaries (64 Bytes) ---
    63,  // Just under cache line (fits entirely within one line)
    64,  // Exactly one L1 cache line (64 bytes)
    66,  // Just over cache line (spills into second cache line)
    72,

    // --- Allocator Bin / Bucket Transitions (jemalloc & tcmalloc) ---
    79,  // Just under 80-byte allocator bucket
    80,  // 80-byte allocation class boundary
    95,  // Just under 96-byte allocator bucket
    107, 114,

    // --- Dual Cache Line & Medium Keys (Scaling exponentially at ~1.2x) ---
    127,                 // Just under 2 cache lines
    128,                 // Exactly 2 cache lines (AVX-512 register size)
    137, 165, 198, 237,  // Geometric steps mapping allocator classes (144, 176, 208)
    256,                 // 4 cache lines / 256-byte allocator boundary
    285, 342, 410, 492, 591, 709, 851,

    // --- Large Keys & Page Boundaries ---
    1'021,  // Max payload size fitting inside a 1KB allocator block with a null-terminator
    1'024,  // Exactly 1KB (half a page block step for typical modern slab allocators)
    1'225, 1'470, 1'764, 2'116, 2'540, 3'048, 3'657,
    4'096  // Exactly one x86/ARM64 virtual page (often triggering direct mmap)
};

// Table/chart consistency rule: kReadmeSizes (README tables) must be a subset of
// kFullSizes (the throughput chart), so every table row has a matching point on
// the curve. Add a README size to the full set too. Enforced at compile time.
constexpr bool ReadmeSizesAreSubsetOfFull() {
  for (const int want : kReadmeSizes) {
    bool found = false;
    for (const int have : kFullSizes) {
      if (have == want) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

static_assert(ReadmeSizesAreSubsetOfFull(), "every kReadmeSizes entry must also appear in kFullSizes");

// The active throughput size set, selected once by the environment.
std::span<const int> ThroughputSizes() {
  const char* const full = std::getenv("MBO_HASH_BENCHMARK_FULL");  // NOLINT(concurrency-mt-unsafe): startup only
  if (full != nullptr && std::string_view(full) == "1") {
    return kFullSizes;
  }
  return kReadmeSizes;
}

template<typename Algo>
void BmHash64(benchmark::State& state) {
  const auto length = static_cast<std::size_t>(state.range(0));
  // NOLINTNEXTLINE(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed data per length
  std::mt19937_64 rng(0x1234);
  const std::string data = algo::RandomString(rng, length);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::GetHash64(data, kSeed));
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(length));
  state.SetLabel(std::string(Algo::Name()));
}

template<typename Algo>
requires HasGetHash128<Algo>
void BmHash128(benchmark::State& state) {
  const auto length = static_cast<std::size_t>(state.range(0));
  // NOLINTNEXTLINE(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed data per length
  std::mt19937_64 rng(0x1234);
  const std::string data = algo::RandomString(rng, length);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::GetHash128(data, kSeed));
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(length));
  state.SetLabel(std::string(Algo::Name()));
}

// --- Latency benchmark: hashing a realistic MIX of key lengths --------------
//
// A hash table does not hash one length in a hot loop (that is BmHash64); it
// hashes a stream of differently-sized keys, so the per-length size dispatch
// cannot be branch-predicted. We model that with two fixed, documented length
// distributions given as inverse-CDF control points (cumulative percentile ->
// length in bytes), piecewise-linear between points:
//   - Short-Identifier: programming identifiers / DB keys / UUIDs (log-normal).
//   - Web/URL: paths, URLs, and larger text keys (heavy-tailed).
// The keys are sampled ONCE from these with a fixed-seed PRNG and SHARED across
// every algorithm in a run, so all algorithms hash the byte-identical key set
// (fair comparison) and the set is reproducible across runs (only the string
// LENGTHS matter; the bytes are irrelevant filler).
constexpr std::size_t kLatencyKeys = 1'024;  // power of two for cheap masking

struct LatencyDist {
  std::string_view name;
  std::array<std::pair<double, int>, 9> cdf;  // ascending (percentile, length)
};

constexpr std::array<LatencyDist, 2> kLatencyDists = {{
    {.name = "Short-Identifier",
     .cdf = {{
         {0.10, 8},
         {0.25, 12},
         {0.50, 16},
         {0.75, 23},
         {0.90, 31},
         {0.95, 38},
         {0.99, 53},
         {0.999, 80},
         {1.0, 128},  // Clean 100% ceiling representing the SSO/AVX-512 transition
     }}},
    {.name = "Web-URL",
     .cdf = {{
         {0.10, 15},
         {0.25, 28},
         {0.50, 45},
         {0.75, 75},
         {0.90, 120},
         {0.95, 220},
         {0.99, 512},
         {0.999, 2'048},
         {1.0, 4'096},  // Clean 100% ceiling representing a full virtual page
     }}},
}};

// Inverse CDF: percentile p in [0,1) -> length. Piecewise-linear between control
// points; below the first point interpolate from (0, 1 byte), at/above the last
// clamp to its length (do not extrapolate the tail into huge outliers).
std::size_t SampleLength(const LatencyDist& dist, double percentile) {
  double prev_p = 0.0;
  double prev_len = 1.0;
  for (const auto& [pct, len] : dist.cdf) {
    if (percentile < pct) {
      const double frac = (percentile - prev_p) / (pct - prev_p);
      return static_cast<std::size_t>(std::lround(prev_len + (frac * (len - prev_len))));
    }
    prev_p = pct;
    prev_len = len;
  }
  return static_cast<std::size_t>(dist.cdf.back().second);
}

// The two key sets, built once (fixed seed) and shared by every latency
// benchmark in the run. `state.range(0)` selects the distribution by index.
const std::vector<std::string>& LatencyKeys(std::size_t dist_index) {
  static const std::array<std::vector<std::string>, kLatencyDists.size()> kKeySets = [] {
    std::array<std::vector<std::string>, kLatencyDists.size()> sets;
    for (std::size_t idx = 0; idx < kLatencyDists.size(); ++idx) {
      // NOLINTNEXTLINE(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed, reproducible set
      std::mt19937_64 rng(0x1a7e9c1);
      sets[idx].reserve(kLatencyKeys);

      // Generate exactly 1023 keys using the distribution
      for (std::size_t i = 0; i < kLatencyKeys - 1; ++i) {
        const double percentile = static_cast<double>(rng()) / (static_cast<double>(UINT64_MAX) + 1.0);
        sets[idx].push_back(algo::RandomString(rng, SampleLength(kLatencyDists[idx], percentile)));
      }

      // Enforce that the 1024th key is guaranteed to be the 100% bounds anchor.
      const std::size_t absolute_max_len = kLatencyDists[idx].cdf.back().second;
      // Passing the RNG to RandomString is safe here because the RNG is only
      // used for generating the string content, not for determining its length.
      // Since this is the final step of a isolated vector generation block, it
      // does not affect the reproducibility of the earlier keys, but it does
      // guarantee that the random string data itself remains completely unique.
      sets[idx].push_back(algo::RandomString(rng, absolute_max_len));
    }
    return sets;
  }();
  return kKeySets[dist_index];
}

template<typename Algo>
void BmHash64Latency(benchmark::State& state) {
  const std::vector<std::string>& keys = LatencyKeys(static_cast<std::size_t>(state.range(0)));

  // Optional: Track cumulative distribution weight
  int64_t total_bytes_per_shuffle = 0;
  for (const auto& key : keys) {
    total_bytes_per_shuffle += static_cast<int64_t>(key.size());
  }

  std::size_t counter = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::GetHash64(keys[counter++ & (kLatencyKeys - 1)], kSeed));
  }

  state.SetItemsProcessed(state.iterations());
  // Allow plotting the total processed gigabytes per second even during
  // unpredictable random branching profiles:
  state.SetBytesProcessed(state.iterations() * (total_bytes_per_shuffle / static_cast<int64_t>(kLatencyKeys)));
  state.SetLabel(std::string(Algo::Name()));
}

// Registers the 64-bit benchmark for one algorithm, plus the 128-bit one where
// the descriptor provides it (detected via the public HasGetHash128 concept).
template<typename Algo>
void RegisterAlgo() {
  const std::string name(Algo::Name());
  const std::span<const int> sizes = ThroughputSizes();
  auto* const hash64 = benchmark::RegisterBenchmark(absl::StrCat("BmHash64<", name, ">"), BmHash64<Algo>);
  for (const int size : sizes) {
    hash64->Arg(size);
  }
  if constexpr (HasGetHash128<Algo>) {
    auto* const hash128 = benchmark::RegisterBenchmark(absl::StrCat("BmHash128<", name, ">"), BmHash128<Algo>);
    for (const int size : sizes) {
      hash128->Arg(size);
    }
  }
  // One benchmark family with the scenarios as Args, so it reports as a grouped
  // table (BmHash64Latency<algo>/0, /1 - one row per scenario), parallel to the
  // throughput families. The Arg index -> distribution name legend is emitted as
  // the `latency_dists` custom context (see main).
  auto* const latency =
      benchmark::RegisterBenchmark(absl::StrCat("BmHash64Latency<", name, ">"), BmHash64Latency<Algo>);
  for (std::size_t dist = 0; dist < kLatencyDists.size(); ++dist) {
    latency->Arg(static_cast<int>(dist));
  }
}

// Registers benchmarks for every descriptor in the central algo::AllAlgorithms
// list -- an algorithm added there is automatically benchmarked here. The tuple
// (of empty descriptor structs) is passed by value purely to deduce the pack.
template<typename... Algos>
void RegisterAll(std::tuple<Algos...> /*algorithms*/) {
  (RegisterAlgo<Algos>(), ...);
}

// NOLINTEND(*-array-index,*-magic-numbers)

}  // namespace
}  // namespace mbo::hash

int main(int argc, char** argv) {
  mbo::hash::RegisterAll(mbo::hash::algo::AllAlgorithms{});
  benchmark::Initialize(&argc, argv);
  // The build compiler is a first-class axis of a measurement (GCC vs Clang perf
  // differs), so record what THIS binary was built with in the dataset context;
  // the stored bundle's filename is tagged with `compiler` too.
#if defined(__clang__)
  benchmark::AddCustomContext("compiler", absl::StrCat("clang-", __clang_major__));
  benchmark::AddCustomContext("compiler_version", __clang_version__);
#elif defined(__GNUC__)
  benchmark::AddCustomContext("compiler", absl::StrCat("gcc-", __GNUC__));
  benchmark::AddCustomContext("compiler_version", __VERSION__);
#endif
  // Emit the curated README size subset (kReadmeSizes) so the report tool extracts
  // the small table straight from a FULL dataset - no separate fast run, and no
  // second size list to drift (this C++ list is the single source of truth).
  benchmark::AddCustomContext("readme_sizes", absl::StrJoin(mbo::hash::kReadmeSizes, ","));
  // Legend for the latency benchmark's Arg index -> distribution name, in order,
  // so the report can label BmHash64Latency<algo>/0, /1 with the scenario names.
  benchmark::AddCustomContext(
      "latency_dists",
      absl::StrJoin(mbo::hash::kLatencyDists, ",", [](std::string* out, const mbo::hash::LatencyDist& dist) {
        absl::StrAppend(out, dist.name);
      }));
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
