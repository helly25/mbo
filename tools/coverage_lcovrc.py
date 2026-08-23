#!/usr/bin/env python3
"""Generate genhtml color thresholds from mbo's coverage policy."""

import argparse
import json
from pathlib import Path
from typing import Any

import coverage_policy


_METRICS = ("line", "function", "branch")
_POLICY_KEYS = {"line": "lines", "function": "functions", "branch": "branches"}


def render(policy: dict[str, Any]) -> str:
    overall = coverage_policy.overall(policy)
    lines: list[str] = []
    for metric in _METRICS:
        key = _POLICY_KEYS[metric]
        lines.append(f"genhtml_{metric}_hi_limit = {overall[key].target:g}")
        lines.append(f"genhtml_{metric}_med_limit = {overall[key].minimum:g}")
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("policy", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(render(json.loads(args.policy.read_text(encoding="utf-8"))), encoding="utf-8")


if __name__ == "__main__":
    main()
