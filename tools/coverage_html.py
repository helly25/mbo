#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

"""Add mbo's metric-specific policy legend to a genhtml report."""

import argparse
import json
from pathlib import Path
from typing import Any


_ANCHOR = '            <tr><td class="ruler"><img src="glass.png" width=3 height=3 alt=""></td></tr>'


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
                f'<span class="coverLegendCovMed">medium: &gt;= {floor} %</span> '
                f'<span class="coverLegendCovHi">high: &gt;= {goal} %</span>'
            )
        cells.append(f"<b>{label}:</b> {text}")
    return (
        '            <tr>\n'
        '              <td class="headerItem">Coverage policy:</td>\n'
        f'              <td class="headerValue" colspan="6">{" &middot; ".join(cells)}</td>\n'
        '            </tr>\n'
    )


def apply(report: Path, policy: dict[str, Any]) -> None:
    replacement = legend(policy) + _ANCHOR
    modified = 0
    for path in report.rglob("*.html"):
        text = path.read_text(encoding="utf-8")
        if _ANCHOR not in text:
            continue
        path.write_text(text.replace(_ANCHOR, replacement, 1), encoding="utf-8")
        modified += 1
    if modified == 0:
        raise ValueError(f"genhtml ruler anchor not found below {report}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("policy", type=Path)
    args = parser.parse_args()
    apply(args.report, json.loads(args.policy.read_text(encoding="utf-8")))


if __name__ == "__main__":
    main()
