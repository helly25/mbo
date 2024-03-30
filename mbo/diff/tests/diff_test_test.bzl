# SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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
load("//mbo/diff:diff.bzl", "diff_test")

def _bool_arg(arg):
    return "1" if arg else "0"

def diff_test_test(
        name,
        file_old,
        file_new,
        expected_diff,
        ignore_blank_lines = False,
        ignore_case = False,
        ignore_matching_chunks = True,
        ignore_matching_lines = "",
        ignore_space_change = False,
        strip_comments = "",
        strip_parsed_comments = True,
        **kwargs):
    """Create a diff test that compares `file_old` vs `file_new`.

    If the test detects differences, then the output will be in `diff -du` form.

    Args:
        name:                  Name of the test
        file_old:              The old file.
        file_new:              The new file.
        expected_diff:         The expected diff result.
        ignore_blank_lines:    Ignore chunks which include only blank lines.
        ignore_case:           Whether to ignore letter case.
        ignore_matching_chunks:Whether `ignore_matching_lines` applies to chanks or single lines.
        ignore_matching_lines: Ignore lines that match this regexp (https://github.com/google/re2/wiki/Syntax).
        ignore_space_change:   Ignore leading and traling whitespace changes.
        strip_comments:        Strip out anything starting from `strip_comments`.
        strip_parsed_comments: Whether to parse lines when stripping comments.
        **kwargs:              Keyword args to pass down to native rules.
    """
    native.genrule(
        name = name + "_diff",
        srcs = [
            file_old,
            file_new,
        ],
        outs = [name + ".diff"],
        tools = ["//mbo/diff:unified_diff"],
        cmd = """
            $(location //mbo/diff:unified_diff) --skip_time \\
                $(location {file_old}) $(location {file_new}) > $@ \\
                --ignore_blank_lines={ignore_blank_lines} \\
                --ignore_case={ignore_case} \\
                --ignore_matching_chunks={ignore_matching_chunks} \\
                --ignore_matching_lines={ignore_matching_lines} \\
                --ignore_space_change={ignore_space_change} \\
                --strip_comments={strip_comments} \\
                --strip_file_header_prefix="external/com_helly25_mbo/" \\
                --strip_parsed_comments={strip_parsed_comments} \\
                || true
        """.format(
            file_old = file_old,
            file_new = file_new,
            ignore_blank_lines = _bool_arg(ignore_blank_lines),
            ignore_case = _bool_arg(ignore_case),
            ignore_matching_chunks = _bool_arg(ignore_matching_chunks),
            ignore_matching_lines = shell.quote(ignore_matching_lines),
            ignore_space_change = _bool_arg(ignore_space_change),
            strip_comments = shell.quote(strip_comments),
            strip_parsed_comments = _bool_arg(strip_parsed_comments),
        ),
        tags = kwargs.get("tags, None"),
        testonly = True,
        visibility = ["//visibility:private"],
    )
    diff_test(
        name = name,
        file_old = expected_diff,
        file_new = name + ".diff",
        **kwargs
    )
