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
load("//mbo/diff:diff.bzl", "bool_arg", "diff_test")

def diff_test_test(
        name,
        file_old,
        file_new,
        expected_diff,
        algorithm = "",
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
        expected_diff:            The expected diff result.
        algorithm:                Algorithm to use ('unified', 'direct', etc).
        ignore_all_space:         Ignore all leading, trailing, and consecutive internal whitespace changes.
        ignore_consecutive_space: Ignore all whitespace changes, even if one line has whitespace where the other line has none.
        ignore_blank_lines:       Ignore chunks which include only blank lines.
        ignore_case:              Whether to ignore letter case.
        ignore_matching_chunks:   Whether `ignore_matching_lines` applies to chanks or single lines.
        ignore_matching_lines:    Ignore lines that match this regexp (https://github.com/google/re2/wiki/Syntax).
        ignore_trailing_space:    Ignore traling whitespace changes.
        regex_replace_lhs:        Regular expression and replacement for left side:  <sep><regex><sep><replace><sep>.
        regex_replace_rhs:        Regular expression and replacement for right side: <sep><regex><sep><replace><sep>.
        strip_comments:           Strip out anything starting from `strip_comments`.
        strip_parsed_comments:    Whether to parse lines when stripping comments.
        **kwargs:                 Keyword args to pass down to native rules.
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
                --algorithm={algorithm} \\
                --ignore_all_space={ignore_all_space} \\
                --ignore_consecutive_space={ignore_consecutive_space} \\
                --ignore_blank_lines={ignore_blank_lines} \\
                --ignore_case={ignore_case} \\
                --ignore_matching_chunks={ignore_matching_chunks} \\
                --ignore_matching_lines={ignore_matching_lines} \\
                --ignore_trailing_space={ignore_trailing_space} \\
                --regex_replace_lhs={regex_replace_lhs} \\
                --regex_replace_rhs={regex_replace_rhs} \\
                --strip_comments={strip_comments} \\
                --strip_file_header_prefix="external/(com_)?helly25_mbo[^/]*/" \\
                --strip_parsed_comments={strip_parsed_comments} \\
                || true
        """.format(
            file_old = file_old,
            file_new = file_new,
            algorithm = shell.quote(algorithm),
            ignore_all_space = bool_arg(ignore_all_space),
            ignore_consecutive_space = bool_arg(ignore_consecutive_space),
            ignore_blank_lines = bool_arg(ignore_blank_lines),
            ignore_case = bool_arg(ignore_case),
            ignore_matching_chunks = bool_arg(ignore_matching_chunks),
            ignore_matching_lines = shell.quote(ignore_matching_lines),
            ignore_trailing_space = bool_arg(ignore_trailing_space),
            regex_replace_lhs = shell.quote(regex_replace_lhs),
            regex_replace_rhs = shell.quote(regex_replace_rhs),
            strip_comments = shell.quote(strip_comments),
            strip_parsed_comments = bool_arg(strip_parsed_comments),
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
