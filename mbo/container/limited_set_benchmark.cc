// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#include <functional>  // IWYU pragma: keep
#include <initializer_list>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_join.h"
#include "absl/strings/strip.h"
#include "benchmark/benchmark.h"
#include "mbo/container/limited_options.h"
#include "mbo/container/limited_set.h"
#include "mbo/container/limited_vector.h"
#include "mbo/types/compare.h"  // IWYU pragma: keep

namespace mbo::container {
namespace {

class Random {
 public:
  static constexpr int kUniformValueMin = std::numeric_limits<int>::min();
  static constexpr int kUniformValueMax = std::numeric_limits<int>::max();

  Random()
      : mt_(kRandomInit),  // NOLINT(cert-msc32-c,cert-msc51-cpp)
        uniform_(kUniformValueMin, kUniformValueMax) {}

  int Uniform() { return uniform_(mt_); }

 private:
  static constexpr int kRandomInit = 42;  // Benhchmarks must be repeatable.

  std::mt19937 mt_;
  std::uniform_int_distribution<int> uniform_;
};

template<std::size_t Size, bool HaveOrMiss, template<typename> typename Compare, LimitedOptionsFlag... Flags>
class Benchmarks {
 private:
  enum class Function { kContains, kFind, kIndexOf };

  using BenchmarkedContainer = LimitedSet<int, LimitedOptions<Size, Flags...>{}, Compare<int>>;

  static constexpr std::size_t kNumTestsValues = 100'000;

  static BenchmarkedContainer GetData(Random& random) {
    BenchmarkedContainer data;
    while (data.size() < Size) {
      data.insert(random.Uniform());
    }
    return data;
  }

  static std::vector<int> GetInput(Random& random, const BenchmarkedContainer& data) {
    if (HaveOrMiss) {
      return std::vector<int>{data.begin(), data.end()};
    }
    std::vector<int> input;
    while (input.size() < kNumTestsValues) {
      int value = random.Uniform();
      if (!data.contains(value)) {
        input.push_back(value);
      }
    }
    return input;
  }

  template<Function func>
  static void Benchmark(benchmark::State& state) {
    Random random;  // One per benchmark function instantiation means all functions use the same data.

    struct TestData {
      TestData(Random& random) : data(GetData(random)), input(GetInput(random, data)) {}

      const BenchmarkedContainer data;
      const std::vector<int> input;
    };

    static constexpr std::size_t kNumTestDataSets = 113;  // Number of test data sets.
    const std::vector<TestData> test_data = [&] {
      std::vector<TestData> test_data;
      test_data.reserve(kNumTestDataSets);
      while (test_data.size() < kNumTestDataSets) {
        test_data.emplace_back(random);
      }
      return test_data;
    }();
    std::size_t test = 0;
    std::size_t item = 0;
    int64_t item_count = 0;
    const auto* data = &test_data[0].data;
    const auto* input = &test_data[0].input;
    // NOLINTNEXTLINE(readability-identifier-length)
    for (auto _ : state) {
      if constexpr (func == Function::kContains) {
        if (HaveOrMiss != data->contains((*input)[item])) {
          state.SkipWithError("BAD_RESULT");
          break;
        }
      } else if constexpr (func == Function::kFind) {
        if (HaveOrMiss != (data->find((*input)[item]) != data->end())) {
          state.SkipWithError("BAD_RESULT");
          break;
        }
      } else if constexpr (func == Function::kIndexOf) {
        if (HaveOrMiss != (data->index_of((*input)[item]) != BenchmarkedContainer::npos)) {
          state.SkipWithError("BAD_RESULT");
          break;
        }
      } else {
        state.SkipWithError("NOT IMPLEMENTED");
        break;
      }
      // state.PauseTiming();
      ++item_count;
      if (++item >= input->size()) {
        item = 0;
        ++test;
        test %= test_data.size();
        data = &test_data[test].data;
        input = &test_data[test].input;
      }
      // state.ResumeTiming();
    }
    state.SetItemsProcessed(item_count);
    state.counters["Size"] = Size;
  }

 public:
  static void BmContains(benchmark::State& state) { Benchmark<Function::kContains>(state); }

  static void BmFind(benchmark::State& state) { Benchmark<Function::kFind>(state); }

