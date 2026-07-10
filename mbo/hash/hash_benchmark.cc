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
#include <cstdint>
#include <cstdlib>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "benchmark/benchmark.h"
#include "mbo/hash/hash_test_util.h"

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-magic-numbers)

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
constexpr std::array<int, 53> kFullSizes = {
    1,   2,   3,   4,   5,   6,   7,   8,     9,     11,    13,    15,    16,    18,    19,    22,    27,    32,
    38,  46,  47,  48,  55,  63,  64,  66,    79,    95,    114,   127,   128,   137,   165,   198,   237,   256,
    285, 342, 410, 492, 591, 709, 851, 1'021, 1'024, 1'225, 1'470, 1'764, 2'116, 2'540, 3'048, 3'657, 4'096,
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
  std::mt19937_64 rng(
      0x1234);  // NOLINT(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed data per length
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
  std::mt19937_64 rng(
      0x1234);  // NOLINT(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed data per length
  const std::string data = algo::RandomString(rng, length);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::GetHash128(data, kSeed));
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(length));
  state.SetLabel(std::string(Algo::Name()));
}

// Latency benchmark: keys of unpredictable random length in [0, max_len], and
// each hash result selects the next key, serializing the chain. This defeats
// the branch predictor on the size dispatch and measures latency rather than
// hot-loop throughput -- the cost profile hash-table workloads actually pay.
template<typename Algo>
void BmHash64Latency(benchmark::State& state) {
  const auto max_len = static_cast<std::size_t>(state.range(0));
  std::mt19937_64 rng(
      0x1a7e9c1);  // NOLINT(cert-msc51-cpp,cert-msc32-c,bugprone-random-generator-seed): fixed key set per run
  constexpr std::size_t kNumKeys = 1'024;  // power of two for cheap masking
  std::vector<std::string> keys;
  keys.reserve(kNumKeys);
  for (std::size_t i = 0; i < kNumKeys; ++i) {
    keys.push_back(algo::RandomString(rng, rng() % (max_len + 1)));
  }
  uint64_t hash = 0;
  for (auto _ : state) {
    hash = Algo::GetHash64(keys[hash & (kNumKeys - 1)], kSeed);
    benchmark::DoNotOptimize(hash);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetLabel(std::string(Algo::Name()));
}

// Registers the 64-bit benchmark for one algorithm, plus the 128-bit one where
// the descriptor provides it (detected via the public HasGetHash128 concept).
template<typename Algo>
void RegisterAlgo() {
  const std::string name(Algo::Name());
  const std::span<const int> sizes = ThroughputSizes();
  auto* const hash64 = benchmark::RegisterBenchmark("BmHash64<" + name + ">", BmHash64<Algo>);
  for (const int size : sizes) {
    hash64->Arg(size);
  }
  if constexpr (HasGetHash128<Algo>) {
    auto* const hash128 = benchmark::RegisterBenchmark("BmHash128<" + name + ">", BmHash128<Algo>);
    for (const int size : sizes) {
      hash128->Arg(size);
    }
  }
  benchmark::RegisterBenchmark("BmHash64Latency<" + name + ">", BmHash64Latency<Algo>)->Arg(16)->Arg(64)->Arg(1'024);
}

// Registers benchmarks for every descriptor in the central algo::AllAlgorithms
// list -- an algorithm added there is automatically benchmarked here. The tuple
// (of empty descriptor structs) is passed by value purely to deduce the pack.
template<typename... Algos>
void RegisterAll(std::tuple<Algos...> /*algorithms*/) {
  (RegisterAlgo<Algos>(), ...);
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash

int main(int argc, char** argv) {
  mbo::hash::RegisterAll(mbo::hash::algo::AllAlgorithms{});
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
