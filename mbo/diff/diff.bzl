# Copyright 2023 M.Boerger
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

"""Macro `diff_test` allows to compare two files."""

load("@bazel_skylib//lib:shell.bzl", "shell")

def diff_test(
        name,
        file_old,
        file_new,
        failure_message = "",
        **kwargs):
    """Create a diff test that compares `file_old` vs `file_new`.

    If the test detects differences, then the output will be in `diff -du` form.

    Args:
        name: Name of the test
        file_old: The old file.
        file_new: The new file.
        failure_message: Additional message to log if the files don't match.
        **kwargs: Keyword args to pass down to native rules.
    """
    _diff_test(
        name = name,
        file_old = file_old,
        file_new = file_new,
        failure_message = failure_message,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
        **kwargs
    )

def _runfiles_path(f):
    if f.root.path:
        return f.path[len(f.root.path) + 1:]  # generated file
    else:
        return f.path  # source file

def _diff_test_impl(ctx):
    if ctx.attr.is_windows:
        fail("Windows not yet supported")
    else:
        test_bin = ctx.actions.declare_file(ctx.label.name + "-test.sh")
        ctx.actions.write(
            output = test_bin,
            content = r"""#!/usr/bin/env bash

set -euo pipefail

OLD="{file_old}"
NEW="{file_new}"
[[ "${{OLD}}" =~ ^external/* ]] && OLD="${{OLD#external/}}" || OLD="${{TEST_WORKSPACE}}/${{OLD}}"
[[ "${{NEW}}" =~ ^external/* ]] && NEW="${{NEW#external/}}" || NEW="${{TEST_WORKSPACE}}/${{NEW}}"
if [[ -d "${{RUNFILES_DIR:-/dev/null}}" && "${{RUNFILES_MANIFEST_ONLY:-}}" != 1 ]]; then
  OLD="${{RUNFILES_DIR}}/${{OLD}}"
  NEW="${{RUNFILES_DIR}}/${{NEW}}"
elif [[ -f "${{RUNFILES_MANIFEST_FILE:-/dev/null}}" ]]; then
  OLD="$(grep -F -m1 "${{OLD}} " "${{RUNFILES_MANIFEST_FILE}}" | sed 's/^[^ ]* //')"
  NEW="$(grep -F -m1 "${{NEW}} " "${{RUNFILES_MANIFEST_FILE}}" | sed 's/^[^ ]* //')"
elif [[ -f "${{TEST_SRCDIR}}/${{OLD}}" && -f "${{TEST_SRCDIR}}/${{NEW}}" ]]; then
  OLD="${{TEST_SRCDIR}}/${{OLD}}"
  NEW="${{TEST_SRCDIR}}/${{NEW}}"
else
  echo >&2 "ERROR: could not find \"{file_old}\" and \"{file_new}\""
  exit 1
fi
if ! diff -du "${{OLD}}" "${{NEW}}"; then
  echo >&2 "FAIL: files \"{file_old}\" and \"{file_new}\" differ. "{failure_message}
  exit 1
fi
""".format(
                failure_message = shell.quote(ctx.attr.failure_message),
                file_old = _runfiles_path(ctx.file.file_old),
                file_new = _runfiles_path(ctx.file.file_new),
            ),
            is_executable = True,
        )
    return DefaultInfo(
        executable = test_bin,
        files = depset(direct = [test_bin]),
        runfiles = ctx.runfiles(files = [
            test_bin,
            ctx.file.file_old,
            ctx.file.file_new,
        ]),
    )

_diff_test = rule(
    attrs = {
        "failure_message": attr.string(),
        "file_old": attr.label(
            allow_single_file = True,
            doc = "The old (left) file of the comparison.",
            mandatory = True,
        ),
        "file_new": attr.label(
            allow_single_file = True,
            doc = "The new (right) file of the comparison.",
            mandatory = True,
        ),
        "is_windows": attr.bool(mandatory = True),
        "_diff_tool": attr.label(
            doc = "The diff tool executable.",
            default = Label("//mbo/diff:unified_diff"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    test = True,
    implementation = _diff_test_impl,
)
