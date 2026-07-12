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

"""Verifies the `//mbo/hash:hash.bzl` ports against the C++ prime implementation.

C++ is the source of truth: for a fixed set of inputs, the Starlark hash (folded
at load time) must equal what the `hash_tool` C++ binary prints. A `diff_test`
per offered algorithm compares the two. The design is algorithm-count agnostic -
this only covers the algorithms bzl actually offers.
"""

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//mbo/diff:diff.bzl", "diff_test")
load("//mbo/hash:hash.bzl", "hash")

# Printable ASCII (0x20 .. 0x7E) - the fold-input alphabet. Used to build inputs
# that span a range of byte values as well as a range of lengths.
_PRINTABLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

# Lengths cross dumbo's 8-byte block boundary and every tail size (1..8), and
# extend past a single/multiple blocks for good measure.
_LENGTHS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 23, 24, 25, 31, 32, 33, 63, 64, 65, 100, 127, 128, 129, 255, 256, 300]

# A handful of concrete strings, including mangle fold inputs.
_EXTRA = ["0.13.0|", "0.13.1|", "hello world, this is dumbo!", "|"]

_FNS = {"dumbo": hash.dumbo, "fnv1a": hash.fnv1a}

def _pattern(length):
    return "".join([_PRINTABLE[i % len(_PRINTABLE)] for i in range(length)])

def _hex16(value):
    # Starlark `%` has no width/zero-pad; pad manually to match C++ `%016X`.
    digits = "%X" % value
    return ("0" * (16 - len(digits))) + digits

def hash_bzl_verify(name, algos = ["dumbo", "fnv1a"], tool = "//mbo/hash:hash_tool"):
    """Wires up the bzl-vs-C++ diff tests for each `algo` in `algos`.

    Args:
        name: Base name for the generated inputs/vectors/test targets.
        algos: Offered algorithms to verify (each must be a key of `_FNS`).
        tool: Label of the C++ reference binary (`hash_tool`).
    """
    inputs = [_pattern(length) for length in _LENGTHS] + _EXTRA

    # The shared inputs, one per line (raw; fed to `hash_tool` on stdin).
    write_file(
        name = name + "_inputs",
        out = name + "_inputs.txt",
        content = inputs,
        newline = "unix",
    )

    for algo in algos:
        hash_fn = _FNS[algo]

        # The Starlark result: one 16-hex hash per input, folded at load time.
        write_file(
            name = "%s_%s_bzl" % (name, algo),
            out = "%s_%s_bzl.txt" % (name, algo),
            content = [_hex16(hash_fn(data)) for data in inputs],
            newline = "unix",
        )

        # The C++ prime result over the same inputs.
        native.genrule(
            name = "%s_%s_cpp" % (name, algo),
            srcs = [name + "_inputs.txt"],
            outs = ["%s_%s_cpp.txt" % (name, algo)],
            cmd = "$(execpath %s) %s < $(execpath %s_inputs.txt) > $@" % (tool, algo, name),
            tools = [tool],
        )

        diff_test(
            name = "%s_%s_test" % (name, algo),
            file_old = "%s_%s_cpp.txt" % (name, algo),
            file_new = "%s_%s_bzl.txt" % (name, algo),
            # write_file omits the trailing newline that hash_tool prints.
            ignore_missing_final_newline = True,
        )