  static void BmIndexOf(benchmark::State& state) { Benchmark<Function::kIndexOf>(state); }
};

template<bool HaveOrMiss>
std::string MakeName(std::string_view compare, std::string_view flags, std::string_view func) {
  return absl::StrJoin(
      std::initializer_list<std::string_view>{
          absl::StripPrefix(func, "Bm"),
          compare,
          absl::StripPrefix(flags, "LimitedOptionsFlag::k"),
          HaveOrMiss ? "Good" : "Miss",
      },
      ", ");
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define MBO_REGISTER_BENCHMARK(Size, Compare, Func, HaveOrMiss, Flags) \
  BENCHMARK(Benchmarks<Size, HaveOrMiss, Compare, Flags>::Func)->Name(MakeName<HaveOrMiss>(#Compare, #Flags, #Func))

#define MBO_REGISTER_BENCHMARKS_FLAGS(Size, Compare, Func, HaveOrMiss)                   \
  MBO_REGISTER_BENCHMARK(Size, Compare, Func, HaveOrMiss, LimitedOptionsFlag::kDefault); \
  MBO_REGISTER_BENCHMARK(Size, Compare, Func, HaveOrMiss, LimitedOptionsFlag::kNoOptimizeIndexOf)

#define MBO_REGISTER_BENCHMARKS_COMPARE(Size, Func, HaveOrMiss)     \
  MBO_REGISTER_BENCHMARKS_FLAGS(Size, std::less, Func, HaveOrMiss); \
  MBO_REGISTER_BENCHMARKS_FLAGS(Size, mbo::types::CompareLess, Func, HaveOrMiss)

#define MBO_REGISTER_BENCHMARKS_SIZE(Size)                  \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmContains, true);  \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmContains, false); \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmFind, true);      \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmFind, false);     \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmIndexOf, true);   \
  MBO_REGISTER_BENCHMARKS_COMPARE(Size, BmIndexOf, false)

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory)

MBO_REGISTER_BENCHMARKS_SIZE(1);
MBO_REGISTER_BENCHMARKS_SIZE(2);
MBO_REGISTER_BENCHMARKS_SIZE(3);
MBO_REGISTER_BENCHMARKS_SIZE(4);
MBO_REGISTER_BENCHMARKS_SIZE(5);
MBO_REGISTER_BENCHMARKS_SIZE(6);
MBO_REGISTER_BENCHMARKS_SIZE(7);
MBO_REGISTER_BENCHMARKS_SIZE(8);
MBO_REGISTER_BENCHMARKS_SIZE(9);
MBO_REGISTER_BENCHMARKS_SIZE(10);
MBO_REGISTER_BENCHMARKS_SIZE(11);
MBO_REGISTER_BENCHMARKS_SIZE(12);
MBO_REGISTER_BENCHMARKS_SIZE(13);
MBO_REGISTER_BENCHMARKS_SIZE(14);
MBO_REGISTER_BENCHMARKS_SIZE(15);
MBO_REGISTER_BENCHMARKS_SIZE(16);
MBO_REGISTER_BENCHMARKS_SIZE(17);
MBO_REGISTER_BENCHMARKS_SIZE(18);
MBO_REGISTER_BENCHMARKS_SIZE(18);
MBO_REGISTER_BENCHMARKS_SIZE(19);
MBO_REGISTER_BENCHMARKS_SIZE(20);
MBO_REGISTER_BENCHMARKS_SIZE(21);
MBO_REGISTER_BENCHMARKS_SIZE(22);
MBO_REGISTER_BENCHMARKS_SIZE(23);
MBO_REGISTER_BENCHMARKS_SIZE(24);
MBO_REGISTER_BENCHMARKS_SIZE(25);
MBO_REGISTER_BENCHMARKS_SIZE(26);
MBO_REGISTER_BENCHMARKS_SIZE(27);
MBO_REGISTER_BENCHMARKS_SIZE(28);
MBO_REGISTER_BENCHMARKS_SIZE(29);
MBO_REGISTER_BENCHMARKS_SIZE(30);
MBO_REGISTER_BENCHMARKS_SIZE(31);
MBO_REGISTER_BENCHMARKS_SIZE(32);
MBO_REGISTER_BENCHMARKS_SIZE(33);
MBO_REGISTER_BENCHMARKS_SIZE(34);
MBO_REGISTER_BENCHMARKS_SIZE(35);
MBO_REGISTER_BENCHMARKS_SIZE(36);
MBO_REGISTER_BENCHMARKS_SIZE(37);
MBO_REGISTER_BENCHMARKS_SIZE(38);
MBO_REGISTER_BENCHMARKS_SIZE(39);
MBO_REGISTER_BENCHMARKS_SIZE(40);
MBO_REGISTER_BENCHMARKS_SIZE(41);
MBO_REGISTER_BENCHMARKS_SIZE(42);
MBO_REGISTER_BENCHMARKS_SIZE(43);
MBO_REGISTER_BENCHMARKS_SIZE(44);
MBO_REGISTER_BENCHMARKS_SIZE(45);
MBO_REGISTER_BENCHMARKS_SIZE(46);
MBO_REGISTER_BENCHMARKS_SIZE(47);
MBO_REGISTER_BENCHMARKS_SIZE(48);
MBO_REGISTER_BENCHMARKS_SIZE(49);
MBO_REGISTER_BENCHMARKS_SIZE(50);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory)
// NOLINTEND(cppcoreguidelines-macro-usage)

}  // namespace
}  // namespace mbo::container

BENCHMARK_MAIN();  // NOLINT
