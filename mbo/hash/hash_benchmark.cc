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

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "benchmark/benchmark.h"
#include "mbo/hash/hash_test_util.h"

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-magic-numbers)

constexpr uint64_t kSeed = 5'381;

// Throughput key lengths, chosen to straddle the dispatch-tier boundaries and
// the small-string-optimization cutoffs so every cliff is visible: 7/8 bracket
// the end of the fully-unrolled <=8 path, 15/16 the <=16 path (15 is also the
// libstdc++ SSO cap), 22 the libc++ SSO cap, 32 the proposed small-key cap,
// 63/64 the short-chain tier edge, and 256/1024/4096 the bulk throughput.
constexpr std::array<int, 12> kThroughputSizes = {1, 7, 8, 15, 16, 22, 32, 63, 64, 256, 1'024, 4'096};

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
  auto* const hash64 = benchmark::RegisterBenchmark("BmHash64<" + name + ">", BmHash64<Algo>);
  for (const int size : kThroughputSizes) {
    hash64->Arg(size);
  }
  if constexpr (HasGetHash128<Algo>) {
    auto* const hash128 = benchmark::RegisterBenchmark("BmHash128<" + name + ">", BmHash128<Algo>);
    for (const int size : kThroughputSizes) {
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
