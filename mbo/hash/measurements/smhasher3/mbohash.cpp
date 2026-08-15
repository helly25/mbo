/*
 * mbo/hash in-house family: mumbo/jumbo (64/128) and dumbo (compact 64)
 * Copyright (C) The helly25 authors (helly25.com)
 *
 * Apache-2.0. This plugin includes the ACTUAL mbo/hash headers (copied into
 * mbo_include/) so SMHasher3 verifies the real implementation, not a
 * transcription. See mbo/hash/measurements/README.md.
 */
// Platform.h first: it defines seed_t, the FLAG_IMPL_* / FLAG_HASH_* enums, and
// the HashInfo/REGISTER_HASH prerequisites that Hashlib.h then uses (this is the
// order SMHasher3's own hashes use; reversing it breaks every $.field and flag).
// The clang-format guard is load-bearing: SortIncludes alphabetizes "Hashlib.h"
// before "Platform.h" and silently reverts the fix.
// clang-format off
#include "Platform.h"
#include "Hashlib.h"
// clang-format on
#include "mbo/hash/hash_dumbo.h"
#include "mbo/hash/hash_fambo.h"
#include "mbo/hash/hash_mumbo.h"
#include "mbo/hash/hash_tembo.h"

//------------------------------------------------------------
// mumbo (64-bit) and jumbo (128-bit) both seed directly from SMHasher3's seed.
// The algorithms are little-endian defined and portable, so the value is the
// same on every platform; PUT_U64<bswap> only serializes the output.

template<bool bswap>
static void MumboHash64(const void* in, const size_t len, const seed_t seed, void* out) {
  const std::string_view data((const char*)in, len);
  const uint64_t h = ::mbo::hash::mumbo::GetHash64(data, (uint64_t)seed);
  PUT_U64<bswap>(h, (uint8_t*)out, 0);
}

template<bool bswap>
static void JumboHash128(const void* in, const size_t len, const seed_t seed, void* out) {
  const std::string_view data((const char*)in, len);
  const ::mbo::hash::Hash128 h = ::mbo::hash::jumbo::GetHash128(data, (uint64_t)seed);
  PUT_U64<bswap>(h.h1, (uint8_t*)out, 0);
  PUT_U64<bswap>(h.h2, (uint8_t*)out, 8);
}

template<bool bswap>
static void DumboHash64(const void* in, const size_t len, const seed_t seed, void* out) {
  const std::string_view data((const char*)in, len);
  const uint64_t h = ::mbo::hash::dumbo::Algorithm::GetHash64(data, (uint64_t)seed);
  PUT_U64<bswap>(h, (uint8_t*)out, 0);
}

template<bool bswap>
static void FamboHash64(const void* in, const size_t len, const seed_t seed, void* out) {
  const std::string_view data((const char*)in, len);
  const uint64_t h = ::mbo::hash::fambo::Algorithm::GetHash64(data, (uint64_t)seed);
  PUT_U64<bswap>(h, (uint8_t*)out, 0);
}

template<bool bswap, uint64_t kNumLanes, uint64_t kNumConsts>
static void TemboHash64(const void* in, const size_t len, const seed_t seed, void* out) {
  const std::string_view data((const char*)in, len);
  const uint64_t h = ::mbo::hash::tembo::Algorithm<kNumLanes, kNumConsts, false>::GetHash64(data, (uint64_t)seed);
  PUT_U64<bswap>(h, (uint8_t*)out, 0);
}

//------------------------------------------------------------
REGISTER_FAMILY(mbo_hash, $.src_url = "https://github.com/helly25/mbo", $.src_status = HashFamilyInfo::SRC_ACTIVE);

REGISTER_HASH(
    mumbo_64,
    $.desc = "mumbo, mbo/hash in-house 64-bit (MUM widening-multiply)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x0DAE1DCD,
    $.verification_BE = 0xFBC5E0C5,
    $.hashfn_native = MumboHash64<false>,
    $.hashfn_bswap = MumboHash64<true>);

