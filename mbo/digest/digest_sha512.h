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

#ifndef MBO_DIGEST_DIGEST_SHA512_H_
#define MBO_DIGEST_DIGEST_SHA512_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// SHA-384, SHA-512, SHA-512/224 and SHA-512/256 (FIPS 180-4, the 64-bit-word
// SHA-2 family), transcribed from the specification; constexpr-safe with one
// code path for compile time and run time. The four algorithms share the
// compression function and differ only in the initial hash value and the
// output truncation. All constants (including the SHA-512/t IVs, which FIPS
// derives by hashing the algorithm name under a perturbed IV) were generated
// programmatically; all values are pinned in digest_test.cc.
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)
// NOLINTBEGIN(readability-identifier-length): the two-letter working
// variables in Compress mirror the spec's a..h (FIPS 180-4, section 6.4.2).

namespace sha2_64_internal {

// Round constants: fractional parts of the cube roots of the first 80 primes
// (FIPS 180-4, section 4.2.3).
inline constexpr std::array<uint64_t, 80> kRoundK = {
    0x428A2F98D728AE22, 0x7137449123EF65CD, 0xB5C0FBCFEC4D3B2F, 0xE9B5DBA58189DBBC, 0x3956C25BF348B538,
    0x59F111F1B605D019, 0x923F82A4AF194F9B, 0xAB1C5ED5DA6D8118, 0xD807AA98A3030242, 0x12835B0145706FBE,
    0x243185BE4EE4B28C, 0x550C7DC3D5FFB4E2, 0x72BE5D74F27B896F, 0x80DEB1FE3B1696B1, 0x9BDC06A725C71235,
    0xC19BF174CF692694, 0xE49B69C19EF14AD2, 0xEFBE4786384F25E3, 0x0FC19DC68B8CD5B5, 0x240CA1CC77AC9C65,
    0x2DE92C6F592B0275, 0x4A7484AA6EA6E483, 0x5CB0A9DCBD41FBD4, 0x76F988DA831153B5, 0x983E5152EE66DFAB,
    0xA831C66D2DB43210, 0xB00327C898FB213F, 0xBF597FC7BEEF0EE4, 0xC6E00BF33DA88FC2, 0xD5A79147930AA725,
    0x06CA6351E003826F, 0x142929670A0E6E70, 0x27B70A8546D22FFC, 0x2E1B21385C26C926, 0x4D2C6DFC5AC42AED,
    0x53380D139D95B3DF, 0x650A73548BAF63DE, 0x766A0ABB3C77B2A8, 0x81C2C92E47EDAEE6, 0x92722C851482353B,
    0xA2BFE8A14CF10364, 0xA81A664BBC423001, 0xC24B8B70D0F89791, 0xC76C51A30654BE30, 0xD192E819D6EF5218,
    0xD69906245565A910, 0xF40E35855771202A, 0x106AA07032BBD1B8, 0x19A4C116B8D2D0C8, 0x1E376C085141AB53,
    0x2748774CDF8EEB99, 0x34B0BCB5E19B48A8, 0x391C0CB3C5C95A63, 0x4ED8AA4AE3418ACB, 0x5B9CCA4F7763E373,
    0x682E6FF3D6B2B8A3, 0x748F82EE5DEFB2FC, 0x78A5636F43172F60, 0x84C87814A1F0AB72, 0x8CC702081A6439EC,
    0x90BEFFFA23631E28, 0xA4506CEBDE82BDE9, 0xBEF9A3F7B2C67915, 0xC67178F2E372532B, 0xCA273ECEEA26619C,
    0xD186B8C721C0C207, 0xEADA7DD6CDE0EB1E, 0xF57D4F7FEE6ED178, 0x06F067AA72176FBA, 0x0A637DC5A2C898A6,
    0x113F9804BEF90DAE, 0x1B710B35131C471B, 0x28DB77F523047D84, 0x32CAAB7B40C72493, 0x3C9EBE0A15C9BEBC,
    0x431D67C49C100D4C, 0x4CC5D4BECB3E42B6, 0x597F299CFC657E2A, 0x5FCB6FAB3AD6FAEC, 0x6C44198C4A475817,
};

// Initial hash values (FIPS 180-4, sections 5.3.4/5.3.5): fractional parts of
// the square roots of the first 8 primes (SHA-512) and of the 9th through
// 16th primes (SHA-384).
inline constexpr std::array<uint64_t, 8> kInit512 = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179,
};
inline constexpr std::array<uint64_t, 8> kInit384 = {
    0xCBBB9D5DC1059ED8, 0x629A292A367CD507, 0x9159015A3070DD17, 0x152FECD8F70E5939,
    0x67332667FFC00B31, 0x8EB44A8768581511, 0xDB0C2E0D64F98FA7, 0x47B5481DBEFA4FA4,
};

