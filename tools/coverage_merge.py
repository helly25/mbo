#!/usr/bin/env python3
"""Overlay supplemental LCOV hits onto an existing production report."""

import argparse
from collections import defaultdict
from pathlib import Path
import re


def _source_key(source: str) -> str:
    source = source.replace("\\", "/")
    return "mbo/" + source.rsplit("/mbo/", 1)[1] if "/mbo/" in source else source


def _records(report: str) -> dict[str, str]:
    if report and not report.endswith("end_of_record\n"):
        raise ValueError("LCOV report does not end with end_of_record")
    result = {}
    for record in report.split("end_of_record\n"):
        match = re.search(r"(?m)^SF:(.+)$", record)
        if match:
            key = _source_key(match.group(1))
            if key in result:
                raise ValueError(f"LCOV report contains duplicate source record: {key}")
            result[key] = record
    return result


def _supplemental_hits(record: str) -> tuple[dict[int, int], dict[str, int], dict[tuple[str, str, str], int]]:
    lines: dict[int, int] = defaultdict(int)
    functions: dict[str, int] = defaultdict(int)
    branches: dict[tuple[str, str, str], int] = defaultdict(int)
    for entry in record.splitlines():
        if entry.startswith("DA:"):
            line, hits, *_ = entry[3:].split(",")
            lines[int(line)] += int(hits)
        elif entry.startswith("FNDA:"):
            hits, name = entry[5:].split(",", 1)
            functions[name] += int(hits)
        elif entry.startswith("BRDA:"):
            line, block, branch, taken = entry[5:].split(",")
            if taken != "-":
                branches[(line, block, branch)] += int(taken)
    return lines, functions, branches


def _overlay_record(primary: str, supplemental: str) -> tuple[str, int]:
    line_hits, function_hits, branch_hits = _supplemental_hits(supplemental)
    result = []
    line_values = []
    function_values = []
    branch_values = []
    added_hits = 0
    for entry in primary.splitlines():
        if entry.startswith("DA:"):
            fields = entry[3:].split(",")
            extra = line_hits[int(fields[0])]
            added_hits += extra
            fields[1] = str(int(fields[1]) + extra)
            entry = "DA:" + ",".join(fields)
            line_values.append(int(fields[1]))
        elif entry.startswith("FNDA:"):
            hits, name = entry[5:].split(",", 1)
            extra = function_hits[name]
            added_hits += extra
            hits = str(int(hits) + extra)
            entry = f"FNDA:{hits},{name}"
            function_values.append(int(hits))
        elif entry.startswith("BRDA:"):
            line, block, branch, taken = entry[5:].split(",")
            extra = branch_hits[(line, block, branch)]
            added_hits += extra
            if extra:
                taken = str((0 if taken == "-" else int(taken)) + extra)
            entry = f"BRDA:{line},{block},{branch},{taken}"
            branch_values.append(0 if taken == "-" else int(taken))
        elif entry.startswith("LF:"):
            entry = f"LF:{len(line_values)}"
        elif entry.startswith("LH:"):
            entry = f"LH:{sum(value > 0 for value in line_values)}"
        elif entry.startswith("FNF:"):
            entry = f"FNF:{len(function_values)}"
        elif entry.startswith("FNH:"):
            entry = f"FNH:{sum(value > 0 for value in function_values)}"
        elif entry.startswith("BRF:"):
            entry = f"BRF:{len(branch_values)}"
        elif entry.startswith("BRH:"):
            entry = f"BRH:{sum(value > 0 for value in branch_values)}"
        result.append(entry)
    return "\n".join(result) + "\n", added_hits


def overlay(primary: str, supplemental: str) -> str:
    """Adds matching supplemental hits without changing production coverpoints."""
    supplemental_records = _records(supplemental)
    if not supplemental_records:
        raise ValueError("supplemental LCOV report contains no source records")
    result = []
    added_hits = 0
    for key, record in _records(primary).items():
        if key in supplemental_records:
            record, record_added_hits = _overlay_record(record, supplemental_records[key])
            added_hits += record_added_hits
        result.append(record + "end_of_record\n")
    if not added_hits:
        raise ValueError("supplemental LCOV report contains no matching production hits")
    return "".join(result)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("primary", type=Path)
    parser.add_argument("supplemental", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(
        overlay(
            args.primary.read_text(encoding="utf-8"),
            args.supplemental.read_text(encoding="utf-8"),
        ),
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