REGISTER_HASH(
    jumbo_128,
    $.desc = "jumbo, mbo/hash in-house native 128-bit (MUM dual-4-lane)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 128,
    $.verification_LE = 0xE595E9E3,
    $.verification_BE = 0x008B1206,
    $.hashfn_native = JumboHash128<false>,
    $.hashfn_bswap = JumboHash128<true>);

REGISTER_HASH(
    dumbo_64,
    $.desc = "dumbo, mbo/hash in-house 64-bit (compact single-lane MUM, weakly seeded)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x6F1EB379,
    $.verification_BE = 0x36783BC3,
    $.hashfn_native = DumboHash64<false>,
    $.hashfn_bswap = DumboHash64<true>);

REGISTER_HASH(
    fambo_64,
    $.desc = "fambo, mbo/hash in-house 64-bit (fast adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0xBDF11D70,
    $.verification_BE = 0x87654321,
    $.hashfn_native = FamboHash64<false>,
    $.hashfn_bswap = FamboHash64<true>);

REGISTER_HASH(
    tembo_3_4,
    $.desc = "tembo_3_4, mbo/hash in-house, 3 lanes, 48-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0xE5F354B0,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 3, 4>,
    $.hashfn_bswap = TemboHash64<true, 3, 4>);

REGISTER_HASH(
    tembo_4_4,
    $.desc = "tembo_4_4, mbo/hash in-house, 4 lanes, 64-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x6EFF1A2B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 4, 4>,
    $.hashfn_bswap = TemboHash64<true, 4, 4>);

REGISTER_HASH(
    tembo_5_4,
    $.desc = "tembo_5_4, mbo/hash in-house, 5 lanes, 80-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 5, 4>,
    $.hashfn_bswap = TemboHash64<true, 5, 4>);

REGISTER_HASH(
    tembo_6_4,
    $.desc = "tembo_6_4, mbo/hash in-house, 6 lanes, 96-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 6, 4>,
    $.hashfn_bswap = TemboHash64<true, 6, 4>);

REGISTER_HASH(
    tembo_7_4,
    $.desc = "tembo_7_4, mbo/hash in-house, 7 lanes, 112-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 7, 4>,
    $.hashfn_bswap = TemboHash64<true, 7, 4>);

REGISTER_HASH(
    tembo_8_4,
    $.desc = "tembo_8_4, mbo/hash in-house, 8 lanes, 128-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 8, 4>,
    $.hashfn_bswap = TemboHash64<true, 8, 4>);

REGISTER_HASH(
    tembo_8_8,
    $.desc = "tembo_8_8, mbo/hash in-house, 8 lanes, 128-bit, 8 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 8, 8>,
    $.hashfn_bswap = TemboHash64<true, 8, 8>);

REGISTER_HASH(
    tembo_12_4,
    $.desc = "tembo_12_4, mbo/hash in-house, 12 lanes, 192-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 12, 4>,
    $.hashfn_bswap = TemboHash64<true, 12, 4>);

REGISTER_HASH(
    tembo_12_8,
    $.desc = "tembo_12_8, mbo/hash in-house, 12 lanes, 192-bit, 8 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 12, 8>,
    $.hashfn_bswap = TemboHash64<true, 12, 8>);

REGISTER_HASH(
    tembo_16_4,
    $.desc = "tembo_16_4, mbo/hash in-house, 16 lanes, 256-bit, 4 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 16, 4>,
    $.hashfn_bswap = TemboHash64<true, 16, 4>);

REGISTER_HASH(
    tembo_16_8,
    $.desc = "tembo_16_8, mbo/hash in-house, 16 lanes, 256-bit, 8 consts (template configurable adaptive MUM)",
    $.hash_flags = 0,
    $.impl_flags = FLAG_IMPL_MULTIPLY_64_128 | FLAG_IMPL_CANONICAL_LE,
    $.bits = 64,
    $.verification_LE = 0x8BD8598B,
    $.verification_BE = 0x87654321,
    $.hashfn_native = TemboHash64<false, 16, 8>,
    $.hashfn_bswap = TemboHash64<true, 16, 8>);
