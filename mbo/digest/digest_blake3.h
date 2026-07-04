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

#ifndef MBO_DIGEST_DIGEST_BLAKE3_H_
#define MBO_DIGEST_DIGEST_BLAKE3_H_

// IWYU pragma: private, include "mbo/digest/digest.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"

// BLAKE3 (https://github.com/BLAKE3-team/BLAKE3-specs), transcribed from the
// specification's reference implementation structure; constexpr-safe with one
// code path for compile time and run time. Scalar and sequential - the
// Merkle-tree structure that lets other implementations parallelize is
// faithfully computed (values are the canonical BLAKE3 values, pinned against
// the official test-vector suite), but no SIMD/thread parallelism is used,
// matching this library's constexpr single-path design.
//
// All three modes are provided:
// - `Digest` / `DigestXof<N>`: plain hashing (default 32 bytes; any output
//   length via the XOF - longer outputs share their prefix, they are not
//   domain-separated).
// - `DigestKeyed`: native keyed mode (exactly 32-byte key).
// - `DeriveKey<N>`: the KDF mode (hardcoded-context domain separation).
//
// Upstream attribution: BLAKE3 - Copyright (C) 2019-2020 Jack O'Connor and
// Samuel Neves, released under CC0-1.0 OR Apache-2.0; this transcription
// follows the public specification (see the repository-root NOTICE file).
namespace mbo::digest {

// NOLINTBEGIN(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index,*-easily-swappable-parameters)

namespace blake3_internal {

inline constexpr std::size_t kBlockSize = 64;     // Bytes per compression block.
inline constexpr std::size_t kChunkSize = 1'024;  // Bytes per chunk (16 blocks).
inline constexpr std::size_t kKeySize = 32;

// The BLAKE3 IV - the SHA-256 initial hash values.
inline constexpr std::array<uint32_t, 8> kInit = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
};

// Domain flags (spec section 2.8).
inline constexpr uint32_t kChunkStart = 1U << 0U;
inline constexpr uint32_t kChunkEnd = 1U << 1U;
inline constexpr uint32_t kParent = 1U << 2U;
inline constexpr uint32_t kRoot = 1U << 3U;
inline constexpr uint32_t kKeyedHash = 1U << 4U;
inline constexpr uint32_t kDeriveKeyContext = 1U << 5U;
inline constexpr uint32_t kDeriveKeyMaterial = 1U << 6U;

// Message word permutation applied between rounds (spec table 2).
inline constexpr std::array<uint8_t, 16> kMsgPermutation = {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8};

// The G mixing function (spec section 2.2; BLAKE2s rotations 16/12/8/7).
// NOLINTBEGIN(readability-identifier-length): parameters mirror the spec's
// a/b/c/d state indices and x/y message words.
constexpr void MixG(
    std::array<uint32_t, 16>& state,
    std::size_t ia,
    std::size_t ib,
    std::size_t ic,
    std::size_t id,
    uint32_t mx,
    uint32_t my) noexcept {
  state[ia] = state[ia] + state[ib] + mx;
  state[id] = std::rotr(state[id] ^ state[ia], 16);
  state[ic] = state[ic] + state[id];
  state[ib] = std::rotr(state[ib] ^ state[ic], 12);
  state[ia] = state[ia] + state[ib] + my;
  state[id] = std::rotr(state[id] ^ state[ia], 8);
  state[ic] = state[ic] + state[id];
  state[ib] = std::rotr(state[ib] ^ state[ic], 7);
}

// NOLINTEND(readability-identifier-length)

// The compression function (spec section 2.2): 7 rounds with the message
// permuted between rounds; returns the full 16-word state (the first 8 words
// are the new chaining value; all 16 feed the root output).
constexpr std::array<uint32_t, 16> Compress(
    const std::array<uint32_t, 8>& chaining,
    const std::array<uint32_t, 16>& block_words,
    uint64_t counter,
    uint32_t block_len,
    uint32_t flags) noexcept {
  std::array<uint32_t, 16> state = {
      chaining[0],
      chaining[1],
      chaining[2],
      chaining[3],
      chaining[4],
      chaining[5],
      chaining[6],
      chaining[7],
      kInit[0],
      kInit[1],
      kInit[2],
      kInit[3],
      static_cast<uint32_t>(counter),
      static_cast<uint32_t>(counter >> 32U),
      block_len,
      flags,
  };
  std::array<uint32_t, 16> message = block_words;
  for (std::size_t round = 0;; ++round) {
    MixG(state, 0, 4, 8, 12, message[0], message[1]);
    MixG(state, 1, 5, 9, 13, message[2], message[3]);
    MixG(state, 2, 6, 10, 14, message[4], message[5]);
    MixG(state, 3, 7, 11, 15, message[6], message[7]);
    MixG(state, 0, 5, 10, 15, message[8], message[9]);
    MixG(state, 1, 6, 11, 12, message[10], message[11]);
    MixG(state, 2, 7, 8, 13, message[12], message[13]);
    MixG(state, 3, 4, 9, 14, message[14], message[15]);
    if (round == 6) {
      break;
    }
    std::array<uint32_t, 16> permuted = {};
    for (std::size_t i = 0; i < 16; ++i) {
      permuted[i] = message[kMsgPermutation[i]];
    }
    message = permuted;
  }
  for (std::size_t i = 0; i < 8; ++i) {
    state[i] ^= state[i + 8];
    state[i + 8] ^= chaining[i];
  }
  return state;
}

// Loads a (possibly zero-padded) 64-byte block as 16 little-endian words.
constexpr std::array<uint32_t, 16> BlockWords(const std::array<char, kBlockSize>& block) noexcept {
  std::array<uint32_t, 16> words = {};
  for (std::size_t i = 0; i < 16; ++i) {
    words[i] = ::mbo::hash::hash_internal::Load32(block.data() + (4 * i));  // little-endian
  }
  return words;
}

// A pending output node: everything needed to either produce a chaining value
// (interior of the tree) or, with the ROOT flag, the final output stream.
struct Output {
  std::array<uint32_t, 8> input_chaining = {};
  std::array<uint32_t, 16> block_words = {};
  uint64_t counter = 0;
  uint32_t block_len = 0;
  uint32_t flags = 0;
};

constexpr std::array<uint32_t, 8> ChainingValue(const Output& output) noexcept {
  const std::array<uint32_t, 16> state =
      Compress(output.input_chaining, output.block_words, output.counter, output.block_len, output.flags);
  return {state[0], state[1], state[2], state[3], state[4], state[5], state[6], state[7]};
}

// Streaming state: the current chunk, plus the stack of subtree chaining
// values (one per set bit of the chunk counter; 54 levels cover any 64-bit
// input length).
struct State {
  std::array<uint32_t, 8> key = {};
  uint32_t flags = 0;
  // Current chunk.
  std::array<uint32_t, 8> chunk_chaining = {};
  uint64_t chunk_counter = 0;
  std::array<char, kBlockSize> block = {};
  std::size_t block_len = 0;
  std::size_t blocks_compressed = 0;
  std::size_t chunk_len = 0;  // Bytes absorbed into the current chunk.
  // Subtree stack.
  std::array<std::array<uint32_t, 8>, 54> stack = {};
  std::size_t stack_len = 0;
};

constexpr State Init(const std::array<uint32_t, 8>& key, uint32_t flags) noexcept {
  State state = {};
  state.key = key;
  state.flags = flags;
  state.chunk_chaining = key;
  return state;
}

constexpr uint32_t ChunkStartFlag(const State& state) noexcept {
  return state.blocks_compressed == 0 ? kChunkStart : 0;
}

// The pending output node for the current (final) chunk.
constexpr Output ChunkOutput(const State& state) noexcept {
  return {
      .input_chaining = state.chunk_chaining,
      .block_words = BlockWords(state.block),
      .counter = state.chunk_counter,
      .block_len = static_cast<uint32_t>(state.block_len),
      .flags = state.flags | ChunkStartFlag(state) | kChunkEnd,
  };
}

constexpr Output ParentOutput(
    const std::array<uint32_t, 8>& left,
    const std::array<uint32_t, 8>& right,
    const std::array<uint32_t, 8>& key,
    uint32_t flags) noexcept {
  Output output = {
      .input_chaining = key, .block_words = {}, .counter = 0, .block_len = kBlockSize, .flags = flags | kParent};
  for (std::size_t i = 0; i < 8; ++i) {
    output.block_words[i] = left[i];
    output.block_words[i + 8] = right[i];
  }
  return output;
}

// Merges a completed chunk's chaining value into the subtree stack: each
// trailing zero bit of the completed-chunk count merges one stack level
// (spec section 5.1.2).
constexpr void PushChunkChaining(State& state, std::array<uint32_t, 8> chaining, uint64_t total_chunks) noexcept {
  while ((total_chunks & 1U) == 0) {
    --state.stack_len;
    chaining = ChainingValue(ParentOutput(state.stack[state.stack_len], chaining, state.key, state.flags));
    total_chunks >>= 1U;
  }
  state.stack[state.stack_len] = chaining;
  ++state.stack_len;
}

constexpr void Update(State& state, std::string_view data) noexcept {
  const char* ptr = data.data();
  std::size_t remaining = data.size();
  while (remaining > 0) {
    // A full chunk is only closed once more input arrives, so the final
    // chunk (even a full one) stays pending for ChunkOutput at finalize.
    if (state.chunk_len == kChunkSize) {
      const std::array<uint32_t, 8> chunk_chaining = ChainingValue(ChunkOutput(state));
      PushChunkChaining(state, chunk_chaining, state.chunk_counter + 1);
      ++state.chunk_counter;
      state.chunk_chaining = state.key;
      state.block = {};
      state.block_len = 0;
      state.blocks_compressed = 0;
      state.chunk_len = 0;
    }
    // Same one level down: a full block is only compressed once more input
    // arrives within the chunk.
    if (state.block_len == kBlockSize) {
      const std::array<uint32_t, 16> state16 = Compress(
          state.chunk_chaining, BlockWords(state.block), state.chunk_counter, kBlockSize,
          state.flags | ChunkStartFlag(state));
      state.chunk_chaining = {state16[0], state16[1], state16[2], state16[3],
                              state16[4], state16[5], state16[6], state16[7]};
      ++state.blocks_compressed;
      state.block = {};
      state.block_len = 0;
    }
    const std::size_t block_space = kBlockSize - state.block_len;
    const std::size_t chunk_space = kChunkSize - state.chunk_len;
    const std::size_t space = block_space < chunk_space ? block_space : chunk_space;
    const std::size_t take = remaining < space ? remaining : space;
    for (std::size_t i = 0; i < take; ++i) {
      state.block[state.block_len + i] = ptr[i];
    }
    state.block_len += take;
    state.chunk_len += take;
    ptr += take;
    remaining -= take;
  }
}

// Collapses the pending chunk and the subtree stack into the root node and
// squeezes `OutputSize` bytes: output block `i` is the full 16-word state of
// the root compression with counter `i` and the ROOT flag (spec section
// 2.6). Takes the state by value so the caller's state stays valid
// (peekable finalize).
template<std::size_t OutputSize>
constexpr std::array<uint8_t, OutputSize> Finalize(const State& state) noexcept {
  Output output = ChunkOutput(state);
  std::size_t remaining_parents = state.stack_len;
  while (remaining_parents > 0) {
    --remaining_parents;
    output = ParentOutput(state.stack[remaining_parents], ChainingValue(output), state.key, state.flags);
  }
  std::array<uint8_t, OutputSize> digest = {};
  std::size_t written = 0;
  for (uint64_t block_counter = 0; written < OutputSize; ++block_counter) {
    const std::array<uint32_t, 16> words =
        Compress(output.input_chaining, output.block_words, block_counter, output.block_len, output.flags | kRoot);
    const std::size_t take = OutputSize - written < kBlockSize ? OutputSize - written : kBlockSize;
    for (std::size_t i = 0; i < take; ++i) {
      digest[written + i] = static_cast<uint8_t>(words[i / 4] >> (8 * (i % 4)));  // little-endian
    }
    written += take;
  }
  return digest;
}

// Converts a 32-byte key to the 8 little-endian key words.
constexpr std::array<uint32_t, 8> KeyWords(std::string_view key) noexcept {
  std::array<uint32_t, 8> words = {};
  const std::size_t key_len = key.size() < kKeySize ? key.size() : kKeySize;
  std::array<char, kKeySize> padded = {};
  for (std::size_t i = 0; i < key_len; ++i) {
    padded[i] = key[i];
  }
  for (std::size_t i = 0; i < 8; ++i) {
    words[i] = ::mbo::hash::hash_internal::Load32(padded.data() + (4 * i));
  }
  return words;
}

}  // namespace blake3_internal

