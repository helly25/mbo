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

"""Macro `diff_test` allows to compare two files."""

load("@bazel_skylib//lib:shell.bzl", "shell")

def bool_arg(arg):
    return "1" if arg else "0"

def diff_test(
        name,
        file_old,
        file_new,
        algorithm = "unified",
        failure_message = "",
        ignore_all_space = False,
        ignore_consecutive_space = False,
        ignore_blank_lines = False,
        ignore_case = False,
        ignore_matching_chunks = True,
        ignore_matching_lines = "",
        ignore_trailing_space = False,
        regex_replace_lhs = "",
        regex_replace_rhs = "",
        strip_comments = "",
        strip_parsed_comments = True,
        **kwargs):
    """Create a diff test that compares `file_old` vs `file_new`.

    If the test detects differences, then the output will be in `diff -du` form.

    Args:
        name:                     Name of the test
        file_old:                 The old file.
        file_new:                 The new file.
        algorithm:                Algorithm to use ('unified', 'direct', etc).
        failure_message:          Additional message to log if the files don't match.
        ignore_all_space:         Ignore all leading, trailing, and consecutive internal whitespace changes.
        ignore_consecutive_space: Ignore all whitespace changes, even if one line has whitespace where the other line has none.
        ignore_blank_lines:       Ignore chunks which include only blank lines.
        ignore_case:              Whether to ignore letter case.
        ignore_matching_chunks:   Whether `ignore_matching_lines` applies to chunks or single lines.
        ignore_matching_lines:    Ignore lines that match this regexp (https://github.com/google/re2/wiki/Syntax).
        ignore_trailing_space:    Ignore traling whitespace changes.
        regex_replace_lhs:        Regular expression and replacement for left side:  <sep><regex><sep><replace><sep>.
        regex_replace_rhs:        Regular expression and replacement for right side: <sep><regex><sep><replace><sep>.
        strip_comments:           Strip out anything starting from `strip_comments`.
        strip_parsed_comments:    Whether to parse lines when stripping comments.
        **kwargs:                 Keyword args to pass down to native rules.
    """
    _diff_test(
        name = name,
        file_old = file_old,
        file_new = file_new,
        algorithm = algorithm,
        failure_message = failure_message,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
        ignore_all_space = ignore_all_space,
        ignore_consecutive_space = ignore_consecutive_space,
        ignore_blank_lines = ignore_blank_lines,
        ignore_case = ignore_case,
        ignore_matching_lines = ignore_matching_lines,
        ignore_matching_chunks = ignore_matching_chunks,
        ignore_trailing_space = ignore_trailing_space,
        regex_replace_lhs = regex_replace_lhs,
        regex_replace_rhs = regex_replace_rhs,
        strip_comments = strip_comments,
        strip_parsed_comments = strip_parsed_comments,
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

set -xeuo pipefail

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
if ! {diff_tool} "${{OLD}}" "${{NEW}}" \
    --algorithm={algorithm} \
    --ignore_all_space={ignore_all_space} \
    --ignore_consecutive_space={ignore_consecutive_space} \
    --ignore_blank_lines={ignore_blank_lines} \
    --ignore_case={ignore_case} \
    --ignore_matching_chunks={ignore_matching_chunks} \
    --ignore_matching_lines={ignore_matching_lines} \
    --ignore_trailing_space={ignore_trailing_space} \
    --regex_replace_lhs={regex_replace_lhs} \\
    --regex_replace_rhs={regex_replace_rhs} \\
    --strip_comments={strip_comments} \
    --strip_file_header_prefix="external/com_helly25_mbo/" \
    --strip_parsed_comments={strip_parsed_comments} \
; then
  echo >&2 "FAIL: files \"{file_old}\" and \"{file_new}\" differ. " {failure_message}
  exit 1
fi
""".format(
                diff_tool = ctx.executable._diff_tool.short_path,
                failure_message = shell.quote(ctx.attr.failure_message),
                file_old = _runfiles_path(ctx.file.file_old),
                file_new = _runfiles_path(ctx.file.file_new),
                algorithm = shell.quote(ctx.attr.algorithm),
                ignore_all_space = bool_arg(ctx.attr.ignore_all_space),
                ignore_consecutive_space = bool_arg(ctx.attr.ignore_consecutive_space),
                ignore_blank_lines = bool_arg(ctx.attr.ignore_blank_lines),
                ignore_case = bool_arg(ctx.attr.ignore_case),
                ignore_matching_chunks = bool_arg(ctx.attr.ignore_matching_chunks),
                ignore_matching_lines = shell.quote(ctx.attr.ignore_matching_lines),
                ignore_trailing_space = bool_arg(ctx.attr.ignore_trailing_space),
                regex_replace_lhs = shell.quote(ctx.attr.regex_replace_lhs),
                regex_replace_rhs = shell.quote(ctx.attr.regex_replace_rhs),
                strip_comments = shell.quote(ctx.attr.strip_comments),
                strip_parsed_comments = bool_arg(ctx.attr.strip_parsed_comments),
            ),
            is_executable = True,
        )
    return DefaultInfo(
        executable = test_bin,
        files = depset(direct = [test_bin]),
        runfiles = ctx.runfiles(files = [
            test_bin,
            ctx.executable._diff_tool,
            ctx.file.file_old,
            ctx.file.file_new,
        ]),
    )

_diff_test = rule(
    attrs = {
        "algorithm": attr.string(
            default = "",
            doc = "The diff algorithm to use.",
            values = ["direct", "unified"],
        ),
        "failure_message": attr.string(
            doc = "An extra failure message to append to diff failure lines.",
        ),
        "file_new": attr.label(
            allow_single_file = True,
            doc = "The new (right) file of the comparison.",
            mandatory = True,
        ),
        "file_old": attr.label(
            allow_single_file = True,
            doc = "The old (left) file of the comparison.",
            mandatory = True,
        ),
        "ignore_all_space": attr.bool(
            doc = "Ignore all leading, trailing, and consecutive internal whitespace changes.",
            default = False,
        ),
        "ignore_blank_lines": attr.bool(
            doc = "Ignore chunks which include only blank lines.",
            default = False,
        ),
        "ignore_case": attr.bool(
            doc = "Whether to ignore letter case.",
            default = False,
        ),
        "ignore_consecutive_space": attr.bool(
            doc = "Ignore all whitespace changes, even if one line has whitespace where the other line has none.",
            default = False,
        ),
        "ignore_matching_chunks": attr.bool(
            doc = "Whether `ignore_matching_lines` applies to chanks or single lines.",
            default = True,
        ),
        "ignore_matching_lines": attr.string(
            doc = "Ignore lines that match this regexp (https://github.com/google/re2/wiki/Syntax).",
        ),
        "ignore_trailing_space": attr.bool(
            doc = "Ignore traling whitespace changes.",
            default = False,
        ),
        "is_windows": attr.bool(
            mandatory = True,
        ),
        "regex_replace_lhs": attr.string(
            doc = "Regular expression and replacement for left side:  <sep><regex><sep><replace><sep>.",
            default = "",
        ),
        "regex_replace_rhs": attr.string(
            doc = "Regular expression and replacement for right side:  <sep><regex><sep><replace><sep>.",
            default = "",
        ),
        "strip_comments": attr.string(
            doc = "Strip out any thing starting from `strip_comments`.",
        ),
        "strip_parsed_comments": attr.bool(
            doc = "Whether to parse lines when stripping comments.",
            default = True,
        ),
        "_diff_tool": attr.label(
            doc = "The diff tool executable.",
            default = Label("//mbo/diff"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
    test = True,
    implementation = _diff_test_impl,
    provides = [DefaultInfo],
)
