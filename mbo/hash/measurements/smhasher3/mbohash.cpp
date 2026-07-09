/*
 * mbo/hash in-house family: mumbo (64), jumbo (128), dumbo (legacy 64)
 * Copyright (C) The helly25 authors (helly25.com)
 *
 * Apache-2.0. This plugin includes the ACTUAL mbo/hash headers (copied into
 * mbo_include/) so SMHasher3 verifies the real implementation, not a
 * transcription. See mbo/hash/measurements/README.md.
 */
#include "Hashlib.h"
#include "Platform.h"
#include "mbo/hash/hash_dumbo.h"
#include "mbo/hash/hash_mumbo.h"

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
    $.desc = "jumbo, mbo/hash in-house native 128-bit (MUM dual-lane)",
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
