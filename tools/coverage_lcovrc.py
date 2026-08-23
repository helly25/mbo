#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

"""Generate genhtml color thresholds from mbo's coverage policy."""

import argparse
import json
from pathlib import Path
from typing import Any


_METRICS = ("line", "function", "branch")
_POLICY_KEYS = {"line": "lines", "function": "functions", "branch": "branches"}


def render(policy: dict[str, Any]) -> str:
    minimum = policy["minimum"]
    target = {**minimum, **policy.get("target", {})}
    lines: list[str] = []
    for metric in _METRICS:
        key = _POLICY_KEYS[metric]
        medium = int(minimum[key])
        high = int(target[key])
        if not 0 <= medium <= high <= 100:
            raise ValueError(
                f"{metric} coverage thresholds must satisfy 0 <= minimum <= target <= 100: "
                f"{medium}, {high}"
            )
        lines.append(f"genhtml_{metric}_hi_limit = {high}")
        lines.append(f"genhtml_{metric}_med_limit = {medium}")
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("policy", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(render(json.loads(args.policy.read_text(encoding="utf-8"))), encoding="utf-8")


if __name__ == "__main__":
    main()
