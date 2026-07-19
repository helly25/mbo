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

"""Macro `verify_digest_test`: verify committed files against saved digests."""

def verify_digest_test(name, *, algorithm = "sha256", digests = {}, checksums = {}, **kwargs):
    """Fail a test if any file's digest no longer matches its saved value.

    A file may only change when its saved digest is updated in the same commit,
    so nothing drifts unnoticed. Two ways to supply the saved digest:

    - `digests` (preferred): map each file to a checksum sidecar (a coreutils
      `<hash>  <file>` line as produced by `//mbo/digest:digest` or `sha256sum`).
      The sidecar is an independent, externally verifiable artifact - anyone can
      re-check it with stock `sha256sum -c`, no Bazel required - and it keeps the
      long hex out of the BUILD file.
    - `checksums`: map each file directly to its expected hex digest, inline in
      the BUILD file. Handy for a quick content pin with no companion file (the
      value is visible in review); it is not a portable `sha256sum -c` artifact.

    Both are verified the same way, with `//mbo/digest:digest --check`. Supply
    either or both; at least one entry is required.

    Args:
        name:      Name of the test.
        algorithm: Digest algorithm to verify with (any `//mbo/digest:digest -a`
                   name, default `sha256`); must match how the digests were made.
        digests:   Dict mapping each file to its checksum sidecar file.
        checksums: Dict mapping each file to its expected hex digest (inline).
        **kwargs:  Passed through to the underlying test rule (tags, etc.).
    """
    if not digests and not checksums:
        fail("verify_digest_test needs at least one `digests` or `checksums` entry.")
    _verify_digest_test(
        name = name,
        algorithm = algorithm,
        sidecar_files = list(digests.keys()),
        sidecars = list(digests.values()),
        checksums = checksums,
        **kwargs
    )

def _verify_digest_test_impl(ctx):
    # The sidecars embed workspace-relative paths, so `--check` must run from the
    # workspace runfiles root - exactly where Bazel starts a test - and resolve
    # each listed file there. `sidecar_files` are carried in the runfiles for that.
    inputs = list(ctx.files.sidecar_files) + list(ctx.files.sidecars)
    check_paths = [sidecar.short_path for sidecar in ctx.files.sidecars]

    # Inline checksums: materialize the same `<hash>  <file>` line and check it
    # identically, so both forms share exactly one `digest --check` path.
    for index, (target, hexdigest) in enumerate(ctx.attr.checksums.items()):
        file = target.files.to_list()[0]
        sidecar = ctx.actions.declare_file("{}.inline{}.sum".format(ctx.label.name, index))
        ctx.actions.write(output = sidecar, content = "{}  {}\n".format(hexdigest, file.short_path))
        inputs.append(file)
        inputs.append(sidecar)
        check_paths.append(sidecar.short_path)

    if not check_paths:
        fail("verify_digest_test needs at least one file to check.")

    checks = "\n".join([
        '"${{DIGEST}}" --algorithm={algorithm} --check "{sidecar}" || sidecar="{sidecar}"'.format(
            algorithm = ctx.attr.algorithm,
            sidecar = sidecar,
        )
        for sidecar in check_paths
    ])

    test_bin = ctx.actions.declare_file(ctx.label.name + "-test.sh")
    ctx.actions.write(
        output = test_bin,
        content = r"""#!/usr/bin/env bash
set -euo pipefail

cd "${{TEST_SRCDIR}}/${{TEST_WORKSPACE}}"
DIGEST="{digest_tool}"

rc=0
sidecar=""
{checks}
if [[ -n "${{sidecar}}" ]]; then
  rc=1
  input="$(head -n 1 ${{sidecar}} | cut -d' ' -f3-)"
  echo >&2 "FAIL: a file no longer matches its committed {algorithm} digest; to regenerate the saved digest(s):"
  echo >&2 "  bazel run //mbo/digest -- -a {algorithm} --cwd=\"\${{PWD}}\" '${{input}}'"
fi
exit "${{rc}}"
""".format(
            digest_tool = ctx.executable._digest_tool.short_path,
            algorithm = ctx.attr.algorithm,
            checks = checks,
        ),
        is_executable = True,
    )
    return DefaultInfo(
        executable = test_bin,
        files = depset(direct = [test_bin]),
        runfiles = ctx.runfiles(files = [test_bin, ctx.executable._digest_tool] + inputs),
    )

_verify_digest_test = rule(
    attrs = {
        "algorithm": attr.string(
            default = "sha256",
            doc = "Digest algorithm to verify with (any `//mbo/digest:digest -a` name).",
        ),
        "checksums": attr.label_keyed_string_dict(
            allow_files = True,
            doc = "Files mapped to their expected hex digest (inline, no sidecar file).",
        ),
        "sidecar_files": attr.label_list(
            allow_files = True,
            doc = "The files verified via `sidecars` (carried in the runfiles).",
        ),
        "sidecars": attr.label_list(
            allow_files = True,
            doc = "The checksum sidecars (coreutils `<hash>  <file>` lines) to check against.",
        ),
        "_digest_tool": attr.label(
            doc = "The digest tool executable.",
            default = Label("//mbo/digest:digest"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    test = True,
    implementation = _verify_digest_test_impl,
    provides = [DefaultInfo],
)
