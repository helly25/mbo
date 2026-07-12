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

Of the in-house mumbo/jumbo and dumbo family only `dumbo` is ported (single
accumulator, no lanes/streaming/128-bit form - see `hash_dumbo.h`); mumbo and
jumbo stay C++-only. The standard `fnv1a` is also provided (it is what the mangle
seed generation historically folded).

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
)
