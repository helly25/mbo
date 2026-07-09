#!/usr/bin/env python3
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

"""Capture, store, render, and plot //mbo/hash:hash_benchmark results.

See mbo/hash/measurements/README.md for the full design. Summary:

- Sub-nanosecond numbers on a shared machine => aggregate by the MEAN OF THE
  k FASTEST reps (k = reps/3): the low tail approximates the uncontended cost
  (contention only adds time) and rejects the single-sample fluke a pure
  minimum keeps; apply google/benchmark precautions (interleaving, warmup).
- Store the full run context (date, git revision + dirty flag, host, CPU,
  load, cpu_scaling) so every number is attributable.
- JSON storage (google/benchmark emits JSON natively; no transcode).

WORKFLOW (squash merges!): a git SHA taken on a feature branch is useless after
the squash merge - the dev commit never reaches main. So authoritative datasets
are measured on the MERGED main commit in a follow-up PR:
    (work, commit, PR, merge) then (measure on main, commit results, PR, merge).
The tool records the SHA/dirty/branch and warns when the checkout is not a clean
main tree.

Pipeline (tables/store/plot are pure JSON -> no bazel needed):
    hash_benchmark_report.py run   --mode full --reps 20 --raw data/RAW.json --out results.json --tables
    hash_benchmark_report.py store --raw data/RAW.json --out results.json
    hash_benchmark_report.py tables --results results.json      # README (fast) subset
    hash_benchmark_report.py plot  --results results.json --out curve.svg
"""

import argparse
import concurrent.futures
import datetime
import gzip
import json
import os
import re
import shlex
import statistics
import subprocess
import sys

# README tables show this (dense) set; the stored FULL dataset holds ~3x more
# (a slow exponential) for the curve. Mirror kReadmeSizes in hash_benchmark.cc.
_README_SIZES = [1, 3, 7, 8, 11, 15, 16, 19, 22, 27, 32, 47, 48, 63, 64, 256, 1024, 4096]

# Ordering uses the benchmark's algorithm keys (data keys); _LABEL_128 only
# renames "mumbo" to "jumbo" for display in the 128-bit table.
_ORDER_64 = ["mumbo", "rapidhash", "xxh3", "xxh64", "murmur3", "siphash24", "fnv1a", "dumbo"]
_ORDER_128 = ["mumbo", "xxh3", "murmur3"]
_LABEL_128 = {"mumbo": "jumbo"}  # BmHash128<mumbo> is the jumbo (128-bit) face

_NAME_RE = re.compile(r"^BmHash(64|128)(Latency)?<([A-Za-z0-9]+)>/(\d+)$")
_BENCHMARK_TARGET = "//mbo/hash:hash_benchmark"

# mbo algorithm -> SMHasher3 registration name(s). The in-house mumbo/jumbo/
# dumbo require a patched SMHasher3 that registers them (see the SMHasher3
# harness in README.md); the third-party names are SMHasher3's built-ins. Names
# are confirmed against `SMHasher3 --list`; override per run with --names.
_SMHASHER_NAMES = {
    "mumbo": ["mumbo-64"],
    "jumbo": ["jumbo-128"],
    "dumbo": ["dumbo-64"],
    "fnv1a": ["FNV-1a"],
    "xxh64": ["XXH-64"],
    "xxh3": ["XXH3-64", "XXH3-128"],
    "rapidhash": ["rapidhash"],
    "siphash": ["SipHash-2-4"],
    "murmur3": ["MurmurHash3"],
}
# Default set - ALL algorithms, explicitly including the legacy `dumbo`.
_SMHASHER_ALL = ["mumbo", "jumbo", "dumbo", "fnv1a", "xxh64", "xxh3", "rapidhash", "siphash", "murmur3"]


def _timestamp():
    """Local wall-clock stamp `YYYYMMDD_HHMMSS` for output filenames."""
    now = datetime.datetime.now()
    return (
        f"{now.year:04}{now.month:02}{now.day:02}_{now.hour:02}{now.minute:02}{now.second:02}"
    )


def _timestamped(path, stamp):
    """Prefix a path's filename with `stamp_`, so every written artifact is uniquely named."""
    directory, name = os.path.split(path)
    return os.path.join(directory, f"{stamp}_{name}")


