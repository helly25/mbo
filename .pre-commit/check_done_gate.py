#!/usr/bin/env python3
"""Fail unless main.yml's `done` gate `needs` every other job (no silent escape)."""

import sys

import yaml

WF, GATE = ".github/workflows/main.yml", "done"

jobs = yaml.safe_load(open(WF))["jobs"]
needs = jobs[GATE].get("needs") or []
needs = {needs} if isinstance(needs, str) else set(needs)
missing = (set(jobs) - {GATE}) - needs

for job in sorted(missing):
    print(f"::error file={WF}::job '{job}' is missing from {GATE}.needs")
if missing:
    sys.exit(1)
print(f"OK: {GATE}.needs covers all {len(set(jobs)) - 1} jobs")
