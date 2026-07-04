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

#include "mbo/digest/digest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

// Known-answer vectors: the FIPS 180-4 / NIST CAVP examples plus
// padding-boundary lengths (55/56/63/64/65 bytes straddle the 0x80+length
// padding cases). Every hex value was generated with python's hashlib, whose
// implementations are independently NIST-validated.
namespace mbo::digest {
namespace {

using ::testing::ElementsAreArray;

static_assert(IsDigestAlgorithm<sha256::Algorithm>);
static_assert(IsDigestAlgorithm<sha224::Algorithm>);
static_assert(HasStreaming<sha256::Algorithm>);
static_assert(HasStreaming<sha224::Algorithm>);

constexpr uint8_t Nibble(char chr) noexcept {
  if (chr >= '0' && chr <= '9') {
    return static_cast<uint8_t>(chr - '0');
  }
  return static_cast<uint8_t>(chr - 'a' + 10);  // NOLINT(*-magic-numbers)
}

// Parses "ab01..." into bytes at compile time (test-side inverse of
// `ToHexString`).
template<std::size_t DigestSize>
constexpr std::array<uint8_t, DigestSize> FromHex(std::string_view hex) noexcept {
  std::array<uint8_t, DigestSize> result = {};
  if (hex.size() != 2 * DigestSize) {
    return result;  // Wrong-length vector: returns zeros, which cannot match a real digest.
  }
  for (std::size_t i = 0; i < DigestSize; ++i) {
    // NOLINTNEXTLINE(*-constant-array-index)
    result[i] = static_cast<uint8_t>(
        (static_cast<uint32_t>(Nibble(hex[2 * i])) << 4U) | static_cast<uint32_t>(Nibble(hex[(2 * i) + 1])));
  }
  return result;
}

struct TestVector {
  std::string_view input;
  std::string_view hex;
};

// 65 'a's; the boundary-length inputs below are prefixes of this.
constexpr std::string_view kManyAs = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

constexpr std::string_view kTwoBlockMsg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
constexpr std::string_view kFourBlockMsg =
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

template<typename Algo>
struct AlgoTraits;

template<>
struct AlgoTraits<sha256::Algorithm> {
  static constexpr std::string_view kName = "sha256";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {.input = "abc", .hex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {.input = kTwoBlockMsg, .hex = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
      {.input = kFourBlockMsg, .hex = "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"},
      {.input = std::string_view{"\0", 1}, .hex = "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"},
      {.input = kManyAs.substr(0, 55), .hex = "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
      {.input = kManyAs.substr(0, 56), .hex = "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
      {.input = kManyAs.substr(0, 63), .hex = "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
      {.input = kManyAs.substr(0, 64), .hex = "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
      {.input = kManyAs.substr(0, 65), .hex = "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"},
  });
  static constexpr std::string_view kMillionAHex = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
};

template<>
struct AlgoTraits<sha224::Algorithm> {
  static constexpr std::string_view kName = "sha224";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"},
      {.input = "abc", .hex = "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"},
      {.input = kTwoBlockMsg, .hex = "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525"},
      {.input = kFourBlockMsg, .hex = "c97ca9a559850ce97a04a96def6d99a9e0e0e2ab14e6b8df265fc0b3"},
      {.input = std::string_view{"\0", 1}, .hex = "fff9292b4201617bdc4d3053fce02734166a683d7d858a7f5f59b073"},
      {.input = kManyAs.substr(0, 55), .hex = "fb0bd626a70c28541dfa781bb5cc4d7d7f56622a58f01a0b1ddd646f"},
      {.input = kManyAs.substr(0, 56), .hex = "d40854fc9caf172067136f2e29e1380b14626bf6f0dd06779f820dcd"},
      {.input = kManyAs.substr(0, 63), .hex = "1d4e051f4d6fed2a63fd2421e65834cec00d64456553de3496ae8b1d"},
      {.input = kManyAs.substr(0, 64), .hex = "a88cd5cde6d6fe9136a4e58b49167461ea95d388ca2bdb7afdc3cbf4"},
      {.input = kManyAs.substr(0, 65), .hex = "ff8716f600af42959d0efb52e1f21b01bb328733009344d511c299fb"},
  });
  static constexpr std::string_view kMillionAHex = "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67";
};

template<typename Algo>
class DigestTest : public ::testing::Test {};

using AllAlgorithms = ::testing::Types<sha256::Algorithm, sha224::Algorithm>;
TYPED_TEST_SUITE(DigestTest, AllAlgorithms);

TYPED_TEST(DigestTest, KnownAnswers) {
  using Traits = AlgoTraits<TypeParam>;
  for (const TestVector& vector : Traits::kVectors) {
    EXPECT_THAT(TypeParam::Digest(vector.input), ElementsAreArray(FromHex<TypeParam::kDigestSize>(vector.hex)))
        << Traits::kName << " input of length " << vector.input.size();
    EXPECT_EQ(ToHexString(TypeParam::Digest(vector.input)), vector.hex);
  }
}

TYPED_TEST(DigestTest, ConstexprMatchesRuntime) {
  using Traits = AlgoTraits<TypeParam>;
  // Compile-time proof for one vector per padding case...
  static constexpr auto kCompileTime = TypeParam::Digest(Traits::kVectors[1].input);  // "abc"
  static_assert(kCompileTime == FromHex<TypeParam::kDigestSize>(AlgoTraits<TypeParam>::kVectors[1].hex));
  // ...and runtime==constexpr equality across all vectors (guards the
  // dual-path `Load32BE`).
  for (const TestVector& vector : Traits::kVectors) {
    const auto runtime = TypeParam::Digest(vector.input);
    EXPECT_THAT(runtime, ElementsAreArray(FromHex<TypeParam::kDigestSize>(vector.hex)));
  }
}

TYPED_TEST(DigestTest, StreamingMatchesOneShot) {
  using Traits = AlgoTraits<TypeParam>;
  std::string all;
  for (const TestVector& vector : Traits::kVectors) {
    all += vector.input;
  }
  const auto expected = TypeParam::Digest(all);
  for (const std::size_t chunk_size : {1U, 3U, 7U, 13U, 63U, 64U, 65U, 200U}) {
    Streamer<TypeParam> stream;
    for (std::size_t pos = 0; pos < all.size(); pos += chunk_size) {
      stream.Update(std::string_view(all).substr(pos, chunk_size));
    }
    EXPECT_THAT(stream.Finalize(), ElementsAreArray(expected)) << "chunk size " << chunk_size;
  }
}

TYPED_TEST(DigestTest, StreamerIsPeekable) {
  Streamer<TypeParam> stream;
  stream.Update("abc");
  EXPECT_THAT(stream.Finalize(), ElementsAreArray(TypeParam::Digest("abc")));
  stream.Update("def");
  EXPECT_THAT(stream.Finalize(), ElementsAreArray(TypeParam::Digest("abcdef")));
}

TYPED_TEST(DigestTest, MillionA) {
  using Traits = AlgoTraits<TypeParam>;
  const std::string input(1'000'000, 'a');
  EXPECT_EQ(ToHexString(TypeParam::Digest(input)), Traits::kMillionAHex);
}

TEST(DigestTest, ToHexString) {
  constexpr std::array<uint8_t, 4> kBytes = {0x00, 0x0F, 0xA5, 0xFF};
  EXPECT_EQ(ToHexString(kBytes), "000fa5ff");
}

}  // namespace
}  // namespace mbo::digest