def _sh(cmd):
    """Best-effort command -> stripped stdout, or None on failure."""
    try:
        return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def _source_provenance():
    """git revision, dirty flag, branch, and whether it is a clean main tree."""
    sha = _sh(["git", "rev-parse", "HEAD"])
    dirty = bool(_sh(["git", "status", "--porcelain"]))
    branch = _sh(["git", "rev-parse", "--abbrev-ref", "HEAD"])
    authoritative = (branch == "main") and not dirty
    return {"git_sha": sha, "git_dirty": dirty, "git_branch": branch, "authoritative": authoritative}


def _machine_augment():
    """CPU brand / OS not always in google/benchmark's context; fill from the OS."""
    brand = _sh(["sysctl", "-n", "machdep.cpu.brand_string"])  # macOS
    if not brand:
        cpuinfo = _sh(["sh", "-c", "grep -m1 'model name' /proc/cpuinfo | cut -d: -f2"])  # linux
        brand = cpuinfo.strip() if cpuinfo else None
    return {"cpu_brand": brand, "uname": _sh(["uname", "-srm"])}


def _run_benchmark(mode, reps, min_time, warmup):
    """Runs the bazel benchmark with the measurement precautions; returns parsed JSON."""
    env = dict(os.environ)
    if mode == "full":
        env["MBO_HASH_BENCHMARK_FULL"] = "1"
    cmd = [
        "bazel",
        "run",
        "-c",
        "opt",
        _BENCHMARK_TARGET,
        "--",
        "--benchmark_format=json",
        f"--benchmark_repetitions={reps}",
        f"--benchmark_min_time={min_time}s",
        f"--benchmark_min_warmup_time={warmup}s",
        "--benchmark_enable_random_interleaving=true",
    ]
    print(f"$ MBO_HASH_BENCHMARK_FULL={env.get('MBO_HASH_BENCHMARK_FULL', '')} {' '.join(cmd)}", file=sys.stderr)
    out = subprocess.run(cmd, capture_output=True, text=True, check=True, env=env).stdout
    return json.loads(out[out.index("{") :])


