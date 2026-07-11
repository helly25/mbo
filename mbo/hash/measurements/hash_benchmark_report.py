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
import filecmp
import gzip
import json
import os
import re
import shlex
import statistics
import subprocess
import sys
import tarfile
import tempfile

# README tables read their sizes from the data itself (see _throughput_table),
# so there is no size list here to drift. The curated README set = kReadmeSizes
# in hash_benchmark.cc (the FAST mode); the tables are rendered from a fast-mode
# dataset, the ns-vs-length chart from the dense FULL dataset.

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
    """CPU brand / OS / model not always in google/benchmark's context; fill from the
    OS. Best-effort and cross-platform: any field that cannot be read is omitted.
    The build compiler is reported by the benchmark binary itself (`compiler` /
    `compiler_version` custom context) - system `cc` need not be what bazel built
    with - so it is not captured here."""
    brand = _sh(["sysctl", "-n", "machdep.cpu.brand_string"])  # macOS
    model = _sh(["sysctl", "-n", "hw.model"])  # macOS, e.g. "Mac17,9"
    if not brand:  # Linux
        brand = _sh(["sh", "-c", "grep -m1 'model name' /proc/cpuinfo | cut -d: -f2-"])
    if not model:  # Linux board/product name, best-effort
        model = _sh(["sh", "-c", "cat /sys/devices/virtual/dmi/id/product_name 2>/dev/null"])
    augment = {"cpu_brand": (brand or "").strip() or None, "uname": _sh(["uname", "-srm"])}
    if model and model.strip():
        augment["cpu_model"] = model.strip()
    return augment


def _run_benchmark(mode, reps, min_time, warmup, config=None):
    """Runs the bazel benchmark with the measurement precautions; returns parsed JSON.

    `config` selects a bazel `--config` (e.g. 'clang' / 'gcc'), so the toolchain -
    and therefore the compiler the benchmark records into the dataset - is the one
    you want to measure."""
    env = dict(os.environ)
    if mode == "full":
        env["MBO_HASH_BENCHMARK_FULL"] = "1"
    cmd = [
        "bazel",
        "run",
        "-c",
        "opt",
        *([f"--config={config}"] if config else []),
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


def _slug(text):
    """Lowercase; collapse each run of non-alphanumerics to one '-'; trim."""
    return re.sub(r"[^a-z0-9]+", "-", (text or "").lower()).strip("-")


def _platform_slug(ctx):
    """Filesystem-safe machine identity `<os>-<arch>-<cpu-brand>` from a dataset's
    context, e.g. `macos-arm64-apple-m5-pro`. Drives the per-machine data/ layout."""
    parts = (ctx.get("uname") or "").split()  # `uname -srm`: <sysname> <release> <machine>
    sysname = parts[0] if parts else ""
    machine = parts[-1] if len(parts) >= 3 else ""
    os_name = {"Darwin": "macos", "Linux": "linux"}.get(sysname, _slug(sysname) or "os")
    return f"{os_name}-{_slug(machine) or 'arch'}-{_slug(ctx.get('cpu_brand') or '') or 'cpu'}"


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
            in-house algorithms the binary must register mumbo-64/jumbo-128 and
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


def _throughput_table(data, preferred, relabel):
    # Length-per-row, algorithm-per-column; each row's bold marks the fastest
    # algorithm at that length. The sizes are read from the data's own buckets
    # (the single source of truth), so there is no second size list to drift from
    # the C++ kReadmeSizes - feed this the README (fast-mode) dataset.
    algos = _order(list(data), preferred)
    sizes = sorted({int(s) for a in data.values() for s in a})
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
        _throughput_table(results["throughput64"], _ORDER_64, {}),
        "",
        f"### 128-bit one-shot throughput (ns/op, {agg}; native-128 algorithms only)",
        "",
        _throughput_table(results["throughput128"], _ORDER_128, _LABEL_128),
        "",
        f"### Mixed-length latency (ns/hash, {agg}; lower is better)",
        "",
        _latency_table(results["latency"]),
    ]
    return "\n".join(out)