// SHA-512/t initial hash values (FIPS 180-4, section 5.3.6): SHA-512 of the
// ASCII string "SHA-512/t" computed with the SHA-512 IV xored with
// 0xA5A5...A5 (rederived programmatically, matching the published values).
// NOLINTBEGIN(readability-identifier-naming): mirrors the FIPS "SHA-512/t" names.
inline constexpr std::array<uint64_t, 8> kInit512_224 = {
    0x8C3D37C819544DA2, 0x73E1996689DCD4D6, 0x1DFAB7AE32FF9C82, 0x679DD514582F9FCF,
    0x0F6D2B697BD44DA8, 0x77E36F7304C48942, 0x3F9D85A86A1D36C8, 0x1112E6AD91D692A1,
};
inline constexpr std::array<uint64_t, 8> kInit512_256 = {
    0x22312194FC2BF72C, 0x9F555FA3C84C64C2, 0x2393B86B6F53B151, 0x963877195940EABD,
    0x96283EE2A88EFFE3, 0xBE5E1E2553863992, 0x2B0199FC2C85B8AA, 0x0EB72DDC81C52CA2,
};
// NOLINTEND(readability-identifier-naming)

inline constexpr std::size_t kBlockSize = 128;

// Compression function: absorbs one 128-byte block (FIPS 180-4, section 6.4.2).
constexpr void Compress(std::array<uint64_t, 8>& hash, const char* block) noexcept {
  std::array<uint64_t, 80> schedule = {};
  for (std::size_t i = 0; i < 16; ++i) {
    schedule[i] = ::mbo::hash::hash_internal::Load64BE(block + (8 * i));
  }
  for (std::size_t i = 16; i < 80; ++i) {
    const uint64_t sig0 = std::rotr(schedule[i - 15], 1) ^ std::rotr(schedule[i - 15], 8) ^ (schedule[i - 15] >> 7U);
    const uint64_t sig1 = std::rotr(schedule[i - 2], 19) ^ std::rotr(schedule[i - 2], 61) ^ (schedule[i - 2] >> 6U);
    schedule[i] = schedule[i - 16] + sig0 + schedule[i - 7] + sig1;
  }

  uint64_t sa = hash[0];
  uint64_t sb = hash[1];
  uint64_t sc = hash[2];
  uint64_t sd = hash[3];
  uint64_t se = hash[4];
  uint64_t sf = hash[5];
  uint64_t sg = hash[6];
  uint64_t sh = hash[7];
  for (std::size_t i = 0; i < 80; ++i) {
    const uint64_t sum1 = std::rotr(se, 14) ^ std::rotr(se, 18) ^ std::rotr(se, 41);
    const uint64_t choose = (se & sf) ^ (~se & sg);
    const uint64_t temp1 = sh + sum1 + choose + kRoundK[i] + schedule[i];
    const uint64_t sum0 = std::rotr(sa, 28) ^ std::rotr(sa, 34) ^ std::rotr(sa, 39);
    const uint64_t majority = (sa & sb) ^ (sa & sc) ^ (sb & sc);
    const uint64_t temp2 = sum0 + majority;
    sh = sg;
    sg = sf;
    sf = se;
    se = sd + temp1;
    sd = sc;
    sc = sb;
    sb = sa;
    sa = temp1 + temp2;
  }
  hash[0] += sa;
  hash[1] += sb;
  hash[2] += sc;
  hash[3] += sd;
  hash[4] += se;
  hash[5] += sf;
  hash[6] += sg;
  hash[7] += sh;
}

// Streaming state shared by the whole 64-bit family. The buffer's fill level
// is `total_len % kBlockSize`.
struct State {
  std::array<uint64_t, 8> hash = {};
  std::array<char, kBlockSize> buffer = {};
  uint64_t total_len = 0;  // Bytes absorbed, including the buffered tail.
};

constexpr State Init(const std::array<uint64_t, 8>& init) noexcept {
  return {.hash = init, .buffer = {}, .total_len = 0};
}

