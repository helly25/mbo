#!/usr/bin/env python3
"""Apply the repository coverage policy to a Bazel LCOV report."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class FileCoverage:
    lines: dict[int, int] = field(default_factory=dict)
    functions: list[tuple[int, int]] = field(default_factory=list)
    branches: list[tuple[int, bool]] = field(default_factory=list)


def _repo_path(value: str) -> str | None:
    value = value.replace("\\", "/")
    marker = "/mbo/"
    if value.startswith("mbo/"):
        return value
    if marker in value:
        return "mbo/" + value.split(marker, 1)[1]
    return None


def parse_lcov(path: Path, source_root: Path = Path(".")) -> dict[str, FileCoverage]:
    result: dict[str, FileCoverage] = {}
    current: FileCoverage | None = None
    function_lines: dict[str, int] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        if raw.startswith("SF:"):
            name = _repo_path(raw[3:])
            current = result.setdefault(name, FileCoverage()) if name else None
            function_lines = {}
        elif current is not None and raw.startswith("FN:"):
            definition = raw[3:].split(",")
            function_lines[definition[-1]] = int(definition[0])
        elif current is not None and raw.startswith("DA:"):
            line, hits, *_ = raw[3:].split(",")
            current.lines[int(line)] = current.lines.get(int(line), 0) + int(hits)
        elif current is not None and raw.startswith("FNDA:"):
            hits, name = raw[5:].split(",", 1)
            current.functions.append((function_lines.get(name, 0), int(hits)))
        elif current is not None and raw.startswith("BRDA:"):
            line, _, _, taken = raw[5:].split(",")
            current.branches.append((int(line), taken not in ("-", "0")))
        elif raw == "end_of_record":
            current = None
    for name, data in result.items():
        source = source_root / name
        if not source.is_file():
            continue
        source_lines = source.read_text(encoding="utf-8").splitlines()
        excluded_lines = {
            number
            for number, line in enumerate(source_lines, start=1)
            if "LCOV_EXCL_LINE" in line
        }
        excluded_branches = excluded_lines | {
            number
            for number, line in enumerate(source_lines, start=1)
            if "LCOV_EXCL_BR_LINE" in line
        }
        excluded_functions: set[int] = set()
        for index, line in enumerate(source_lines):
            if "LCOV_EXCL_FUNC_LINE" not in line:
                continue
            for continuation in range(index, len(source_lines)):
                excluded_functions.add(continuation + 1)
                if "{" in source_lines[continuation]:
                    break
        data.lines = {line: hits for line, hits in data.lines.items() if line not in excluded_lines}
        data.functions = [function for function in data.functions if function[0] not in excluded_functions]
        data.branches = [(line, taken) for line, taken in data.branches if line not in excluded_branches]
    return result


def _matches(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def select_files(report: dict[str, FileCoverage], policy: dict) -> dict[str, FileCoverage]:
    include = policy.get("include", ["mbo/**"])
    exclude = policy.get("exclude", [])
    return {
        path: data
        for path, data in report.items()
        if _matches(path, include) and not _matches(path, exclude)
    }


def counts(files: dict[str, FileCoverage], changed: dict[str, set[int]] | None = None) -> dict:
    values = {"lines": [0, 0], "functions": [0, 0], "branches": [0, 0]}
    for path, data in files.items():
        lines = changed.get(path, set()) if changed is not None else None
        for line, hits in data.lines.items():
            if lines is None or line in lines:
                values["lines"][1] += 1
                values["lines"][0] += hits > 0
        if changed is None:
            values["functions"][1] += len(data.functions)
            values["functions"][0] += sum(hits > 0 for _, hits in data.functions)
        for line, taken in data.branches:
            if lines is None or line in lines:
                values["branches"][1] += 1
                values["branches"][0] += taken
    return {
        metric: {
            "covered": value[0],
            "total": value[1],
            "percent": round(100.0 * value[0] / value[1], 2) if value[1] else None,
        }
        for metric, value in values.items()
    }


def changed_lines(base: str) -> dict[str, set[int]]:
    diff = subprocess.run(
        ["git", "diff", "--unified=0", f"{base}...HEAD", "--", "mbo"],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout
    changed: dict[str, set[int]] = defaultdict(set)
    path: str | None = None
    for line in diff.splitlines():
        if line.startswith("+++ b/"):
            path = line[6:]
        elif path and line.startswith("@@"):
            match = re.search(r"\+(\d+)(?:,(\d+))?", line)
            if match:
                start, length = int(match.group(1)), int(match.group(2) or 1)
                changed[path].update(range(start, start + length))
    return changed


def measurements(files: dict[str, FileCoverage], policy: dict) -> dict:
    result = {"overall": counts(files)}
    for name, category in policy.get("categories", {}).items():
        selected = {p: d for p, d in files.items() if _matches(p, category["include"])}
        result[name] = counts(selected)
    return result


def thresholds(policy: dict) -> tuple[dict, dict]:
    """Returns enforcement floors and health targets for every report row."""
    overall = policy.get("minimum", {})
    floors = {"overall": overall}
    targets = {"overall": overall}
    for name, category in policy.get("categories", {}).items():
        floors[name] = overall | category.get("minimum", {})
        targets[name] = overall
    return floors, targets


def failures(measured: dict, minimums: dict) -> list[str]:
    result = []
    for category, limits in minimums.items():
        for metric, minimum in limits.items():
            actual = measured[category][metric]["percent"]
            if actual is None or actual < minimum:
                result.append(f"{category} {metric}: {actual}% < {minimum}%")
    return result


def coverage_status(metrics: dict, minimum: dict, target: dict) -> str:
    if not minimum:
        return "N/A"
    abbreviations = {"lines": "L", "functions": "F", "branches": "B"}
    no_data = []
    failed = []
    low = []
    for metric in ("lines", "functions", "branches"):
        if metric not in minimum:
            continue
        actual = metrics[metric]["percent"]
        if actual is None:
            no_data.append(abbreviations[metric])
        elif actual < minimum[metric]:
            failed.append(abbreviations[metric])
        elif actual < target.get(metric, minimum[metric]):
            low.append(abbreviations[metric])
    problems = []
    if no_data:
        problems.append(f'NO DATA: {"/".join(no_data)}')
    if failed:
        problems.append(f'FAIL: {"/".join(failed)}')
    if low:
        problems.append(f'LOW: {"/".join(low)}')
    return "OK" if not problems else f'**{"; ".join(problems)}**'


def has_coverage(metrics: dict, names: tuple[str, ...]) -> bool:
    return any(metrics[name]["total"] for name in names)


def uncovered_patch_locations(
    files: dict[str, FileCoverage], changed: dict[str, set[int]]
) -> tuple[list[str], list[str]]:
    lines = []
    branches = set()
    for path, data in files.items():
        changed_in_file = changed.get(path, set())
        lines.extend(f"{path}:{line}" for line, hits in data.lines.items() if line in changed_in_file and not hits)
        branches.update(
            f"{path}:{line}" for line, taken in data.branches if line in changed_in_file and not taken
        )
    return sorted(lines), sorted(branches)


def markdown(measured: dict, minimums: dict, targets: dict | None = None) -> str:
    targets = minimums if targets is None else targets
    headers = (
        "Category",
        "Status",
        "Lines",
        "Covered",
        "Total",
        "Functions",
        "Covered",
        "Total",
        "Branches",
        "Covered",
        "Total",
    )
    values = []
    for category, metrics in measured.items():
        cells = [
            category,
            coverage_status(metrics, minimums.get(category, {}), targets.get(category, {})),
        ]
        for metric in ("lines", "functions", "branches"):
            value = metrics[metric]
            percent = "n/a" if value["percent"] is None else f'{value["percent"]:.2f}%'
            cells.extend((percent, str(value["covered"]), str(value["total"])))
        values.append(tuple(cells))
    widths = tuple(max(len(header), *(len(row[index]) for row in values)) for index, header in enumerate(headers))

    def row(cells: tuple[str, ...]) -> str:
        padded = [cell.ljust(width) for cell, width in zip(cells[:2], widths[:2])]
        padded.extend(cell.rjust(width) for cell, width in zip(cells[2:], widths[2:]))
        return "| " + " | ".join(padded) + " |"

    separators = (
        "-" * widths[0],
        "-" * widths[1],
        *("-" * (width - 1) + ":" for width in widths[2:]),
    )
    rows = [row(headers), row(separators), *(row(value) for value in values)]
    return "\n".join(rows) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lcov", type=Path, required=True)
    parser.add_argument("--policy", type=Path, required=True)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--write-baseline", action="store_true")
    parser.add_argument("--base-ref")
    parser.add_argument("--summary", type=Path)
    args = parser.parse_args(argv)
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    files = select_files(parse_lcov(args.lcov), policy)
    measured = measurements(files, policy)
    if not files:
        print("coverage report contains no files selected by policy", file=sys.stderr)
        return 2
    if args.baseline and args.write_baseline:
        baseline = {
            "schema": 1,
            "description": "Bazel LCOV with GCC 14; scope and exclusions are defined by coverage_policy.json",
            "measurements": measured,
        }
        args.baseline.write_text(json.dumps(baseline, indent=2) + "\n", encoding="utf-8")
    minimums, targets = thresholds(policy)
    text = markdown(measured, minimums, targets)
    patch_failures: list[str] = []
    uncovered_lines: list[str] = []
    uncovered_branches: list[str] = []
    if args.base_ref:
        changed = changed_lines(args.base_ref)
        patch = counts(files, changed)
        minimum = policy.get("patch_minimum", {})
        if has_coverage(patch, ("lines", "branches")):
            text += "\n### Changed coverable lines\n\n" + markdown({"patch": patch}, {"patch": minimum})
            for metric in ("lines", "branches"):
                actual = patch[metric]["percent"]
                if patch[metric]["total"] and actual < minimum.get(metric, 0):
                    patch_failures.append(f"patch {metric}: {actual}% < {minimum[metric]}%")
            if patch_failures:
                uncovered_lines, uncovered_branches = uncovered_patch_locations(files, changed)
    print(text, end="")
    if args.summary:
        args.summary.write_text(text, encoding="utf-8")
    errors = failures(measured, minimums) + patch_failures
    for error in errors:
        print(f"coverage threshold failed: {error}", file=sys.stderr)
    for location in uncovered_lines:
        print(f"uncovered patch line: {location}", file=sys.stderr)
    for location in uncovered_branches:
        print(f"uncovered patch branch: {location}", file=sys.stderr)
    return bool(errors)


if __name__ == "__main__":
    sys.exit(main())
