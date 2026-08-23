#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

"""Add mbo's policy legend and navigation to a genhtml report."""

import argparse
import html
import json
import re
from pathlib import Path, PurePosixPath
from typing import Any


_TITLE_RE = re.compile(r'(?P<title>\s*<tr><td class="title">.*?</td></tr>)')
_HEADER_END_RE = re.compile(
    r'(?P<end>\s*<tr><td class="ruler"><img src="[^"]*glass\.png" width=3 height=3 alt=""></td></tr>\s*</table>)'
)
_NAVIGATION_RE = re.compile(r'\s*<tr><td class="mboNavigation">.*?</td></tr>')
_POLICY_TABLE_RE = re.compile(r'\n\n[ \t]*<table class="mboPolicy".*?</table>\n', re.DOTALL)
_LEGACY_POLICY_RE = re.compile(
    r'\s*<tr>\s*<td class="headerItem">Coverage policy:</td>.*?</tr>\s*', re.DOTALL
)
_STYLE = """  <style>
    .mboNavigation { padding: .35rem 0; text-align: right; }
    .mboPolicy { margin: .35rem 0 1rem; }
  </style>
"""


def legend(policy: dict[str, Any]) -> str:
    minimum = policy["minimum"]
    target = {**minimum, **policy.get("target", {})}
    cells: list[str] = []
    for label, key in (("Lines", "lines"), ("Functions", "functions"), ("Branches", "branches")):
        floor = int(minimum[key])
        goal = int(target[key])
        if floor == goal:
            text = (
                f'<span class="coverLegendCovLo">low: &lt; {floor} %</span> '
                f'<span class="coverLegendCovHi">high: &gt;= {goal} %</span>'
            )
        else:
            text = (
                f'<span class="coverLegendCovLo">low: &lt; {floor} %</span> '
                f'<span class="coverLegendCovMed">medium: &gt;= {floor} % and &lt; {goal} %</span> '
                f'<span class="coverLegendCovHi">high: &gt;= {goal} %</span>'
            )
        cells.append(f"<b>{label}:</b> {text}")
    return (
        '          <table class="mboPolicy" width="100%" border=0 cellspacing=0 cellpadding=0>\n'
        '            <tr>\n'
        '              <td class="headerItem">Coverage policy:</td>\n'
        f'              <td class="headerValue">{" &middot; ".join(cells)}</td>\n'
        '            </tr>\n'
        '          </table>\n'
    )


def _target_parts(target: str) -> tuple[str, ...]:
    path = PurePosixPath(target)
    parts = path.parts
    if (
        not parts
        or path.is_absolute()
        or any(part in (".", "..") or not re.fullmatch(r"[A-Za-z0-9._-]+", part) for part in parts)
    ):
        raise ValueError(f"invalid coverage target: {target!r}")
    return parts


def _navigation(report: Path, page: Path, target: str) -> str:
    depth = len(page.relative_to(report).parent.parts)
    prefix = "../" * depth
    overview = prefix + "../index.html"
    reports = prefix + "../" * (len(_target_parts(target)) + 1) + "index.html"
    return (
        '\n            <tr><td class="mboNavigation">'
        f'<a href="{html.escape(overview, quote=True)}">Report overview</a> &middot; '
        f'<a href="{html.escape(reports, quote=True)}">All coverage reports</a>'
        "</td></tr>\n"
    )


def apply(report: Path, policy: dict[str, Any], target: str) -> None:
    _target_parts(target)
    modified = 0
    for path in report.rglob("*.html"):
        text = path.read_text(encoding="utf-8")
        # Make regeneration idempotent and migrate pages produced by the
        # earlier header-row injector without leaving duplicate UI behind.
        text = text.replace(_STYLE, "")
        text = _NAVIGATION_RE.sub("", text)
        text = _POLICY_TABLE_RE.sub("\n", text)
        text = _LEGACY_POLICY_RE.sub("", text)
        title = _TITLE_RE.search(text)
        header_end = _HEADER_END_RE.search(text)
        if not title or not header_end:
            raise ValueError(f"genhtml header anchors not found in {path}")
        text = text[: title.end()] + _navigation(report, path, target) + text[title.end() :]
        header_end = _HEADER_END_RE.search(text)
        assert header_end is not None
        text = text[: header_end.end()] + "\n\n" + legend(policy) + text[header_end.end() :]
        if "</head>" not in text:
            raise ValueError(f"genhtml head anchor not found in {path}")
        path.write_text(text.replace("</head>", _STYLE + "</head>", 1), encoding="utf-8")
        modified += 1
    if modified == 0:
        raise ValueError(f"no genhtml pages found below {report}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("policy", type=Path)
    parser.add_argument("target")
    args = parser.parse_args()
    apply(args.report, json.loads(args.policy.read_text(encoding="utf-8")), args.target)


if __name__ == "__main__":
    main()
