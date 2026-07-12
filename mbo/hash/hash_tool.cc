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

// A minimal hash CLI: `hash_tool <algo> [<data>]`. It prints the plain 64-bit
// hash (`GetHash64`, each algorithm's canonical default seed) as 16 uppercase
// hex digits; the build-seed mangle is deliberately NOT applied. With <data> it
// hashes that one argument; without, it reads stdin line by line and prints one
// hash per line. It is the C++ reference (the prime implementation) that the
// Starlark ports in `//mbo/hash:hash.bzl` are diffed against, and a small
// standalone tool. It does nothing else.

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "mbo/container/limited_map.h"
#include "mbo/hash/hash.h"

namespace {

using HashFn = uint64_t (*)(std::string_view);

// Plain `GetHash64` per algorithm, each with its own canonical default seed
// (dumbo 0, fnv1a offset basis, ...). Captureless lambdas decay to `HashFn`.
constexpr auto kAlgorithms = mbo::container::ToLimitedMap<std::string_view, HashFn>({
    {"dumbo", [](std::string_view data) { return mbo::hash::dumbo::Algorithm::GetHash64(data); }},
    {"fnv1a", [](std::string_view data) { return mbo::hash::fnv1a::Algorithm::GetHash64(data); }},
    {"mumbo", [](std::string_view data) { return mbo::hash::mumbo::Algorithm::GetHash64(data); }},
    {"murmur3", [](std::string_view data) { return mbo::hash::murmur3::Algorithm::GetHash64(data); }},
    {"siphash", [](std::string_view data) { return mbo::hash::siphash::Algorithm::GetHash64(data); }},
});

std::optional<HashFn> Lookup(std::string_view algo) {
  const auto it = kAlgorithms.find(algo);
  if (it == kAlgorithms.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace

int main(int argc, char** argv) {
  const std::span<char* const> args(argv, static_cast<std::size_t>(argc));
  if (args.size() < 2 || args.size() > 3) {
    std::cerr << absl::StreamFormat(
        "Usage: %s <algo> [<data>]  (data read from stdin, one per line, if omitted)\n",
        args.empty() ? "hash_tool" : args[0]);
    return 2;
  }
  const std::optional<HashFn> hash_fn = Lookup(args[1]);
  if (!hash_fn.has_value()) {
    std::cerr << absl::StreamFormat("Unknown algo '%s'; known: dumbo, fnv1a, mumbo, murmur3, siphash.\n", args[1]);
    return 2;
  }
  if (args.size() == 3) {
    std::cout << absl::StreamFormat("%016X\n", (*hash_fn)(args[2]));
    return 0;
  }
  std::string line;
  while (std::getline(std::cin, line)) {
    std::cout << absl::StreamFormat("%016X\n", (*hash_fn)(line));
  }
  return 0;
}