constexpr void Update(State& state, std::string_view data) noexcept {
  const std::size_t fill = state.total_len % kBlockSize;
  state.total_len += data.size();
  const char* ptr = data.data();
  std::size_t remaining = data.size();
  if (fill != 0) {
    const std::size_t take = remaining < kBlockSize - fill ? remaining : kBlockSize - fill;
    for (std::size_t i = 0; i < take; ++i) {
      state.buffer[fill + i] = ptr[i];
    }
    ptr += take;
    remaining -= take;
    if (fill + take < kBlockSize) {
      return;
    }
    Compress(state.hash, state.buffer.data());
  }
  while (remaining >= kBlockSize) {
    Compress(state.hash, ptr);
    ptr += kBlockSize;
    remaining -= kBlockSize;
  }
  for (std::size_t i = 0; i < remaining; ++i) {
    state.buffer[i] = ptr[i];
  }
}

// Padding + output (FIPS 180-4, section 5.1.2): append 0x80, zero-fill to 112
// mod 128, append the bit length as a big-endian 128-bit integer (the high 64
// bits carry `total_len >> 61`, exact for any 64-bit byte count). Takes the
// state by value so the caller's state stays valid (peekable finalize).
template<std::size_t DigestSize>
constexpr std::array<uint8_t, DigestSize> Finalize(State state) noexcept {
  static_assert(DigestSize <= 64 && DigestSize % 4 == 0);
  const uint64_t bits_high = state.total_len >> 61U;
  const uint64_t bits_low = state.total_len << 3U;
  std::size_t fill = state.total_len % kBlockSize;
  state.buffer[fill++] = static_cast<char>(0x80);
  if (fill > kBlockSize - 16) {
    for (; fill < kBlockSize; ++fill) {
      state.buffer[fill] = 0;
    }
    Compress(state.hash, state.buffer.data());
    fill = 0;
  }
  for (; fill < kBlockSize - 16; ++fill) {
    state.buffer[fill] = 0;
  }
  for (std::size_t i = 0; i < 8; ++i) {
    state.buffer[(kBlockSize - 16) + i] = static_cast<char>(bits_high >> (56 - (8 * i)));
    state.buffer[(kBlockSize - 8) + i] = static_cast<char>(bits_low >> (56 - (8 * i)));
  }
  Compress(state.hash, state.buffer.data());

  std::array<uint8_t, DigestSize> digest = {};
  for (std::size_t i = 0; i < DigestSize; ++i) {
    digest[i] = static_cast<uint8_t>(state.hash[i / 8] >> (56 - (8 * (i % 8))));
  }
  return digest;
}

// The four family members differ only in IV and output truncation; this
// template provides the per-algorithm surface (see the aliases below).
template<std::size_t DigestSizeParam, const std::array<uint64_t, 8>& kInit>
struct Sha2_64Algorithm {  // NOLINT(readability-identifier-naming): family name.
  static constexpr std::size_t kDigestSize = DigestSizeParam;
  static constexpr std::size_t kBlockSize = sha2_64_internal::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = sha2_64_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept {
    State state = Init(kInit);
    Update(state, data);
    return Finalize<kDigestSize>(state);
  }

  static constexpr StreamState StreamInit() noexcept { return Init(kInit); }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept { Update(state, data); }

  static constexpr DigestType StreamFinalize(StreamState state) noexcept { return Finalize<kDigestSize>(state); }
};

}  // namespace sha2_64_internal

namespace sha512 {
inline constexpr std::size_t kDigestSize = 64;
using Algorithm = sha2_64_internal::Sha2_64Algorithm<kDigestSize, sha2_64_internal::kInit512>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha512

namespace sha384 {
inline constexpr std::size_t kDigestSize = 48;
using Algorithm = sha2_64_internal::Sha2_64Algorithm<kDigestSize, sha2_64_internal::kInit384>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha384

namespace sha512_224 {
inline constexpr std::size_t kDigestSize = 28;
using Algorithm = sha2_64_internal::Sha2_64Algorithm<kDigestSize, sha2_64_internal::kInit512_224>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha512_224

namespace sha512_256 {
inline constexpr std::size_t kDigestSize = 32;
using Algorithm = sha2_64_internal::Sha2_64Algorithm<kDigestSize, sha2_64_internal::kInit512_256>;

constexpr Algorithm::DigestType Digest(std::string_view data) noexcept {
  return Algorithm::Digest(data);
}
}  // namespace sha512_256

// NOLINTEND(readability-identifier-length)
// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_SHA512_H_