namespace blake3 {

inline constexpr std::size_t kDigestSize = 32;
inline constexpr std::size_t kBlockSize = blake3_internal::kBlockSize;
inline constexpr std::size_t kKeySize = blake3_internal::kKeySize;

// Plain BLAKE3 with any output length (the XOF form). Longer outputs extend
// shorter ones (shared prefix, no domain separation between lengths).
template<std::size_t OutputSize = kDigestSize>
constexpr std::array<uint8_t, OutputSize> DigestXof(std::string_view data) noexcept {
  blake3_internal::State state = blake3_internal::Init(blake3_internal::kInit, 0);
  blake3_internal::Update(state, data);
  return blake3_internal::Finalize<OutputSize>(state);
}

constexpr std::array<uint8_t, kDigestSize> Digest(std::string_view data) noexcept {
  return DigestXof<kDigestSize>(data);
}

// Native keyed mode (spec section 2.8). Precondition: `key.size() == 32`
// (shorter keys are zero-padded, excess bytes ignored; enforced by the
// known-answer tests at constant evaluation).
template<std::size_t OutputSize = kDigestSize>
constexpr std::array<uint8_t, OutputSize> DigestKeyed(std::string_view key, std::string_view data) noexcept {
  blake3_internal::State state = blake3_internal::Init(blake3_internal::KeyWords(key), blake3_internal::kKeyedHash);
  blake3_internal::Update(state, data);
  return blake3_internal::Finalize<OutputSize>(state);
}

// The KDF mode (spec section 2.8): derives `OutputSize` bytes of key material
// from `key_material` under the (application-unique, hardcoded) `context`
// string. Two stages: hash the context under DERIVE_KEY_CONTEXT, then the
// material keyed by the context digest under DERIVE_KEY_MATERIAL.
template<std::size_t OutputSize = kDigestSize>
constexpr std::array<uint8_t, OutputSize> DeriveKey(std::string_view context, std::string_view key_material) noexcept {
  blake3_internal::State context_state =
      blake3_internal::Init(blake3_internal::kInit, blake3_internal::kDeriveKeyContext);
  blake3_internal::Update(context_state, context);
  const std::array<uint8_t, kKeySize> context_key = blake3_internal::Finalize<kKeySize>(context_state);
  std::array<char, kKeySize> context_key_chars = {};
  for (std::size_t i = 0; i < kKeySize; ++i) {
    context_key_chars[i] = static_cast<char>(context_key[i]);
  }
  blake3_internal::State state = blake3_internal::Init(
      blake3_internal::KeyWords(std::string_view(context_key_chars.data(), kKeySize)),
      blake3_internal::kDeriveKeyMaterial);
  blake3_internal::Update(state, key_material);
  return blake3_internal::Finalize<OutputSize>(state);
}

// The algorithm struct (see `mbo::digest::IsDigestAlgorithm` in digest.h),
// fixed to the default 32-byte output.
struct Algorithm {
  static constexpr std::size_t kDigestSize = ::mbo::digest::blake3::kDigestSize;
  static constexpr std::size_t kBlockSize = ::mbo::digest::blake3::kBlockSize;
  using DigestType = std::array<uint8_t, kDigestSize>;
  using StreamState = blake3_internal::State;

  static constexpr DigestType Digest(std::string_view data) noexcept { return ::mbo::digest::blake3::Digest(data); }

  static constexpr StreamState StreamInit() noexcept { return blake3_internal::Init(blake3_internal::kInit, 0); }

  static constexpr StreamState StreamInitKeyed(std::string_view key) noexcept {
    return blake3_internal::Init(blake3_internal::KeyWords(key), blake3_internal::kKeyedHash);
  }

  static constexpr void StreamUpdate(StreamState& state, std::string_view data) noexcept {
    blake3_internal::Update(state, data);
  }

  static constexpr DigestType StreamFinalize(const StreamState& state) noexcept {
    return blake3_internal::Finalize<kDigestSize>(state);
  }
};

}  // namespace blake3

// NOLINTEND(*-magic-numbers,*-pointer-arithmetic,*-constant-array-index,*-easily-swappable-parameters)

}  // namespace mbo::digest

#endif  // MBO_DIGEST_DIGEST_BLAKE3_H_
