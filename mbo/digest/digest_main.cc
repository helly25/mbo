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

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/initialize.h"
#include "mbo/digest/digest.h"
#include "mbo/strings/indent.h"

// NOLINTBEGIN(*avoid-non-const-global-variables,*abseil-no-namespace)

ABSL_FLAG(  //
    std::string,
    algorithm,
    "sha256",
    R"(Digest algorithm:
- md5, sha1:                                       legacy interop only; COLLISION-BROKEN.
- sha224, sha256, sha384, sha512:                  SHA-2 (FIPS 180-4).
- sha512-224, sha512-256:                          SHA-512/t (FIPS 180-4).
- sha3-224, sha3-256, sha3-384, sha3-512:          SHA-3 (FIPS 202).
- shake128, shake256:                              FIPS 202 XOFs, printed at 32 bytes.
- blake2b, blake2b-256:                            BLAKE2b (RFC 7693), 64/32 bytes.
- blake3:                                          BLAKE3, 32 bytes.
)");
ABSL_FLAG(  //
    std::string,
    a,
    "",
    "Short alias for --algorithm (like `shasum -a`); takes precedence when set.");
ABSL_FLAG(  //
    bool,
    ignore_directories,
    false,
    "Silently skip directory arguments instead of reporting them as errors.");
ABSL_FLAG(  //
    bool,
    d,
    false,
    "Short alias for --ignore_directories.");
ABSL_FLAG(  //
    bool,
    reverse,
    false,
    "Print '<file>  <hash>' instead of the checksum-style '<hash>  <file>'.");

// NOLINTEND(*avoid-non-const-global-variables,*abseil-no-namespace)

namespace mbo::digest {
namespace {

namespace fs = std::filesystem;

// Digests a stream in chunks (no whole-file buffering).
template<typename Algo>
std::string HexDigestStream(std::istream& input) {
  static constexpr std::size_t kChunkSize = 1ULL << 16U;
  Streamer<Algo> stream;
  std::array<char, kChunkSize> buffer = {};
  while (input.read(buffer.data(), kChunkSize).gcount() > 0) {
    stream.Update(std::string_view(buffer.data(), static_cast<std::size_t>(input.gcount())));
  }
  return ToHexString(stream.Finalize());
}

// The SHAKE XOFs have no inherent output size; the CLI prints 32 bytes.
constexpr std::size_t kShakeOutputSize = 32;

using DigestFunc = std::string (*)(std::istream&);

struct NamedAlgorithm {
  std::string_view name;
  DigestFunc func;
};

constexpr auto kAlgorithms = std::to_array<NamedAlgorithm>({
    {.name = "md5", .func = &HexDigestStream<md5::Algorithm>},
    {.name = "sha1", .func = &HexDigestStream<sha1::Algorithm>},
    {.name = "sha224", .func = &HexDigestStream<sha224::Algorithm>},
    {.name = "sha256", .func = &HexDigestStream<sha256::Algorithm>},
    {.name = "sha384", .func = &HexDigestStream<sha384::Algorithm>},
    {.name = "sha512", .func = &HexDigestStream<sha512::Algorithm>},
    {.name = "sha512-224", .func = &HexDigestStream<sha512_224::Algorithm>},
    {.name = "sha512-256", .func = &HexDigestStream<sha512_256::Algorithm>},
    {.name = "sha3-224", .func = &HexDigestStream<sha3_224::Algorithm>},
    {.name = "sha3-256", .func = &HexDigestStream<sha3_256::Algorithm>},
    {.name = "sha3-384", .func = &HexDigestStream<sha3_384::Algorithm>},
    {.name = "sha3-512", .func = &HexDigestStream<sha3_512::Algorithm>},
    {.name = "shake128", .func = &HexDigestStream<shake128::Algorithm<kShakeOutputSize>>},
    {.name = "shake256", .func = &HexDigestStream<shake256::Algorithm<kShakeOutputSize>>},
    {.name = "blake2b", .func = &HexDigestStream<blake2b::Algorithm>},
    {.name = "blake2b-256", .func = &HexDigestStream<blake2b_256::Algorithm>},
    {.name = "blake3", .func = &HexDigestStream<blake3::Algorithm>},
});

DigestFunc FindAlgorithm(std::string_view name) {
  for (const NamedAlgorithm& algorithm : kAlgorithms) {
    if (algorithm.name == name) {
      return algorithm.func;
    }
  }
  return nullptr;
}

void Print(std::string_view hex, std::string_view file_name) {
  if (absl::GetFlag(FLAGS_reverse)) {
    std::cout << file_name << "  " << hex << "\n";
  } else {
    std::cout << hex << "  " << file_name << "\n";
  }
}

// Digests all `files` ('-' = stdin); returns the process exit code. Errors
// are reported per file and processing continues (like the coreutils tools).
int Run(DigestFunc digest, const std::vector<std::string_view>& files) {
  int result = 0;
  for (const std::string_view file_name : files) {
    if (file_name == "-") {
      Print(digest(std::cin), file_name);
      continue;
    }
    const fs::path path(file_name);
    std::error_code error;
    if (fs::is_directory(path, error)) {
      if (!absl::GetFlag(FLAGS_ignore_directories) && !absl::GetFlag(FLAGS_d)) {
        std::cerr << file_name << ": Is a directory (not supported).\n";
        result = 1;
      }
      continue;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      std::cerr << file_name << ": Cannot read file.\n";
      result = 1;
      continue;
    }
    Print(digest(input), file_name);
  }
  return result;
}

}  // namespace
}  // namespace mbo::digest

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(mbo::strings::DropIndent(R"(
    [ <flags> ] <file>...

    Prints one '<hash>  <file>' line per file (matching the sha256sum/shasum
    output format; '--reverse' swaps the columns). The algorithm is selected
    with '--algorithm' or its short alias '-a' (default sha256). '-' reads
    standard input. Directories are errors unless '--ignore_directories'
    (short: '-d') silently skips them.
  )"));
  absl::InitializeLog();
  const std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  std::string algorithm_name = absl::GetFlag(FLAGS_a);
  if (algorithm_name.empty()) {
    algorithm_name = absl::GetFlag(FLAGS_algorithm);
  }
  const mbo::digest::DigestFunc digest = mbo::digest::FindAlgorithm(algorithm_name);
  if (digest == nullptr) {
    std::cerr << "Unknown --algorithm '" << algorithm_name
              << "'. Use: " << std::filesystem::path(args[0]).filename().string() << " --help\n";
    return 1;
  }
  if (args.size() < 2) {  // [0] = program
    std::cerr << "At least one file (or '-' for stdin) is required. Use: "
              << std::filesystem::path(args[0]).filename().string() << " --help\n";
    return 1;
  }
  const std::vector<std::string_view> files(args.begin() + 1, args.end());
  return mbo::digest::Run(digest, files);
}
