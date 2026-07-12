# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Build-time (Starlark) ports of a subset of the `mbo/hash` algorithms.

The single home for the bzl hash offerings, exposed via the public `hash`
struct. C++ is the prime implementation; each offering here is kept byte-for-byte
identical to its C++ counterpart, verified by `//mbo/hash:hash_bzl_vs_cpp_*_test`
(bzl output diffed against the `hash_tool` C++ binary). The design is
algorithm-count agnostic: offering more (or fewer) bzl hashes only widens or
narrows that comparison.

Of the in-house mumbo/jumbo and dumbo family, `mumbo` (the default 64-bit hash)
and `dumbo` (the compact companion) are ported; only their one-shot 64-bit
`GetHash64` is offered here - the native 128-bit `jumbo` form and streaming stay
C++-only. The standard `fnv1a` is also provided (it is what the mangle seed
generation historically folded).

Inputs are either a printable-ASCII string or a list of byte values (0..255);
strings are converted with the printable-ASCII table below (Starlark has no
`ord`).
"""

_MASK64 = (1 << 64) - 1

# Starlark has no `ord`; map printable ASCII (0x20 .. 0x7E) via a table.
_PRINTABLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
_ASCII = {char: index + 32 for index, char in enumerate(_PRINTABLE.elems())}

def _to_bytes(data):
    """Returns `data` as a list of byte values (0..255).

    A list is returned as-is (already byte values); a string is folded through
    the printable-ASCII table and fails on any non-printable input, matching the
    fold-input contract of the mangle seed generation.
    """
    if type(data) != "string":
        return data
    result = []
    for char in data.elems():
        if char not in _ASCII:
            fail("hash input must be printable ASCII, got %r." % char)
        result.append(_ASCII[char])
    return result

def _load_le(data, start, count):
    """Assembles `count` bytes of `data` from `start` as a little-endian uint64 (see `Load64`/`LoadTail`)."""
    result = 0
    for i in range(count):
        result |= data[start + i] << (i * 8)
    return result & _MASK64

def _mult128(lhs, rhs):
    """Full 64x64->128 multiply; returns `(low, high)` (see `hash_internal::Mult128`)."""
    product = (lhs & _MASK64) * (rhs & _MASK64)
    return (product & _MASK64, (product >> 64) & _MASK64)

def _mul128_fold64(lhs, rhs):
    """64x64->128 multiply folded to 64 bits by XORing the halves (see `hash_internal::Mul128Fold64`)."""
    product = (lhs & _MASK64) * (rhs & _MASK64)
    return ((product & _MASK64) ^ (product >> 64)) & _MASK64

# dumbo constants (see `hash_dumbo.h`): golden-ratio reciprocal and the sqrt(3)
# / sqrt(5) fractional parts (SHA-512 initial values, FIPS 180-4).
_DUMBO_INIT = 0x9E3779B97F4A7C15
_DUMBO_WORD = 0xBB67AE8584CAA73B
_DUMBO_STATE = 0x3C6EF372FE94F82B

def _dumbo_mum_step(state, word):
    return _mul128_fold64((word ^ _DUMBO_WORD) & _MASK64, (state ^ _DUMBO_STATE) & _MASK64)

def _dumbo_finalize(state, seed, length):
    low, high = _mult128((state ^ _DUMBO_WORD ^ length) & _MASK64, (seed ^ _DUMBO_STATE) & _MASK64)
    return _mul128_fold64((low ^ _DUMBO_STATE ^ length) & _MASK64, (high ^ _DUMBO_WORD ^ seed) & _MASK64)

def _dumbo(data, seed = 0):
    """dumbo 64-bit hash of `data`; identical to C++ `mbo::hash::dumbo::GetDumboHash`."""
    data = _to_bytes(data)
    length = len(data)
    hash_value = (seed ^ _DUMBO_INIT) & _MASK64
    ptr = 0

    # C++ loops `while (end - ptr) > 8`; Starlark has no `while`, so bound the
    # range (each pass advances 8 bytes) and break on the same condition.
    for _ in range(length // 8 + 1):
        if length - ptr <= 8:
            break
        hash_value = _dumbo_mum_step(hash_value, _load_le(data, ptr, 8))
        ptr += 8

    # Final 1..8 bytes (also the whole key when <= 8) as one overlapping load.
    hash_value = _dumbo_mum_step(hash_value, _load_le(data, ptr, length - ptr))
    return _dumbo_finalize(hash_value, seed, length)

# mumbo (see `hash_mumbo.h`): the library's default 64-bit hash. Its secret bank
# is the 64-bit fractional parts of the square roots of the first sixteen primes
# (the SHA-512 / SHA-384 initial hash values, FIPS 180-4). Unlike dumbo (default
# seed 0) and fnv1a (offset basis), mumbo's canonical default seed is the
# library-wide `kDefaultSeed`. Only the one-shot 64-bit `GetHash64` is ported;
# the native 128-bit `jumbo` form and streaming stay C++-only.
_KDEFAULT_SEED = 5381  # `mbo::hash::kDefaultSeed` (hash_types.h).
_MUMBO_SECRET = [
    0x6A09E667F3BCC908,
    0xBB67AE8584CAA73B,
    0x3C6EF372FE94F82B,
    0xA54FF53A5F1D36F1,
    0x510E527FADE682D1,
    0x9B05688C2B3E6C1F,
    0x1F83D9ABFB41BD6B,
    0x5BE0CD19137E2179,
    0xCBBB9D5DC1059ED8,
    0x629A292A367CD507,
    0x9159015A3070DD17,
    0x152FECD8F70E5939,
    0x67332667FFC00B31,
    0x8EB44A8768581511,
    0xDB0C2E0D64F98FA7,
    0x47B5481DBEFA4FA4,
]
_MUMBO_BULK_WINDOW = 128

def _mumbo_load_small(data, ptr, length):
    """Loads a 0..16 byte key into two words `(a, b)` (see `mumbo_internal::LoadSmall`)."""
    if length >= 4:
        if length >= 9:  # 9..16: two 64-bit loads overlapping the end.
            return _load_le(data, ptr, 8), _load_le(data, ptr + length - 8, 8)
        if length == 8:
            value = _load_le(data, ptr, 8)
            return value, value
        return _load_le(data, ptr, 4), _load_le(data, ptr + length - 4, 4)  # 4..7 (len 4 -> equal)
    if length == 3:
        value = ((data[ptr] << 45) | (data[ptr + 1] << 8) | data[ptr + 2]) & _MASK64
        return value, value
    if length == 2:
        value = ((data[ptr] << 45) | (data[ptr + 1] << 8) | data[ptr]) & _MASK64
        return value, value
    if length == 1:
        value = ((data[ptr] << 45) | data[ptr]) & _MASK64
        return value, value
    return 0, 0

def _mumbo_finish(val_a, val_b, seed, length):
    """The shared two-multiply finalizer (see `mumbo_internal::Finish`)."""
    low, high = _mult128((val_a ^ _MUMBO_SECRET[2] ^ length) & _MASK64, (val_b ^ seed) & _MASK64)
    return _mul128_fold64((low ^ _MUMBO_SECRET[3] ^ length) & _MASK64, (high ^ _MUMBO_SECRET[1]) & _MASK64)

def _mumbo(data, seed = _KDEFAULT_SEED):
    """mumbo 64-bit hash of `data`; identical to C++ `mbo::hash::mumbo::GetHash64`."""
    data = _to_bytes(data)
    length = len(data)

    # Absorb + finalize the seed (structured seeds must not correlate with input).
    seed = _mul128_fold64((seed ^ _MUMBO_SECRET[0]) & _MASK64, _MUMBO_SECRET[1])

    if length <= 16:
        val_a, val_b = _mumbo_load_small(data, 0, length)
        return _mumbo_finish(val_a, val_b, seed, length)

    ptr = 0
    remaining = length

    # >= 128 bytes: eight independent MUM chains over a 128-byte fetch window.
    if length >= _MUMBO_BULK_WINDOW:
        chain = [(seed ^ _MUMBO_SECRET[8 + i]) & _MASK64 for i in range(8)]
        for _ in range(length // _MUMBO_BULK_WINDOW):
            if remaining < _MUMBO_BULK_WINDOW:
                break
            for i in range(8):
                chain[i] = _mul128_fold64(
                    (_load_le(data, ptr + 16 * i, 8) ^ _MUMBO_SECRET[4 + i]) & _MASK64,
                    (_load_le(data, ptr + 16 * i + 8, 8) ^ chain[i]) & _MASK64,
                )
            ptr += _MUMBO_BULK_WINDOW
            remaining -= _MUMBO_BULK_WINDOW
        merged = 0
        for value in chain:
            merged ^= value
        seed = merged & _MASK64

    # 17..127 bytes (and any bulk remainder): one MUM chain, 16 bytes per step.
    # C++ loops `while remaining > 16`; Starlark has no `while`, so bound + break.
    for _ in range(length // 16 + 1):
        if remaining <= 16:
            break
        seed = _mul128_fold64(
            (_load_le(data, ptr, 8) ^ _MUMBO_SECRET[1]) & _MASK64,
            (_load_le(data, ptr + 8, 8) ^ seed) & _MASK64,
        )
        ptr += 16
        remaining -= 16

    # Final 1..16 bytes as two loads overlapping the end (len > 16, so in-bounds).
    val_a = _load_le(data, ptr + remaining - 16, 8)
    val_b = _load_le(data, ptr + remaining - 8, 8)
    return _mumbo_finish(val_a, val_b, seed, length)

# fnv1a constants (see `hash_fnv1a.h`): the offset basis is the canonical default
# seed, so `hash.fnv1a(data)` matches published FNV-1a 64 reference values.
_FNV_OFFSET_BASIS = 0xCBF29CE484222325
_FNV_PRIME = 0x100000001B3

def _fnv1a(data, seed = _FNV_OFFSET_BASIS):
    """FNV-1a 64 of `data`; identical to C++ `mbo::hash::fnv1a::GetHash64`."""
    data = _to_bytes(data)
    hash_value = seed & _MASK64
    for byte in data:
        hash_value = ((hash_value ^ byte) * _FNV_PRIME) & _MASK64
    return hash_value

# The public bzl hash offerings. Each takes a printable-ASCII string or a byte
# list and an optional seed, and returns the 64-bit hash as an integer.
hash = struct(
    dumbo = _dumbo,
    fnv1a = _fnv1a,
    mumbo = _mumbo,
)
