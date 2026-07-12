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

"""Generator for `//mbo/hash:hash_mangle_seed_gen.h` (see `hash_mangle.h`)."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//mbo/hash:hash.bzl", "hash")

# The bucket-0 mangle constant and the full-width odd multiplier that spreads
# a bucket index across all 64 bits. The expansion is deliberately independent
# of the bucket count, so the count never becomes ABI relevant: only the
# resulting constant is baked into code.
_BASE = 0x45828334C99AF44F
_SPREAD = 0x9E3779B97F4A7C15
_MASK64 = (1 << 64) - 1

def mangle_constant(version, seed, buckets):
    """Returns the mangle constant selected by (`version`, `seed`, `buckets`).

    The module version and the raw seed string are folded to one of `buckets`
    constants HERE, before anything enters a C++ action key, so build/remote
    caches see a bounded set of header contents no matter what seed strings
    rotate through the flag. Folding the version makes every release rotate
    the constant by construction - at zero marginal cost, since a release
    recompiles all dependents anyway.

    Args:
        version: The module's own version (`native.module_version()`, injected
            by the `mangle_seed_gen` macro - no duplicated version declaration).
        seed: Arbitrary printable-ASCII string selecting the bucket (folded
            together with the version).
        buckets: 0 disables the mangle (constant 0, `GetHash == GetHash64`);
            1 pins one stable constant across releases and seeds; N >= 2
            bounds the constant to N possible nonzero values.

    Returns:
        The 64-bit constant XORed into `GetHash` values.
    """
    if buckets < 0:
        fail("--//mbo/hash:mangle_seed_buckets must be >= 0, got %d." % buckets)
    if buckets == 0:
        return 0

    # Fold through the in-house dumbo hash (SMHasher3-proven; see `hash.bzl` and
    # `hash_dumbo.h`). The bzl port is kept identical to the C++ one, so the
    # build-time fold and the shipped library agree on the algorithm.
    bucket = hash.dumbo(version + "|" + seed) % buckets
    constant = _BASE ^ ((bucket * _SPREAD) & _MASK64)
    if constant == 0:
        # Invariant: enabled (buckets >= 1) implies an observable, nonzero
        # constant - tests rely on `kMangleConstant == 0` meaning "disabled".
        # Unreachable for sane counts: the only cancelling bucket index is
        # 0xA5960CE159A4B3D3 (~1.2e19), so it requires buckets > 1.19e19.
        fail("--//mbo/hash:mangle_seed_buckets=%d yields mangle constant 0; use fewer buckets." % buckets)
    return constant

# The generated header. `@MANGLE_CONSTANT@` is the only substitution (plain
# string replace: the C++ braces rule out Starlark `format`). It is generated
# per build into `hash_mangle_seed_gen.h` and never committed; `hash_mangle.h`
# errors if it is absent (no committed fallback to drift out of sync).
_HEADER = """\
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

#ifndef MBO_HASH_HASH_MANGLE_SEED_GEN_H_
#define MBO_HASH_HASH_MANGLE_SEED_GEN_H_

// IWYU pragma: private, include "mbo/hash/hash_mangle.h"
// IWYU pragma: friend "mbo/hash/hash_mangle.h"

#include <cstdint>

namespace mbo::hash {

// The build-selected constant XORed into `GetHash` values (see `hash_mangle.h`).
//
// Selected by folding the module's own version (from MODULE.bazel via
// `native.module_version()` - every release rotates the constant by
// construction) with the custom Bazel flags
// `--//mbo/hash:mangle_seed` (any printable ASCII string, folded to a bucket
// inside the generation rule, see `internal/hash_mangle_seed.bzl`) and
// `--//mbo/hash:mangle_seed_buckets` (`0` disables the mangle - constant 0,
// `GetHash == GetHash64`; `1` pins one stable nonzero constant across
// releases; `N` bounds variation to `N` constants so build caches converge).
// When the library is built as a dependency (e.g. as `helly25_mbo`), the
// flags become `--@helly25_mbo//mbo/hash:mangle_seed` and
// `--@helly25_mbo//mbo/hash:mangle_seed_buckets`.
//
// This header is generated per build and is not committed, so it always matches
// the version and flags it was built with; `hash_mangle.h` requires it (a
// missing header is a build error, never a silent fallback).
inline constexpr uint64_t kMangleConstant = @MANGLE_CONSTANT@;

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_MANGLE_SEED_GEN_H_
"""

def _mangle_seed_gen_impl(ctx):
    seed = ctx.attr._mangle_seed[BuildSettingInfo].value
    buckets = ctx.attr._mangle_seed_buckets[BuildSettingInfo].value
    constant = mangle_constant(ctx.attr.version, seed, buckets)
    ctx.actions.write(
        output = ctx.outputs.output,
        content = _HEADER.replace("@MANGLE_CONSTANT@", "0x%XULL" % constant),
    )

_mangle_seed_gen = rule(
    attrs = {
        "output": attr.output(
            mandatory = True,
        ),
        "version": attr.string(
            mandatory = True,
        ),
        "_mangle_seed": attr.label(default = Label("//mbo/hash:mangle_seed")),
        "_mangle_seed_buckets": attr.label(default = Label("//mbo/hash:mangle_seed_buckets")),
    },
    implementation = _mangle_seed_gen_impl,
)

def mangle_seed_gen(name, **kwargs):
    """Instantiates the generator with the module's own version.

    `native.module_version()` reads MODULE.bazel (or, for BCR consumers, the
    resolved helly25_mbo module version) at loading time - the library knows
    its own version without a duplicated declaration. Overrides that strip the
    version (e.g. a bare `git_override`) fold an empty string: less rotation,
    never an error.

    Args:
        name: Target name.
        **kwargs: Forwarded to the underlying rule (`output`, `visibility`).
    """
    _mangle_seed_gen(
        name = name,
        version = native.module_version() or "",
        **kwargs
    )