def _svg_plot(data, algos, title, path, label_map=None):
    """Dependency-free log-log SVG: best-k-mean ns vs length, one line per
    algorithm. Both axes are log-scaled - the ns range spans ~4 decades
    (sub-ns small keys to microseconds of the byte-at-a-time hashes on bulk), so
    a linear y-axis would crush every fast algorithm onto the baseline."""
    import math

    label_map = label_map or {}
    sizes = sorted({int(s) for a in data.values() for s in a})
    if not sizes or not algos:
        return
    width, height, pad_l, pad_r, pad_t, pad_b = 900, 520, 66, 132, 46, 52
    plot_w, plot_h = width - pad_l - pad_r, height - pad_t - pad_b
    x0, x1 = math.log10(sizes[0]), math.log10(sizes[-1])
    vals = [_val(data[a][str(s)]) for a in algos for s in sizes if str(s) in data[a] and _val(data[a][str(s)]) > 0]
    ly0, ly1 = math.log10(min(vals)), math.log10(max(vals))
    span_x, span_y = (x1 - x0) or 1.0, (ly1 - ly0) or 1.0
    colors = ["#2563eb", "#dc2626", "#059669", "#d97706", "#7c3aed", "#0891b2", "#be185d", "#4b5563"]

    def px(size):
        return pad_l + (math.log10(size) - x0) / span_x * plot_w

    def py(ns):
        return pad_t + (ly1 - math.log10(ns)) / span_y * plot_h

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" font-family="sans-serif">',
        f'<rect width="{width}" height="{height}" fill="white"/>',
        f'<text x="{width / 2:.0f}" y="26" font-size="15" font-weight="bold" text-anchor="middle">{title}</text>',
    ]
    exp = math.floor(ly0)  # y gridlines + labels at each power of ten in range
    while exp <= math.ceil(ly1):
        ns = 10.0**exp
        if ly0 - 1e-9 <= math.log10(ns) <= ly1 + 1e-9:
            y = py(ns)
            svg.append(f'<line x1="{pad_l}" y1="{y:.1f}" x2="{pad_l + plot_w:.1f}" y2="{y:.1f}" stroke="#eee"/>')
            svg.append(f'<text x="{pad_l - 6}" y="{y + 3:.1f}" font-size="10" text-anchor="end">{ns:g}</text>')
        exp += 1
    size = 1  # x gridlines at powers of two, labels at powers of four
    while size <= sizes[-1]:
        if x0 - 1e-9 <= math.log10(size) <= x1 + 1e-9:
            x = px(size)
            svg.append(f'<line x1="{x:.1f}" y1="{pad_t}" x2="{x:.1f}" y2="{pad_t + plot_h:.1f}" stroke="#f3f4f6"/>')
            if int(round(math.log2(size))) % 2 == 0:
                svg.append(f'<text x="{x:.1f}" y="{pad_t + plot_h + 16:.1f}" font-size="10" text-anchor="middle">{_size_label(size)}</text>')
        size *= 2
    svg.append(f'<text x="{pad_l + plot_w / 2:.0f}" y="{height - 12}" font-size="12" text-anchor="middle">key length (log scale)</text>')
    svg.append(
        f'<text x="18" y="{pad_t + plot_h / 2:.0f}" font-size="12" '
        f'transform="rotate(-90 18 {pad_t + plot_h / 2:.0f})" text-anchor="middle">ns / op (log scale)</text>'
    )
    for i, algo in enumerate(algos):  # one polyline + legend row per algorithm
        color = colors[i % len(colors)]
        pts = " ".join(
            f"{px(s):.1f},{py(_val(data[algo][str(s)])):.1f}"
            for s in sizes
            if str(s) in data[algo] and _val(data[algo][str(s)]) > 0
        )
        svg.append(f'<polyline points="{pts}" fill="none" stroke="{color}" stroke-width="2"/>')
        ly = pad_t + 8 + i * 17
        svg.append(f'<line x1="{pad_l + plot_w + 14:.1f}" y1="{ly:.1f}" x2="{pad_l + plot_w + 30:.1f}" y2="{ly:.1f}" stroke="{color}" stroke-width="2"/>')
        svg.append(f'<text x="{pad_l + plot_w + 34:.1f}" y="{ly + 4:.1f}" font-size="11" fill="{color}">{label_map.get(algo, algo)}</text>')
    svg.append("</svg>")
    with open(path, "w") as handle:
        handle.write("\n".join(svg) + "\n")  # trailing newline: matches the end-of-file-fixer'd committed form (verify)
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
    p_run.add_argument("--config", help="bazel --config for the benchmark build (e.g. clang, gcc); picks the toolchain and the recorded compiler")

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

    p_bundle = sub.add_parser("bundle", help="pack a run's artifacts into a per-machine .tgz under data/ (LFS-tracked)")
    p_bundle.add_argument("--results", required=True, help="canonical results JSON; its context names the machine")
    p_bundle.add_argument("--include", nargs="*", default=[], help="extra files to pack (raw.json.gz, smhasher.json, logs)")
    p_bundle.add_argument("--data-dir", default="mbo/hash/measurements/data")

    p_verify = sub.add_parser("verify", help="regenerate the SVG charts from a bundle and diff against the committed charts")
    p_verify.add_argument("--bundle", required=True, help="the .tgz to verify")
    p_verify.add_argument("--charts-dir", default="mbo/hash/measurements", help="dir holding the committed hash_throughput_*.svg")

    args = parser.parse_args(argv)
    # One stamp per invocation, so all files a run writes share it. Every
    # written artifact is prefixed `YYYYMMDD_HHMMSS_` so nothing is overwritten
    # and the filename records when it was produced.
    stamp = _timestamp()

    if args.command == "run":
        raw = _run_benchmark(args.mode, args.reps, args.min_time, args.warmup, args.config)
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
        results = _load_json(args.results)
        base, ext = os.path.splitext(_timestamped(args.out, stamp))
        _svg_plot(
            results["throughput64"],
            _order(list(results["throughput64"]), _ORDER_64),
            "mbo/hash - 64-bit one-shot throughput",
            f"{base}_64{ext}",
        )
        if results.get("throughput128"):
            _svg_plot(
                results["throughput128"],
                _order(list(results["throughput128"]), _ORDER_128),
                "mbo/hash - 128-bit one-shot throughput",
                f"{base}_128{ext}",
                _LABEL_128,
            )
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

    if args.command == "bundle":
        results = _load_json(args.results)
        ctx = results.get("context", {})
        slug = _platform_slug(ctx)
        cores = ctx.get("num_cpus", "?")
        compiler = _slug(ctx.get("compiler") or "cc")
        sha = ((ctx.get("source") or {}).get("git_sha") or "nogit")[:8]
        dest_dir = os.path.join(args.data_dir, slug)
        os.makedirs(dest_dir, exist_ok=True)
        dest = os.path.join(dest_dir, f"{slug}_{cores}c_{compiler}_{sha}_{stamp}.tgz")
        with tarfile.open(dest, "w:gz") as tar:
            tar.add(args.results, arcname="results.json")  # stable name so `verify` finds it
            for path in args.include:
                if path and os.path.exists(path):
                    tar.add(path, arcname=os.path.basename(path))
        print(dest)  # stdout: bundle path (for scripting)
        print(f"wrote {dest}", file=sys.stderr)
        return 0

    if args.command == "verify":
        with tempfile.TemporaryDirectory() as tmp:
            with tarfile.open(args.bundle, "r:gz") as tar:
                try:
                    tar.extractall(tmp, filter="data")  # py>=3.12 hardened extraction
                except TypeError:
                    tar.extractall(tmp)  # noqa: S202 - our own bundle, older Python
            results = _load_json(os.path.join(tmp, "results.json"))
            charts = [("throughput64", _ORDER_64, "mbo/hash - 64-bit one-shot throughput", "hash_throughput_64.svg", None)]
            if results.get("throughput128"):
                charts.append(
                    ("throughput128", _ORDER_128, "mbo/hash - 128-bit one-shot throughput", "hash_throughput_128.svg", _LABEL_128)
                )
            mismatches = []
            for key, order, title, svg_name, labels in charts:
                regen = os.path.join(tmp, svg_name)
                _svg_plot(results[key], _order(list(results[key]), order), title, regen, labels)
                committed = os.path.join(args.charts_dir, svg_name)
                if not os.path.exists(committed) or not filecmp.cmp(regen, committed, shallow=False):
                    mismatches.append(svg_name)
            if mismatches:
                print(f"VERIFY FAILED: committed charts do not match the bundle data: {', '.join(mismatches)}", file=sys.stderr)
                return 1
            print(f"VERIFY OK: committed charts match {os.path.basename(args.bundle)}", file=sys.stderr)
            return 0

    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
