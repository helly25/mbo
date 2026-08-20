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
    functions: list[int] = field(default_factory=list)
    branches: list[tuple[int, bool]] = field(default_factory=list)


def _repo_path(value: str) -> str | None:
    value = value.replace("\\", "/")
    marker = "/mbo/"
    if value.startswith("mbo/"):
        return value
    if marker in value:
        return "mbo/" + value.split(marker, 1)[1]
    return None


def parse_lcov(path: Path) -> dict[str, FileCoverage]:
    result: dict[str, FileCoverage] = {}
    current: FileCoverage | None = None
    for raw in path.read_text(encoding="utf-8").splitlines():
        if raw.startswith("SF:"):
            name = _repo_path(raw[3:])
            current = result.setdefault(name, FileCoverage()) if name else None
        elif current is not None and raw.startswith("DA:"):
            line, hits, *_ = raw[3:].split(",")
            current.lines[int(line)] = current.lines.get(int(line), 0) + int(hits)
        elif current is not None and raw.startswith("FNDA:"):
            hits, _ = raw[5:].split(",", 1)
            current.functions.append(int(hits))
        elif current is not None and raw.startswith("BRDA:"):
            line, _, _, taken = raw[5:].split(",")
            current.branches.append((int(line), taken not in ("-", "0")))
        elif raw == "end_of_record":
            current = None
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
            values["functions"][0] += sum(hits > 0 for hits in data.functions)
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


def failures(measured: dict, policy: dict) -> list[str]:
    result = []
    thresholds = {"overall": policy.get("minimum", {})}
    thresholds.update({k: v.get("minimum", {}) for k, v in policy.get("categories", {}).items()})
    for category, limits in thresholds.items():
        for metric, minimum in limits.items():
            actual = measured[category][metric]["percent"]
            if actual is None or actual < minimum:
                result.append(f"{category} {metric}: {actual}% < {minimum}%")
    return result


def markdown(measured: dict) -> str:
    rows = ["| Category | Lines | Functions | Branches |", "|---|---:|---:|---:|"]
    for category, metrics in measured.items():
        cells = []
        for metric in ("lines", "functions", "branches"):
            value = metrics[metric]
            cells.append("n/a" if value["percent"] is None else f'{value["percent"]:.2f}% ({value["covered"]}/{value["total"]})')
        rows.append(f"| {category} | " + " | ".join(cells) + " |")
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
    text = markdown(measured)
    patch_failures: list[str] = []
    if args.base_ref:
        patch = counts(files, changed_lines(args.base_ref))
        text += "\n### Changed coverable lines\n\n" + markdown({"patch": patch})
        minimum = policy.get("patch_minimum", {})
        for metric in ("lines", "branches"):
            actual = patch[metric]["percent"]
            if patch[metric]["total"] and actual < minimum.get(metric, 0):
                patch_failures.append(f"patch {metric}: {actual}% < {minimum[metric]}%")
    print(text, end="")
    if args.summary:
        args.summary.write_text(text, encoding="utf-8")
    errors = failures(measured, policy) + patch_failures
    for error in errors:
        print(f"coverage threshold failed: {error}", file=sys.stderr)
    return bool(errors)


if __name__ == "__main__":
    sys.exit(main())
