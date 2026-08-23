#!/usr/bin/env python3
"""Add mbo's policy legend and navigation to a genhtml report."""

import argparse
import html
import json
import re
from pathlib import Path, PurePosixPath
from typing import Any, Optional

import coverage_policy


_TITLE_RE = re.compile(r'(?P<title>\s*<tr><td class="title">.*?</td></tr>)')
_HEADER_END_RE = re.compile(
    r'(?P<end>\s*<tr><td class="ruler"><img src="[^"]*glass\.png" width=3 height=3 alt=""></td></tr>\s*</table>)'
)
_NAVIGATION_RE = re.compile(r'\s*<tr><td class="mboNavigation">.*?</td></tr>')
_POLICY_TABLE_RE = re.compile(r'\s*<table class="mboPolicy".*?</table>\s*', re.DOTALL)
_STYLE_RE = re.compile(r'\s*<style>\s*\.mboNavigation\b.*?\.mboPolicy\b.*?</style>\s*', re.DOTALL)
_LEGACY_POLICY_RE = re.compile(
    r'\s*<tr>\s*<td class="headerItem">Coverage policy:</td>.*?</tr>\s*', re.DOTALL
)
_HEADER_SUMMARY_RE = re.compile(r'<table cellpadding=1 border=0 width="100%">')
_HEADER_RATE_RE = re.compile(
    r'(?P<label><td class="headerItem">(?P<metric>Lines|Branches|Functions):</td>\s*)'
    r'<td class="headerCovTableEntry(?:Hi|Med|Lo)">(?P<rate>[0-9.]+)&nbsp;%</td>'
)
_TABLE_RATE_RE = re.compile(
    r'<td class="(?P<owner>owner_)?coverPer(?P<class>Hi|Med|Lo)">(?P<rate>[0-9.]+&nbsp;%|-)</td>'
)
_STYLE = """  <style>
    .mboNavigation { padding: .35rem 0; text-align: center; }
    .mboHeaderSummary { margin: 0 auto; width: 80%; }
    .mboPolicy { border-collapse: collapse; margin: .75rem auto 1rem; width: auto; }
    .mboPolicy th { padding: .2rem .75rem; text-align: left; }
    .mboPolicy td { font-size: 100%; padding: .2rem .75rem; text-align: center; }
  </style>
"""


def legend(policy: dict[str, Any]) -> str:
    overall = coverage_policy.overall(policy)
    rows: list[str] = []
    for label, key in (("Lines", "lines"), ("Branches", "branches"), ("Functions", "functions")):
        floor = overall[key].minimum
        goal = overall[key].target
        if floor == goal:
            medium = "-"
        else:
            medium = f"&ge; {floor:g}% and &lt; {goal:g}%"
        rows.append(
            "            <tr>\n"
            f'              <th scope="row">{label}</th>\n'
            f'              <td class="headerValueLegL">&lt; {floor:g}%</td>\n'
            f'              <td class="headerValueLegM">{medium}</td>\n'
            f'              <td class="headerValueLegH">&ge; {goal:g}%</td>\n'
            f'              <td class="headerValueLeg{overall[key].enforce[0].upper()}">'
            f'{overall[key].enforce.title()}</td>\n'
            "            </tr>\n"
        )
    return (
        '          <table class="mboPolicy">\n'
        "            <thead>\n"
        "              <tr>\n"
        '                <th scope="col">Global coverage policy</th>\n'
        '                <th scope="col">Low</th>\n'
        '                <th scope="col">Medium</th>\n'
        '                <th scope="col">High</th>\n'
        '                <th scope="col">Enforced</th>\n'
        "              </tr>\n"
        "            </thead>\n"
        "            <tbody>\n"
        + "".join(rows)
        + "            </tbody>\n"
        '          </table>\n'
    )


def _rate_class(rate: float, medium: int, high: int) -> str:
    if rate >= high:
        return "Hi"
    if rate >= medium:
        return "Med"
    return "Lo"


def _normalize_rate_classes(text: str, policy: dict[str, Any]) -> str:
    overall = coverage_policy.overall(policy)

    def header(match: re.Match[str]) -> str:
        key = match.group("metric").lower()
        rate = float(match.group("rate"))
        suffix = _rate_class(rate, overall[key].minimum, overall[key].target)
        return f'{match.group("label")}<td class="headerCovTableEntry{suffix}">{match.group("rate")}&nbsp;%</td>'

    text = _HEADER_RATE_RE.sub(header, text)
    matches = list(_TABLE_RATE_RE.finditer(text))
    headings = re.findall(r">(Line|Branch|Function) Coverage\b", text)
    metrics = tuple(
        {"Line": "lines", "Branch": "branches", "Function": "functions"}[heading]
        for heading in headings
    )
    # Small synthetic fixtures and source pages may not carry the directory
    # table headings. Their complete metric rows retain genhtml's fixed order.
    if not metrics and matches:
        metrics = ("lines", "branches", "functions")
    if metrics and len(matches) % len(metrics) != 0:
        raise ValueError(
            f"genhtml coverage-rate columns do not match {metrics}: found {len(matches)}"
        )
    parts: list[str] = []
    start = 0
    for index, match in enumerate(matches):
        key = metrics[index % len(metrics)]
        rate_text = match.group("rate")
        suffix = match.group("class")
        if rate_text != "-":
            rate = float(rate_text.removesuffix("&nbsp;%"))
            suffix = _rate_class(rate, overall[key].minimum, overall[key].target)
        owner = match.group("owner") or ""
        parts.extend((text[start : match.start()], f'<td class="{owner}coverPer{suffix}">{rate_text}</td>'))
        start = match.end()
    parts.append(text[start:])
    return "".join(parts)


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


def _footer_start(text: str) -> Optional[int]:
    marker = text.rfind('<tr><td class="versionInfo">')
    if marker < 0:
        return None
    start = text.rfind("<table", 0, marker)
    if start < 0:
        return None
    return text.rfind("\n", 0, start) + 1


def apply(report: Path, policy: dict[str, Any], target: str) -> None:
    _target_parts(target)
    modified = 0
    for path in report.rglob("*.html"):
        text = path.read_text(encoding="utf-8")
        # Make regeneration idempotent and migrate reports produced by the
        # earlier header-row injector without leaving two policy legends.
        text = _STYLE_RE.sub("\n", text)
        text = _NAVIGATION_RE.sub("", text)
        text = _POLICY_TABLE_RE.sub("\n", text)
        text = _LEGACY_POLICY_RE.sub("", text)
        text = _HEADER_SUMMARY_RE.sub('<table class="mboHeaderSummary" cellpadding=1 border=0>', text, count=1)
        title = _TITLE_RE.search(text)
        header_end = _HEADER_END_RE.search(text)
        footer_start = _footer_start(text)
        if not title or not header_end or footer_start is None:
            raise ValueError(f"genhtml header anchors not found in {path}")
        text = text[: title.end()] + _navigation(report, path, target) + text[title.end() :]
        footer_start = _footer_start(text)
        assert footer_start is not None
        text = text[:footer_start] + legend(policy) + "\n" + text[footer_start:]
        if "</head>" not in text:
            raise ValueError(f"genhtml head anchor not found in {path}")
        text = _normalize_rate_classes(text, policy)
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
