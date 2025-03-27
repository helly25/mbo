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

"""Macros to load external repositories."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", _http_archive = "http_archive")

def http_archive(name, **kwargs):
    """Wraper for `native.http_archive` that will load the archive only once.

    Args:
        name: Name of the repository.
        **kwargs: Keyword args to pass down to native rules.
    """
    if not native.existing_rule(name):
        _http_archive(
            name = name,
            **kwargs
        )

def github_archive(
        name,
        repo,
        commit = "",
        tag = "",
        sha256 = "",
        strip_prefix = "",
        ext = ".tar.gz",
        **kwargs):
    """Fetches the (github) archive for either `tag` or `commit`.

    Args:
        name: The name of the repository. Will become `@<name>`.
        repo: The (github) repository (e.g. `https://github.com/<foo>/<bar>`).
        commit: The commit hash to reference (cannot be combined with `tag`).
        tag: The release tag to reference (cannot be combined with `commit`).
        sha256: The hash code of the downloaded archive (`shasum -a 256 <pkg>`).
        strip_prefix: Automatically generated if `None`, otherwise the prefix to
                      strip (only allowed for `tag`).
        ext: Extension of the archive ('.tar.gz', '.zip').
        **kwargs: Keyword args to pass down to native rules.
    """
    if commit and tag:
        fail("Either `commit` or `tag` must be specified, not both.")
    if commit:
        if strip_prefix:
            fail("Parameter `strip_prefix` not supported for `commit` archives.")
        _github_commit_archive(name, repo, commit, sha256, ext, **kwargs)
    elif tag:
        _github_tag_archive(name, repo, tag, sha256, strip_prefix, **kwargs)
    else:
        fail("Either `commit` or `tag` must be specified.")

def _github_commit_archive(
        name,
        repo,
        commit,
        sha256 = "",
        ext = ".tar.gz",
        **kwargs):
    """Fetches an archived (github) commit as an external repository.

    The repository archive will only be loaded the first time.

    Example:

    ```
    github_archive(
        name = "com_google_absl",
        repo = "https://github.com/abseil/abseil-cpp",
        commit = "78be63686ba732b25052be15f8d6dee891c05749",
        sha256 = "bd6ce0bffe724241dbfe53013b8fd05b43f9a277fb7fafd99377d7ef35285866",
    ```

    Args:
        name: The name of the repository. Will become `@<name>`.
        repo: The (github) repository (e.g. `https://github.com/<foo>/<bar>`).
        commit: The commit hash to reference.
        sha256: The hash code of the downloaded archive (`shasum -a 256 <pkg>`).
        ext: Extension of the archive ('.tar.gz', '.zip').
        **kwargs: Keyword args to pass down to native rules.
    """
    if not native.existing_rule(name):
        repo_name = repo.split("/")[-1]
        _http_archive(
            name = name,
            sha256 = sha256,
            strip_prefix = repo_name + "-" + commit,
            url = repo + "/archive/" + commit + ext,
            **kwargs
        )

def _github_tag_archive(
        name,
        repo,
        tag,
        sha256 = "",
        strip_prefix = "",
        ext = ".tar.gz",
        **kwargs):
    """Fetches a (github) tagged release as an external repository.

    The repository archive will only be loaded the first time.

    Example:

    ```
    github_archive(
        name = "com_google_absl",
        repo = "https://github.com/abseil/abseil-cpp",
        tag = "20230125.3",
        sha256 = "5366d7e7fa7ba0d915014d387b66d0d002c03236448e1ba9ef98122c13b35c36",
    ```
    Args:
        name: The name of the repository. Will become `@<name>`.
        repo: The (github) repository (e.g. `https://github.com/<foo>/<bar>`).
        tag: The release tag to reference.
        sha256: The hash code of the downloaded archive (`shasum -a 256 <pkg>`).
        strip_prefix: Automatically generated if `None`, otherwise the prefix to strip.
        ext: Extension of the archive ('.tar.gz', '.zip').
        **kwargs: Keyword args to pass down to native rules.
    """
    if not native.existing_rule(name):
        repo_name = repo.split("/")[-1]
        if not strip_prefix:
            strip_prefix = repo_name + "-" + tag
        _http_archive(
            name = name,
            sha256 = sha256,
            strip_prefix = strip_prefix,
            canonical_id = tag,
            url = repo + "/archive/refs/tags/" + tag + ext,
            **kwargs
        )
