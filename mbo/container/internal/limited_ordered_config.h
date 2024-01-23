// Copyright M. Boerger (helly25.com)
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

#ifndef MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_CONFIG_H_
#define MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_CONFIG_H_

#include <cstddef>

namespace mbo::container::container_internal {

// The maximum unroll capacity for `LimitedOrdered` based containers: `LimitedSet` and `LimitedMap`.
// Beyond unrolling 32 comparison steps, unrolling has deminishing returns for any architecture.
// For an Apple M2-Max and an AMD Epyc 7000 CPU, 24 turned out to be a good compromise in micro benchmarks.
// For complex code inlining + unrolling might become problematic for larger values.
static constexpr std::size_t kUnrollMaxCapacityDefault = 16;

}  // namespace mbo::container::container_internal

#endif  // MBO_CONTAINER_INTERNAL_LIMITED_ORDERED_CONFIG_H_