def distill(raw, mode):
    """Raw google/benchmark JSON -> canonical per-case stats + full context.

    Per (width, algo, length) keeps min/median/mean/stddev/cv/reps computed from
    the per-iteration rows (the tool's own aggregate rows are ignored so we
    control the statistic). `min` is the headline; the rest support error bars
    and flag noisy cases.
    """
    ctx = dict(raw.get("context", {}))
    ctx.update(_machine_augment())
    ctx["source"] = _source_provenance()
    ctx["measurement"] = {"mode": mode, "aggregate": "best-k mean", "reps": None, "best_k": None}

    # Collect per-iteration times per case.
    samples = {"throughput64": {}, "throughput128": {}, "latency": {}}
    section = {"64": "throughput64", "128": "throughput128"}
    for bench in raw.get("benchmarks", []):
        if bench.get("run_type") != "iteration":
            continue
        match = _NAME_RE.match(bench["name"])
        if not match:
            continue
        width, latency, algo, length = match.groups()
        bucket = "latency" if latency else section[width]
        samples[bucket].setdefault(algo, {}).setdefault(length, []).append(float(bench["real_time"]))

    result = {"context": ctx}
    reps_seen = 0
    best_k_seen = 0
    for bucket, algos in samples.items():
        out = result.setdefault(bucket, {})
        for algo, lengths in algos.items():
            for length, times in lengths.items():
                reps_seen = max(reps_seen, len(times))
                # Headline aggregate: mean of the k fastest reps (k ~= reps/3).
                # Contention only ever slows a run, so the low tail is the
                # uncontended cost; averaging the best k rejects the single-
                # sample noise that a pure minimum would keep.
                best_k = max(1, len(times) // 3)
                best_k_seen = max(best_k_seen, best_k)
                ordered = sorted(times)
                best_times = ordered[:best_k]
                best = statistics.fmean(best_times)
                # CV over the SELECTED fastest k (not all reps): how stable the
                # reported estimate is. A full-set CV would just report the
                # contention we already excluded.
                best_sd = statistics.stdev(best_times) if len(best_times) > 1 else 0.0
                out.setdefault(algo, {})[length] = {
                    "best": round(best, 4),
                    "best_cv": round(best_sd / best, 4) if best else 0.0,
                    "min": round(ordered[0], 4),
                    "median": round(statistics.median(times), 4),
                    "mean": round(statistics.fmean(times), 4),
                    "reps": len(times),
                }
    result["context"]["measurement"]["reps"] = reps_seen
    result["context"]["measurement"]["best_k"] = best_k_seen
    return result


def _provenance_context():
    """Shared context block (machine + source) for any stored dataset."""
    ctx = _machine_augment()
    ctx["source"] = _source_provenance()
    return ctx


# SMHasher3's per-test failures and final verdict. Tolerant: SMHasher3 output
# varies by version, so we capture the raw log and parse best-effort.
# Verdict + optional score, e.g. "Overall result: FAIL  ( 181 / 188 )".
_SMH_VERDICT_RE = re.compile(
    r"Overall result[:\s.]*\b(PASS|FAIL)\b(?:[^(]*\(\s*(\d+)\s*/\s*(\d+)\s*\))?", re.IGNORECASE
)
# A failing test line. SMHasher3 flags failures with a `!!!!!` / `********`
# marker or a trailing "FAIL"; capture a leading test/family name where present.
_SMH_FAIL_RE = re.compile(r"^\s*(?:[!*]{3,}\s*)?(?P<name>[\w :.\[\]/,-]+?)\s*[.\s]*\bFAIL(?:ED)?\b", re.IGNORECASE)


def _smhasher_one(cmd_prefix, name, raw_dir, stamp):
    """Run one SMHasher3 battery and parse its verdict + failing families."""
    print(f"$ {' '.join(cmd_prefix)} {name}", file=sys.stderr)
    proc = subprocess.run([*cmd_prefix, name], capture_output=True, text=True, check=False)
    text = proc.stdout + proc.stderr
    verdict_match = _SMH_VERDICT_RE.search(text)
    verdict = verdict_match.group(1).upper() if verdict_match else ("PASS" if proc.returncode == 0 else "FAIL")
    passed = int(verdict_match.group(2)) if verdict_match and verdict_match.group(2) else None
    total = int(verdict_match.group(3)) if verdict_match and verdict_match.group(3) else None
    # Failing test/family names (minus the overall-verdict line), so the JSON
    # says WHICH tests failed; the full log has the complete "why".
    failures = []
    for line in text.splitlines():
        if _SMH_VERDICT_RE.search(line):
            continue
        match = _SMH_FAIL_RE.match(line)
        if match:
            failures.append(match.group("name").strip())
    entry = {
        "verdict": verdict,
        "score": (f"{passed} / {total}" if passed is not None and total is not None else None),
        "passed": passed,
        "total": total,
        "failures": failures,
        "returncode": proc.returncode,
    }
    if raw_dir:
        os.makedirs(raw_dir, exist_ok=True)
        log_path = os.path.join(raw_dir, f"{stamp}_smhasher_{name}.log")
        with open(log_path, "w") as handle:
            handle.write(text)
        entry["log"] = log_path
        print(f"wrote {log_path}", file=sys.stderr)
    return name, entry


def run_smhasher(smhasher3, names, raw_dir, stamp, jobs=1):
    """Runs SMHasher3 for each registration name; returns a results dict.

    Args:
        smhasher3: SMHasher3 invocation as a command *prefix* (shell-split), so
            it works both as a native binary path and as a wrapper such as
            `docker run --rm -v <tree>:/src -w /src <image> ./build/SMHasher3`
            (the container-built binary does not run natively on macOS). For the
            in-house algorithms the binary must register mumbo-64/jumbo-128/
            dumbo-64 - see README.md.
        names: list of SMHasher3 registration names to run.
        raw_dir: directory to save each run's full log (may be None).
        stamp: timestamp prefix for the saved logs.
        jobs: number of batteries to run concurrently. They are independent, and
            their pass/fail verdicts do not depend on CPU load (only SMHasher3's
            own Speed sub-test would, and that number is not used - performance
            comes from our own benchmark), so parallelism only trades cores for
            wall-clock.

    Returns:
        {name: {"verdict": "PASS"/"FAIL"/"UNKNOWN", "failures": [...], "log": path}}.
    """
    cmd_prefix = shlex.split(smhasher3)
    done = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for future in concurrent.futures.as_completed(
            pool.submit(_smhasher_one, cmd_prefix, name, raw_dir, stamp) for name in names
        ):
            name, entry = future.result()
            done[name] = entry
    return {name: done[name] for name in names}  # restore the requested order


def _resolve_smhasher_names(algos):
    """Expand an --algos selection ("all" or a list) to SMHasher3 names."""
    selected = _SMHASHER_ALL if algos == ["all"] else algos
    names = []
    for algo in selected:
        if algo not in _SMHASHER_NAMES:
            raise SystemExit(f"unknown algorithm {algo!r}; known: {', '.join(_SMHASHER_NAMES)}")
        names.extend(_SMHASHER_NAMES[algo])
    return names


def _warn_context(results):
    ctx = results.get("context", {})
    if ctx.get("cpu_scaling_enabled"):
        print("WARNING: cpu_scaling_enabled - absolute numbers are unreliable (min mitigates).", file=sys.stderr)
    src = ctx.get("source", {})
    if not src.get("authoritative"):
        print(
            f"WARNING: not a clean main checkout (branch={src.get('git_branch')}, dirty={src.get('git_dirty')}); "
            "the git SHA will not survive a squash merge - re-measure on merged main for an authoritative dataset.",
            file=sys.stderr,
        )


# ---- rendering -------------------------------------------------------------


def _val(cell):
    """Headline value of a stored case: the best-k mean (min/float for older data)."""
    if isinstance(cell, dict):
        return cell.get("best", cell.get("min"))
    return float(cell)


def _fmt(value):
    if value < 100:
        return f"{value:.2f}"
    if value < 1000:
        return f"{value:.1f}"
    return f"{value:.0f}"


def _order(algos, preferred):
    return [a for a in preferred if a in algos] + [a for a in algos if a not in preferred]


def _size_label(length):
    return f"{length}B" if length < 1024 else f"{length // 1024}Ki"


def _md_table(headers, rows):
    """Render a vertically aligned GitHub markdown table (first column right-
    aligned, the rest left) so the output needs no reformatting."""
    cols = len(headers)
    width = [len(headers[c]) for c in range(cols)]
    for row in rows:
        for c in range(cols):
            width[c] = max(width[c], len(row[c]), 3)

    def line(cells):
        out = [cells[0].rjust(width[0])]
        out += [cells[c].ljust(width[c]) for c in range(1, cols)]
        return "| " + " | ".join(out) + " |"

    sep = ["-" * (width[0] - 1) + ":"] + ["-" * width[c] for c in range(1, cols)]
    return "\n".join([line(headers), "| " + " | ".join(sep) + " |"] + [line(r) for r in rows])


def _throughput_table(data, preferred, relabel, sizes):
    # Length-per-row, algorithm-per-column: with ~18 README sizes this reads far
    # better than 18 columns, and each row's bold marks the fastest algorithm at
    # that length.
    algos = _order(list(data), preferred)
    sizes = [s for s in sizes if any(str(s) in data[a] for a in algos)]
    headers = ["Length"] + [relabel.get(a, a) for a in algos]
    rows = []
    for size in sizes:
        row = {a: _val(data[a][str(size)]) for a in algos if str(size) in data[a]}
        best = min(row.values()) if row else None
        cells = [_size_label(size)]
        for algo in algos:
            if algo not in row:
                cells.append("-")
                continue
            text = _fmt(row[algo])
            if best is not None and abs(row[algo] - best) < 1e-9:
                text = f"**{text}**"
            cells.append(text)
        rows.append(cells)
    return _md_table(headers, rows)


def _latency_table(data):
    algos = _order(list(data), _ORDER_64)
    sizes = sorted({int(s) for a in data.values() for s in a})
    mins = {s: min(_val(data[a][str(s)]) for a in algos if str(s) in data[a]) for s in sizes}
    headers = ["max len"] + algos
    rows = []
    for size in sizes:
        cells = [str(size)]
        for algo in algos:
            cell = data[algo].get(str(size))
            text = "-" if cell is None else _fmt(_val(cell))
            if cell is not None and abs(_val(cell) - mins[size]) < 1e-9:
                text = f"**{text}**"
            cells.append(text)
        rows.append(cells)
    return _md_table(headers, rows)


def render_tables(results):
    ctx = results.get("context", {})
    host = ctx.get("host_name", "?")
    sha = (ctx.get("source") or {}).get("git_sha") or "?"
    meas = ctx.get("measurement", {})
    agg = f"mean of the {meas.get('best_k', '?')} fastest of {meas.get('reps', '?')} reps"
    out = [
        f"<!-- generated by mbo/hash/measurements/hash_benchmark_report.py; {agg}; {host} @ {sha[:12]} -->",
        "",
        f"### 64-bit one-shot throughput (ns/op, {agg}; lower is better)",
        "",
        _throughput_table(results["throughput64"], _ORDER_64, {}, _README_SIZES),
        "",
        f"### 128-bit one-shot throughput (ns/op, {agg}; native-128 algorithms only)",
        "",
        _throughput_table(results["throughput128"], _ORDER_128, _LABEL_128, _README_SIZES),
        "",
        f"### Mixed-length latency (ns/hash, {agg}; lower is better)",
        "",
        _latency_table(results["latency"]),
    ]
    return "\n".join(out)


def _svg_plot(results, path):
    """Dependency-free SVG: min ns vs length (log x), one line per 64-bit algo."""
    data = results["throughput64"]
    algos = _order(list(data), _ORDER_64)
    sizes = sorted({int(s) for a in data.values() for s in a})
    import math

    width, height, pad = 900, 520, 60
    xs = [math.log10(s) for s in sizes]
    x0, x1 = min(xs), max(xs)
    y1 = max(_val(data[a][str(s)]) for a in algos for s in sizes if str(s) in data[a])
    colors = ["#2563eb", "#dc2626", "#059669", "#d97706", "#7c3aed", "#0891b2", "#be185d", "#4b5563"]

    def px(size):
        return pad + (math.log10(size) - x0) / (x1 - x0) * (width - 2 * pad)

    def py(ns):
        return height - pad - (ns / y1) * (height - 2 * pad)

    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" font-family="sans-serif">']
    svg.append(f'<rect width="{width}" height="{height}" fill="white"/>')
    for size in sizes:  # x gridlines + labels
        x = px(size)
        svg.append(f'<line x1="{x:.1f}" y1="{pad}" x2="{x:.1f}" y2="{height-pad}" stroke="#eee"/>')
        svg.append(f'<text x="{x:.1f}" y="{height-pad+16}" font-size="10" text-anchor="middle">{size}</text>')
    svg.append(f'<text x="{width/2:.0f}" y="{height-12}" font-size="12" text-anchor="middle">key length (bytes, log)</text>')
    svg.append(f'<text x="16" y="{height/2:.0f}" font-size="12" transform="rotate(-90 16 {height/2:.0f})" text-anchor="middle">ns/op (min)</text>')
    for i, algo in enumerate(algos):
        color = colors[i % len(colors)]
        pts = " ".join(f"{px(s):.1f},{py(_val(data[algo][str(s)])):.1f}" for s in sizes if str(s) in data[algo])
        svg.append(f'<polyline points="{pts}" fill="none" stroke="{color}" stroke-width="2"/>')
        svg.append(f'<text x="{width-pad+4}" y="{18+i*16}" font-size="11" fill="{color}">{_LABEL_128.get(algo, algo)}</text>')
    svg.append("</svg>")
    with open(path, "w") as handle:
        handle.write("\n".join(svg))
    print(f"wrote {path}", file=sys.stderr)


def _load_json(path):
    opener = gzip.open if path.endswith(".gz") else open
    with opener(path, "rt") as handle:
        return json.load(handle)


def _dump_canonical(results, path):
    with open(path, "w") as handle:
        json.dump(results, handle, indent=2, sort_keys=True)
        handle.write("\n")


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p_run = sub.add_parser("run", help="run the benchmark, then store and/or render")
    p_run.add_argument("--mode", choices=["fast", "full"], default="full")
    # 9 reps is google/benchmark's recommended minimum for its compare.py U-test,
    # so two stored datasets can be compared for statistical significance.
    p_run.add_argument("--reps", type=int, default=9)
    p_run.add_argument("--min-time", type=float, default=0.1)
    p_run.add_argument("--warmup", type=float, default=0.05)
    p_run.add_argument("--raw", help="write google/benchmark raw JSON here (.gz compresses)")
    p_run.add_argument("--out", help="write the distilled canonical results JSON here")
    p_run.add_argument("--tables", action="store_true")

    p_store = sub.add_parser("store", help="distill raw benchmark JSON to canonical results JSON")
    p_store.add_argument("--raw", required=True)
    p_store.add_argument("--out", required=True)
    p_store.add_argument("--mode", choices=["fast", "full"], default="full")

    p_tables = sub.add_parser("tables", help="render README markdown tables from canonical JSON")
    p_tables.add_argument("--results", required=True)

    p_plot = sub.add_parser("plot", help="render an SVG ns-vs-length curve from canonical JSON")
    p_plot.add_argument("--results", required=True)
    p_plot.add_argument("--out", required=True)

    p_smh = sub.add_parser("smhasher", help="run the SMHasher3 quality battery over a set of algorithms")
    p_smh.add_argument(
        "--smhasher3",
        required=True,
        help="SMHasher3 invocation: a native binary path, or a wrapper command "
        "like 'docker run --rm -v <tree>:/src -w /src <image> ./build/SMHasher3'",
    )
    p_smh.add_argument(
        "--algos",
        default="all",
        help="comma-separated algorithms (default 'all', which includes the legacy dumbo)",
    )
    p_smh.add_argument("--out", help="write the results JSON here")
    p_smh.add_argument("--raw-dir", default="mbo/hash/measurements/data", help="directory for per-run SMHasher3 logs")
    p_smh.add_argument(
        "--jobs",
        type=int,
        default=1,
        help="batteries to run concurrently (independent; pass/fail is load-independent, so this only trades cores for wall-clock)",
    )

    args = parser.parse_args(argv)
    # One stamp per invocation, so all files a run writes share it. Every
    # written artifact is prefixed `YYYYMMDD_HHMMSS_` so nothing is overwritten
    # and the filename records when it was produced.
    stamp = _timestamp()

    if args.command == "run":
        raw = _run_benchmark(args.mode, args.reps, args.min_time, args.warmup)
        if args.raw:
            raw_path = _timestamped(args.raw, stamp)
            opener = gzip.open if raw_path.endswith(".gz") else open
            with opener(raw_path, "wt") as handle:
                json.dump(raw, handle)
            print(f"wrote {raw_path}", file=sys.stderr)
        results = distill(raw, args.mode)
        _warn_context(results)
        if args.out:
            out_path = _timestamped(args.out, stamp)
            _dump_canonical(results, out_path)
            print(f"wrote {out_path}", file=sys.stderr)
        if args.tables or not args.out:
            print(render_tables(results))
        return 0

    if args.command == "store":
        results = distill(_load_json(args.raw), args.mode)
        _warn_context(results)
        out_path = _timestamped(args.out, stamp)
        _dump_canonical(results, out_path)
        print(f"wrote {out_path}", file=sys.stderr)
        return 0

    if args.command == "tables":
        print(render_tables(_load_json(args.results)))
        return 0

    if args.command == "plot":
        out_path = _timestamped(args.out, stamp)
        _svg_plot(_load_json(args.results), out_path)
        return 0

    if args.command == "smhasher":
        algos = [a.strip() for a in args.algos.split(",") if a.strip()]
        names = _resolve_smhasher_names(algos)
        results = {
            "context": {**_provenance_context(), "smhasher": {"names": names, "smhasher3": args.smhasher3}},
            "smhasher": run_smhasher(args.smhasher3, names, args.raw_dir, stamp, args.jobs),
        }
        _warn_context(results)
        for name, entry in results["smhasher"].items():
            score = f" ({entry['score']})" if entry.get("score") else ""
            why = f" - failed: {', '.join(entry['failures'])}" if entry["failures"] else ""
            print(f"  {name}: {entry['verdict']}{score}{why}")
        failed = [n for n, r in results["smhasher"].items() if r["verdict"] != "PASS"]
        print(f"SMHasher3: {len(names) - len(failed)}/{len(names)} PASS" + (f"; FAIL: {', '.join(failed)}" if failed else ""))
        if args.out:
            out_path = _timestamped(args.out, stamp)
            _dump_canonical(results, out_path)
            print(f"wrote {out_path}", file=sys.stderr)
        return 1 if failed else 0

    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
