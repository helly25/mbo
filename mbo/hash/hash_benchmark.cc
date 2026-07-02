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

#include <cstdint>
#include <random>
#include <string>

#include "benchmark/benchmark.h"
#include "mbo/hash/hash_test_util.h"

namespace mbo::hash {
namespace {

// NOLINTBEGIN(*-magic-numbers)

constexpr uint64_t kSeed = 5'381;
constexpr int kMinLen = 1;
constexpr int kMaxLen = 4'096;
constexpr int kRangeMultiplier = 4;

template<typename Algo>
void BmHash64(benchmark::State& state) {
  const auto length = static_cast<std::size_t>(state.range(0));
  std::mt19937_64 rng(0x1234);  // NOLINT(cert-msc51-cpp,cert-msc32-c): fixed data per length
  const std::string data = algo::RandomString(rng, length);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::Get64(data, kSeed));
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(length));
  state.SetLabel(std::string(Algo::Name()));
}

template<typename Algo>
requires algo::HasHash128<Algo>
void BmHash128(benchmark::State& state) {
  const auto length = static_cast<std::size_t>(state.range(0));
  std::mt19937_64 rng(0x1234);  // NOLINT(cert-msc51-cpp,cert-msc32-c): fixed data per length
  const std::string data = algo::RandomString(rng, length);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Algo::Get128(data, kSeed));
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(length));
  state.SetLabel(std::string(Algo::Name()));
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory)
BENCHMARK_TEMPLATE(BmHash64, algo::SimpleHash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash64, algo::DefaultHash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash64, algo::Fnv1aHash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash64, algo::Xxh64Hash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash64, algo::Murmur3Hash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash128, algo::DefaultHash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
BENCHMARK_TEMPLATE(BmHash128, algo::Murmur3Hash)->RangeMultiplier(kRangeMultiplier)->Range(kMinLen, kMaxLen);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory)

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash

BENCHMARK_MAIN();  // NOLINT
