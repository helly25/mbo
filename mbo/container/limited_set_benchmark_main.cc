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

#include <functional>

#include "benchmark/benchmark.h"
#include "mbo/container/limited_options.h"
#include "mbo/container/limited_set_benchmark.h"
#include "mbo/types/compare.h"

namespace mbo::container {

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

}  // namespace mbo::container

BENCHMARK_MAIN();  // NOLINT
