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
    hash_benchmark_report.py tables results.json               # README (fast) subset
    hash_benchmark_report.py plot   results.json --out curve.svg               # throughput 64+128
    hash_benchmark_report.py plot   results.json --out curve.svg --kind latency --scale linear-log
tables/plot/compare/quality take a canonical JSON or a data bundle .tgz, either
positionally or via --results/--bundle. `plot --kind` picks the curves and
`--scale` the axes (log-log default, or linear-log = linear y / log x).
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

# Exact-length LATENCY: BmHash64/BmHash128<algo>/<len> - cost of hashing one exact
# length in a hot loop, reported as ns.
_LATENCY_RE = re.compile(r"^BmHash(64|128)<([A-Za-z0-9]+)>/(\d+)$")
# Bounded-range THROUGHPUT: BmHash{64,128}Throughput<algo>/<Dist>:<bound> - bytes/s
# over a length distribution truncated to each upper bound (Dist in {Short, Web}).
_THROUGHPUT_RE = re.compile(r"^BmHash(64|128)Throughput<([A-Za-z0-9]+)>/([A-Za-z][A-Za-z0-9-]*):(\d+)$")
_BENCHMARK_TARGET = "//mbo/hash:hash_benchmark"

# mbo algorithm -> SMHasher3 registration name(s). The in-house mumbo/jumbo/
# dumbo require a patched SMHasher3 that registers them (see the SMHasher3
# harness in README.md); the third-party names are SMHasher3's built-ins. Names
# are confirmed against `SMHasher3 --list`; override per run with --names.
_SMHASHER_NAMES = {
    "mumbo": ["mumbo-64"],
    "jumbo": ["jumbo-128"],
    "dumbo": ["dumbo-64"],
    "fnv1a": ["FNV-1a-64"],
    "xxh64": ["XXH-64"],
    "xxh3": ["XXH3-64", "XXH3-128"],
    "rapidhash": ["rapidhash"],
    "siphash": ["SipHash-2-4"],
    "murmur3": ["MurmurHash3-128"],
}
# Default set - ALL algorithms, explicitly including the legacy `dumbo`.
_SMHASHER_ALL = ["mumbo", "jumbo", "dumbo", "fnv1a", "xxh64", "xxh3", "rapidhash", "siphash", "murmur3"]

# Legacy / short SMHasher3 names -> the current registered name, applied when a
# bundle's logs are re-parsed so an older dataset (recorded before a name was
# pinned) still joins onto the algorithm instances by `smhasher` name. The
# false-PASS bug came from the bare short name `FNV-1a`, which the binary does not
# register (the real name is `FNV-1a-64`; likewise `MurmurHash3` ->
# `MurmurHash3-128`). Confirm names with `SMHasher3 --list`.
_SMH_NAME_ALIASES = {
    "FNV-1a": "FNV-1a-64",
    "MurmurHash3": "MurmurHash3-128",
}

# The manual/editorial SOT for the README quality tables lives in this JSON (per
# instance: role, notice, seeded, streaming, starlark, ... - NOT measured data).
# The measured columns (verdict/score/failures) come from a data bundle.
_ALGORITHMS_JSON = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hash_algorithms.json")

# TEMPORARY stopgap: the committed bundles were recorded under the invalid short
# names `FNV-1a`/`MurmurHash3` (rejected by the pinned SMHasher3 build), so their
# logs are the "Invalid hash" stub and carry no real data for these two. Hardcode
# the known values until a bundle carries a real `FNV-1a-64`/`MurmurHash3-128`
# measurement, then DELETE the matching entry. `failures` is the editorial summary
# (both fail most of the battery, so listing every family would be noise).
_MISSING_MEASURED = {
    "FNV-1a-64": {
        "verdict": "FAIL",
        "passed": 7,
        "total": 186,
        "failures": [
            "nearly every family: Avalanche, BIC, Sparse, Cyclic, Permutation, "
            "Text, TwoBytes, Bitflip, PerlinNoise, and the complete Seed* cluster"
        ],
    },
    "MurmurHash3-128": {
        "verdict": "FAIL",
        "passed": 123,
        "total": 188,
        "failures": ["BIC, Zeroes, Permutation, and the complete Seed* cluster (11 families)"],
    },
}


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


def _expand_list_arg(prefix, args):
    return [f"{prefix}={arg}" for arg in args] if args else []


def _run_benchmark(mode, reps, min_time, warmup, config=None, copt=None, host_copt=None):
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
        "--color=yes",
        "-c",
        "opt",
        *_expand_list_arg("--config", config),
        *_expand_list_arg("--copt", copt),
        *_expand_list_arg("--host_copt", host_copt),
        _BENCHMARK_TARGET,
        "--",
        "--benchmark_out_format=json",
        "--benchmark_out=/tmp/results.json",
        "--benchmark_format=console",
        f"--benchmark_repetitions={reps}",
        f"--benchmark_min_time={min_time}s",
        f"--benchmark_min_warmup_time={warmup}s",
        "--benchmark_enable_random_interleaving=true",
        "--benchmark_display_aggregates_only=true",
    ]
    print(f"$ MBO_HASH_BENCHMARK_FULL={env.get('MBO_HASH_BENCHMARK_FULL', '')} {' '.join(cmd)}", file=sys.stderr)
    subprocess.run(cmd, text=True, check=True, env=env)
    with open("/tmp/results.json", "rb") as f:
            out = f.read()
    return json.loads(out)


def _distill_buckets(raw):
    """Parse raw google/benchmark JSON into the canonical buckets; return
    (buckets, reps_seen, best_k_seen). Pure - no machine/source context - so a
    live distill and an offline re-distill (keeping a bundle's original context)
    share it. LATENCY (exact length) keeps ns (real_time); THROUGHPUT (bounded
    range) keeps bytes/s (higher is better), keyed by the "<Dist>:<bound>" label.
    """
    lat = {"latency64": {}, "latency128": {}}
    lat_section = {"64": "latency64", "128": "latency128"}
    tput = {"throughput64": {}, "throughput128": {}}
    tput_section = {"64": "throughput64", "128": "throughput128"}
    for bench in raw.get("benchmarks", []):
        if bench.get("run_type") != "iteration":
            continue
        match = _LATENCY_RE.match(bench["name"])
        if match:
            width, algo, length = match.groups()
            lat[lat_section[width]].setdefault(algo, {}).setdefault(length, []).append(float(bench["real_time"]))
            continue
        match = _THROUGHPUT_RE.match(bench["name"])
        if match:
            width, algo, dist, bound = match.groups()
            tput[tput_section[width]].setdefault(algo, {}).setdefault(f"{dist}:{bound}", []).append(
                float(bench["bytes_per_second"])
            )

    buckets = {}
    reps_seen = 0
    best_k_seen = 0

    def best_k_stats(values, lower_is_better):
        # mean of the k best reps (k ~= reps/3): for latency the low tail is the
        # uncontended cost, for throughput the high tail is. `best` is the
        # headline; `best_cv` the spread over the selected k.
        nonlocal reps_seen, best_k_seen
        reps_seen = max(reps_seen, len(values))
        best_k = max(1, len(values) // 3)
        best_k_seen = max(best_k_seen, best_k)
        ordered = sorted(values, reverse=not lower_is_better)  # best first
        best_vals = ordered[:best_k]
        best = statistics.fmean(best_vals)
        best_sd = statistics.stdev(best_vals) if len(best_vals) > 1 else 0.0
        return {
            "best": round(best, 4),
            "best_cv": round(best_sd / best, 4) if best else 0.0,
            "min": round(min(values), 4),
            "max": round(max(values), 4),
            "median": round(statistics.median(values), 4),
            "mean": round(statistics.fmean(values), 4),
            "reps": len(values),
        }

    for bucket, algos in lat.items():
        out = buckets.setdefault(bucket, {})
        for algo, lengths in algos.items():
            for length, times in lengths.items():
                out.setdefault(algo, {})[length] = best_k_stats(times, lower_is_better=True)
    for bucket, algos in tput.items():
        out = buckets.setdefault(bucket, {})
        for algo, cases in algos.items():
            for case, bps in cases.items():
                out.setdefault(algo, {})[case] = best_k_stats(bps, lower_is_better=False)
    return buckets, reps_seen, best_k_seen


def distill(raw, mode):
    """Raw google/benchmark JSON -> canonical per-case stats + full context."""
    ctx = dict(raw.get("context", {}))
    ctx.update(_machine_augment())
    ctx["source"] = _source_provenance()
    buckets, reps_seen, best_k_seen = _distill_buckets(raw)
    ctx["measurement"] = {"mode": mode, "aggregate": "best-k mean", "reps": reps_seen, "best_k": best_k_seen}
    return {"context": ctx, **buckets}


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


def _machine_label(ctx):
    """Readable machine identifier for headings + chart subtitles, e.g.
    'Apple M5 Pro · macOS · arm64 · 18-core · clang-22 · a1b2c3d4'."""
    parts = (ctx.get("uname") or "").split()  # `uname -srm`: <sysname> <release> <machine>
    sysname = parts[0] if parts else ""
    os_name = {"Darwin": "MacOS", "Linux": "Linux"}.get(sysname, sysname or "?")
    bits = [str(ctx.get("cpu_brand") or "?"), os_name, parts[-1] if len(parts) >= 3 else "?"]
    if ctx.get("num_cpus"):
        bits.append(f"{ctx['num_cpus']}-core")
    if ctx.get("compiler"):
        bits.append(str(ctx["compiler"]))
    sha = ((ctx.get("source") or {}).get("git_sha") or "")[:8]
    if sha:
        bits.append(sha)
    return " · ".join(bits)


# SMHasher3's per-battery Summary. Tolerant: the exact spacing varies, so we
# capture the raw log and parse best-effort. The real line (pinned build) is
# "Overall result: FAIL            ( 166 / 188 passed)" - note the trailing
# word "passed" INSIDE the parens, so the score group ends at [^)]* not \s*\).
_SMH_VERDICT_RE = re.compile(
    r"Overall result[:\s.]*\b(PASS|FAIL)\b(?:[^(]*\(\s*(\d+)\s*/\s*(\d+)[^)]*\))?", re.IGNORECASE
)
# One line of the Summary's "Failures:" block, the CLEAN machine-readable failure
# list, e.g. "    BIC                 : [3, 8, 11]" (the scattered per-test
# "!!!!!" lines above it are noise, not the family list).
_SMH_FAILBLOCK_RE = re.compile(r"^\s+(?P<family>[A-Za-z][\w-]*)\s*:\s*\[(?P<idx>[^\]]*)\]\s*$")


def _parse_smhasher(text, returncode=0):
    """Parse one SMHasher3 battery's captured output into a result entry (pure, no
    I/O). Used both for a live run and for re-parsing a bundle's saved log with the
    CURRENT parser - the committed bundles' stored `smhasher.json` predates parser
    fixes (score-less), so the LOGS, re-parsed here, are the measured truth."""
    verdict_match = _SMH_VERDICT_RE.search(text)
    invalid = re.search(r"Invalid hash '([^']*)' specified", text)
    # A completed battery ALWAYS ends with "Overall result: PASS|FAIL (p / n)".
    # Its absence means the battery never ran to completion: an unknown hash name
    # ("Invalid hash '...' specified", which still exits 0), a crash, or a
    # truncated log. That is a broken MEASUREMENT, not a hash that failed its
    # tests, so it reads as ERROR - never a returncode-0 default PASS that hides
    # the problem, and never a plain FAIL that would masquerade as a real result
    # with a missing score. FNV-1a once slipped through as PASS exactly this way
    # (invoked as `FNV-1a`, which the binary does not register; the name is now
    # `FNV-1a-64`), so the distinction is load-bearing.
    if verdict_match:
        verdict = verdict_match.group(1).upper()
        passed = int(verdict_match.group(2)) if verdict_match.group(2) else None
        total = int(verdict_match.group(3)) if verdict_match.group(3) else None
        error = None
    else:
        verdict, passed, total = "ERROR", None, None
        if invalid:
            error = f"invalid SMHasher3 name {invalid.group(1)!r} - check `SMHasher3 --list`"
        elif returncode != 0:
            error = f"SMHasher3 exited {returncode} with no 'Overall result' line"
        else:
            error = "no 'Overall result' line - crashed or truncated output"
    # Failing families as "Family [indices]" (e.g. "BIC [3, 8, 11]"), read from
    # the Summary's indented "Failures:" block so the JSON says WHICH families
    # failed; the full log has the complete "why". Only a FAIL has this block; an
    # ERROR (bad name/crash) never reached the Summary, and its partial log holds
    # only noise (per-subtest lines), so it gets no families.
    failures = []
    if verdict == "FAIL":
        in_block = False
        for line in text.splitlines():
            if re.match(r"\s*Failures:\s*$", line):
                in_block = True
                continue
            if not in_block:
                continue
            block_match = _SMH_FAILBLOCK_RE.match(line)
            if block_match:
                failures.append(f"{block_match.group('family')} [{block_match.group('idx').strip()}]")
            elif line.strip():
                break  # a non-blank, non-matching line ends the block (e.g. the "---" rule)
    return {
        "verdict": verdict,
        "score": (f"{passed} / {total}" if passed is not None and total is not None else None),
        "passed": passed,
        "total": total,
        "failures": failures,
        "returncode": returncode,
        "error": error,
    }


def _smhasher_one(cmd_prefix, name, raw_dir, stamp):
    """Run one SMHasher3 battery, parse it, and save the raw log."""
    print(f"$ {' '.join(cmd_prefix)} {name}", file=sys.stderr)
    proc = subprocess.run([*cmd_prefix, name], capture_output=True, text=True, check=False)
    text = proc.stdout + proc.stderr
    entry = _parse_smhasher(text, proc.returncode)
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
        {name: {"verdict": "PASS"/"FAIL"/"ERROR", "failures": [...], "log": path}}.
        ERROR means the battery did not run to completion (bad name/crash), which
        is a broken measurement, distinct from a hash that FAILed its tests.
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
    # Byte-unit labels used everywhere a length is shown (throughput + latency
    # tables, chart x-axis): "512B", "1KiB", "4KiB" - consistent "B"/"KiB" suffix.
    return f"{length} B" if length < 1024 else f"{length // 1024} KiB"


def _md_table(headers, rows, aligns=None):
    """Render a vertically aligned GitHub markdown table so the output needs no
    reformatting. `aligns` is a per-column 'l'/'r' list; its default right-aligns
    column 0 and left-aligns the rest (the perf-table convention)."""
    cols = len(headers)
    if aligns is None:
        aligns = ["r"] + ["l"] * (cols - 1)
    width = [len(headers[c]) for c in range(cols)]
    for row in rows:
        for c in range(cols):
            width[c] = max(width[c], len(row[c]), 3)

    def cell(text, c):
        return text.rjust(width[c]) if aligns[c] == "r" else text.ljust(width[c])

    def line(cells):
        return "| " + " | ".join(cell(cells[c], c) for c in range(cols)) + " |"

    def sep(c):
        return "-" * (width[c] - 1) + ":" if aligns[c] == "r" else "-" * width[c]

    return "\n".join([line(headers), "| " + " | ".join(sep(c) for c in range(cols)) + " |"] + [line(r) for r in rows])


def _length_table(data, preferred, relabel, sizes=None):
    # LATENCY table: length-per-row, algorithm-per-column, ns; each row's bold
    # marks the fastest (lowest) algorithm at that length. `sizes` is the curated
    # README subset from the dataset's own `readme_sizes` context (the C++
    # kReadmeSizes), so the small table is extracted from the FULL run with no
    # second size list to drift. Without it, every measured length is shown.
    algos = _order(list(data), preferred)
    if sizes is None:
        sizes = sorted({int(s) for a in data.values() for s in a})
    sizes = [s for s in sizes if any(str(s) in data[a] for a in algos)]
    headers = ["Length"] + [relabel.get(a, a) for a in algos]
    aligns = ["r"] + ["r"] * len(algos)
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
    return _md_table(headers, rows, aligns)


def _fmt_gibs(bytes_per_second):
    """bytes/s -> GiB/s string (2/1/0 decimals by magnitude)."""
    gibs = bytes_per_second / (1024.0**3)
    if gibs < 10:
        return f"{gibs:.2f}"
    if gibs < 100:
        return f"{gibs:.1f}"
    return f"{gibs:.0f}"


def _dists(data):
    """Distribution names present in a throughput bucket, in Short-then-Web order."""
    seen = []
    for algo in data.values():
        for key in algo:
            name = key.split(":", 1)[0]
            if name not in seen:
                seen.append(name)
    return _order(seen, ["Short", "Web"])


def _throughput_table(data, dist, preferred, relabel):
    # THROUGHPUT table for one distribution: upper-bound-per-row, algorithm-per-
    # column, GiB/s; bold marks the fastest (highest) at each bound. Keys are
    # "<Dist>:<bound>"; rows are that distribution's bounds ascending.
    algos = _order(list(data), preferred)
    prefix = f"{dist}:"
    bounds = sorted({int(k[len(prefix):]) for a in algos for k in data.get(a, {}) if k.startswith(prefix)})
    headers = ["max len"] + [relabel.get(a, a) for a in algos]
    aligns = ["r"] + ["r"] * len(algos)
    rows = []
    for bound in bounds:
        key = f"{dist}:{bound}"
        row = {a: _val(data[a][key]) for a in algos if key in data.get(a, {})}
        best = max(row.values()) if row else None
        cells = [_size_label(bound)]
        for algo in algos:
            if algo not in row:
                cells.append("-")
                continue
            text = _fmt_gibs(row[algo])
            if best is not None and abs(row[algo] - best) < 1e-9:
                text = f"**{text}**"
            cells.append(text)
        rows.append(cells)
    return _md_table(headers, rows, aligns)


def _agg_label(ctx):
    meas = ctx.get("measurement", {})
    return f"mean of the {meas.get('best_k', '?')} best of {meas.get('reps', '?')} reps"


def _readme_sizes(ctx):
    raw = ctx.get("readme_sizes")  # curated subset emitted by the benchmark (kReadmeSizes)
    return [int(x) for x in str(raw).split(",") if x.strip().isdigit()] if raw else None


def _dist_chart_data(data, dist):
    """One distribution's throughput as chart data: {algo: {bound: {"best": GiB/s}}}."""
    prefix = f"{dist}:"
    out = {}
    for algo, cases in data.items():
        for key, cell in cases.items():
            if key.startswith(prefix):
                out.setdefault(algo, {})[key[len(prefix):]] = {"best": _val(cell) / (1024.0**3)}
    return out


def _sections(results):
    """Per-machine sections in layout order - 64-bit latency, 64-bit throughput
    (Short, Web), 128-bit latency, 128-bit throughput (Short, Web) - each present
    only when the dataset has that data. Each section is a dict with `tag`,
    `title`, `heading`, `table`, `chart` (data or None), `order`, `relabel`,
    `kind`; charts and tables are generated from the same list so they stay in
    lockstep."""
    ctx = results.get("context", {})
    agg = _agg_label(ctx)
    sizes = _readme_sizes(ctx)
    out = []

    def latency(tag, width, order, relabel, extra=""):
        return {
            "tag": tag, "title": f"{width}-bit latency", "kind": "latency",
            "heading": f"{width}-bit latency (ns/hash at exact length, {agg}; {extra}lower is better)",
            "table": _length_table(results[tag], order, relabel, sizes),
            "chart": results[tag], "order": order, "relabel": relabel,
        }

    def throughput(bucket, dist, width, order, relabel):
        return {
            "tag": f"{bucket}_{dist}", "title": f"{width}-bit throughput ({dist})", "kind": "throughput",
            "heading": f"{width}-bit throughput, {dist} lengths (GiB/s over lengths <= max, {agg}; higher is better)",
            "table": _throughput_table(results[bucket], dist, order, relabel),
            "chart": _dist_chart_data(results[bucket], dist), "order": order, "relabel": relabel,
        }

    if results.get("latency64"):
        out.append(latency("latency64", 64, _ORDER_64, {}))
    for dist in _dists(results.get("throughput64", {})):
        out.append(throughput("throughput64", dist, 64, _ORDER_64, {}))
    if results.get("latency128"):
        out.append(latency("latency128", 128, _ORDER_128, _LABEL_128, extra="native-128 only; "))
    for dist in _dists(results.get("throughput128", {})):
        out.append(throughput("throughput128", dist, 128, _ORDER_128, _LABEL_128))
    return out


def render_tables(results):
    ctx = results.get("context", {})
    parts = [f"<!-- {_machine_label(ctx)}; {_agg_label(ctx)} -->"]
    for section in _sections(results):
        parts += ["", f"#### {section['heading']}", "", section["table"]]
    return "\n".join(parts)


# ---- compare: A (baseline) vs B (new) --------------------------------------


def _delta_pct(base, new):
    """Percent change of the headline number, lower-is-better: (B - A) / A * 100,
    so a negative delta means B is faster than A."""
    return (new - base) / base * 100.0 if base else 0.0


def _fmt_delta(pct):
    return f"{pct:+.1f}%"


def _geomean(ratios):
    """Geometric mean of per-case B/A ratios - the right average for ratios; a
    +10%/-10% pair nets ~0, not the misleading 0 an arithmetic mean of the deltas
    would also give but for the wrong reason. Returns None for an empty list."""
    import math

    return math.exp(sum(math.log(r) for r in ratios) / len(ratios)) if ratios else None


def _compare_bucket(base_data, new_data, preferred, relabel):
    """One bucket's Δ% table: length-per-row, algorithm-per-column, each cell the
    per-case Δ% of B vs A. A trailing `geomean` row summarizes each algorithm over
    all shared lengths. Only algorithms and lengths present in BOTH datasets are
    compared (others render `-`). Returns None if there is no shared case."""
    algos = _order([a for a in base_data if a in new_data], preferred)
    sizes = sorted({int(s) for a in algos for s in base_data[a] if s in new_data.get(a, {})})
    if not algos or not sizes:
        return None
    headers = ["Length"] + [relabel.get(a, a) for a in algos]
    ratios = {a: [] for a in algos}
    rows = []
    for size in sizes:
        cells = [_size_label(size)]
        for algo in algos:
            base_cell, new_cell = base_data[algo].get(str(size)), new_data[algo].get(str(size))
            if base_cell is None or new_cell is None:
                cells.append("-")
                continue
            base_val, new_val = _val(base_cell), _val(new_cell)
            cells.append(_fmt_delta(_delta_pct(base_val, new_val)))
            if base_val > 0:
                ratios[algo].append(new_val / base_val)
        rows.append(cells)
    geo = ["geomean"]
    for algo in algos:
        mean = _geomean(ratios[algo])
        geo.append(_fmt_delta((mean - 1.0) * 100.0) if mean is not None else "-")
    rows.append(geo)
    return _md_table(headers, rows)


def render_compare(base, new):
    """Markdown Δ% report between two canonical datasets. Shows both machine
    labels (comparing across machines is legitimate - it is the reader's call),
    then a per-case Δ% table with a geomean row for each section."""
    out = [
        "Δ% = (B - A) / A per case. Latency: negative = B faster (lower ns). "
        "Throughput: positive = B faster (higher bytes/s).",
        f"- A: {_machine_label(base.get('context', {}))}",
        f"- B: {_machine_label(new.get('context', {}))}",
    ]
    for key, order, relabel, title in (
        ("latency64", _ORDER_64, {}, "64-bit latency"),
        ("latency128", _ORDER_128, _LABEL_128, "128-bit latency"),
    ):
        table = _compare_bucket(base.get(key) or {}, new.get(key) or {}, order, relabel)
        if table:
            out += ["", f"#### {title} (Δ%)", "", table]
    for key, order, relabel, width in (
        ("throughput64", _ORDER_64, {}, "64-bit"),
        ("throughput128", _ORDER_128, _LABEL_128, "128-bit"),
    ):
        for dist in _dists(base.get(key) or {}):
            table = _compare_bucket(
                _dist_chart_data(base.get(key) or {}, dist), _dist_chart_data(new.get(key) or {}, dist), order, relabel
            )
            if table:
                out += ["", f"#### {width} throughput, {dist} (Δ%)", "", table]
    return "\n".join(out)


def _linear_ticks(vmax, target=6):
    """~`target` 'nice' gridline values 0..>=vmax with a 1/2/2.5/5 * 10^k step,
    for the linear y-axis (`--scale linear-log`)."""
    import math

    if vmax <= 0:
        return [0.0]
    raw = vmax / target
    mag = 10.0 ** math.floor(math.log10(raw))
    step = next(m * mag for m in (1, 2, 2.5, 5, 10) if m * mag >= raw)
    ticks, tick = [], 0.0
    while tick <= vmax + step * 1e-9:
        ticks.append(tick)
        tick += step
    return ticks


def _svg_plot(data, algos, title, path, label_map=None, subtitle=None, linear_y=False, y_label="ns / op", x_label="key length"):
    """Dependency-free value-vs-length SVG, one line per algorithm (best-k mean).

    The x-axis (key length) is ALWAYS log-scaled: lengths are sampled
    geometrically (1..4096), so a linear x would bunch every small key at the
    origin. The y-axis (ns) is log by default - the range spans ~4 decades
    (sub-ns small keys to microseconds of the byte-at-a-time hashes on bulk), so
    a linear y crushes every fast algorithm onto the baseline. `linear_y` switches
    the y-axis to a 0-based linear scale (the `--scale linear-log` mode) to read
    absolute ns gaps within a narrow range. `subtitle` (the machine identifier) is
    drawn under the title so a chart is self-labeling."""
    import math

    label_map = label_map or {}
    sizes = sorted({int(s) for a in data.values() for s in a})
    if not sizes or not algos:
        return
    width, height, pad_l, pad_r, pad_t, pad_b = 900, 536, 66, 132, 62, 52
    plot_w, plot_h = width - pad_l - pad_r, height - pad_t - pad_b
    x0, x1 = math.log10(sizes[0]), math.log10(sizes[-1])
    vals = [_val(data[a][str(s)]) for a in algos for s in sizes if str(s) in data[a] and _val(data[a][str(s)]) > 0]
    span_x = (x1 - x0) or 1.0
    colors = ["#2563eb", "#dc2626", "#059669", "#d97706", "#7c3aed", "#0891b2", "#be185d", "#4b5563"]

    def px(size):
        return pad_l + (math.log10(size) - x0) / span_x * plot_w

    if linear_y:  # 0-based linear y (span = y_top), gridlines at the nice ticks
        y_ticks = _linear_ticks(max(vals))
        y_top = y_ticks[-1] or 1.0

        def py(ns):
            return pad_t + (y_top - ns) / y_top * plot_h
    else:  # log y (default): decade span, gridlines at each power of ten
        ly0, ly1 = math.log10(min(vals)), math.log10(max(vals))
        span_y = (ly1 - ly0) or 1.0

        def py(ns):
            return pad_t + (ly1 - math.log10(ns)) / span_y * plot_h

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" font-family="sans-serif">',
        f'<rect width="{width}" height="{height}" fill="white"/>',
        f'<text x="{width / 2:.0f}" y="24" font-size="15" font-weight="bold" text-anchor="middle">{title}</text>',
    ]
    if subtitle:
        svg.append(
            f'<text x="{width / 2:.0f}" y="44" font-size="12" fill="#6b7280" text-anchor="middle">{subtitle}</text>'
        )
    if linear_y:  # y gridlines + labels at each nice linear tick
        for tick in y_ticks:
            y = py(tick)
            svg.append(f'<line x1="{pad_l}" y1="{y:.1f}" x2="{pad_l + plot_w:.1f}" y2="{y:.1f}" stroke="#eee"/>')
            svg.append(f'<text x="{pad_l - 6}" y="{y + 3:.1f}" font-size="10" text-anchor="end">{tick:g}</text>')
    else:  # y gridlines + labels at 1-2-5 x 10^exp within range (readable on both
        # the ~4-decade latency axis and the narrow ~1-decade throughput axis)
        exp = math.floor(ly0)
        while exp <= math.ceil(ly1):
            for mantissa in (1, 2, 5):
                val = mantissa * 10.0**exp
                if ly0 - 1e-9 <= math.log10(val) <= ly1 + 1e-9:
                    y = py(val)
                    svg.append(f'<line x1="{pad_l}" y1="{y:.1f}" x2="{pad_l + plot_w:.1f}" y2="{y:.1f}" stroke="#eee"/>')
                    svg.append(f'<text x="{pad_l - 6}" y="{y + 3:.1f}" font-size="10" text-anchor="end">{val:g}</text>')
            exp += 1
    size = 1  # x gridlines at powers of two, labels at powers of four
    while size <= sizes[-1]:
        if x0 - 1e-9 <= math.log10(size) <= x1 + 1e-9:
            x = px(size)
            svg.append(f'<line x1="{x:.1f}" y1="{pad_t}" x2="{x:.1f}" y2="{pad_t + plot_h:.1f}" stroke="#f3f4f6"/>')
            if int(round(math.log2(size))) % 2 == 0:
                svg.append(f'<text x="{x:.1f}" y="{pad_t + plot_h + 16:.1f}" font-size="10" text-anchor="middle">{_size_label(size)}</text>')
        size *= 2
    svg.append(f'<text x="{pad_l + plot_w / 2:.0f}" y="{height - 12}" font-size="12" text-anchor="middle">{x_label} (log scale)</text>')
    svg.append(
        f'<text x="18" y="{pad_t + plot_h / 2:.0f}" font-size="12" '
        f'transform="rotate(-90 18 {pad_t + plot_h / 2:.0f})" text-anchor="middle">'
        f'{y_label} ({"linear" if linear_y else "log"} scale)</text>'
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


# --- publish: labeled per-machine perf section rendered FROM the data bundles ---

_PERF_BEGIN = "<!-- BEGIN mbo/hash benchmark results (generated by `hash_benchmark_report.py publish`; DO NOT EDIT) -->"
_PERF_END = "<!-- END mbo/hash benchmark results -->"

# --- quality: the README tables, from hash_algorithms.json (manual) x a bundle
# (measured). Two generated regions: the "Algorithm overview" and the SMHasher3
# "Results" table. Manual columns come from the JSON; verdict/score/failures from
# the measurement (see render_overview_table / render_results_table).

_OVERVIEW_BEGIN = "<!-- BEGIN algorithm overview (generated by `hash_benchmark_report.py quality`; DO NOT EDIT) -->"
_OVERVIEW_END = "<!-- END algorithm overview -->"
_SMH_BEGIN = "<!-- BEGIN SMHasher3 results (generated by `hash_benchmark_report.py quality`; DO NOT EDIT) -->"
_SMH_END = "<!-- END SMHasher3 results -->"


def _load_algorithms():
    """Load the manual/editorial per-instance data + the overview row order."""
    data = _load_json(_ALGORITHMS_JSON)
    return data["algorithms"], data["overview_order"]


def _measured_entry(name, measured):
    """The measured entry for `name`, aborting loudly if it is missing or did not
    complete (ERROR) - a broken/absent measurement must never render a plausible
    row. Callers cover the known gaps via `_fill_missing_measured` first."""
    entry = measured.get(name)
    if entry is None:
        raise SystemExit(f"{name}: no measured data - pass a bundle that measured it (or add a stopgap)")
    if entry["verdict"] not in ("PASS", "FAIL"):
        raise SystemExit(
            f"{name}: measured verdict {entry['verdict']!r} is not PASS/FAIL - the battery did not "
            f"complete ({entry.get('error') or 'bad name/crash'}); fix the measurement before regenerating"
        )
    return entry


def render_results_table(algorithms, measured):
    """Render the SMHasher3 "### Results" table: manual role/bits from the JSON,
    verdict/score/failures from the measurement (joined by smhasher name)."""
    headers = ["Algorithm", "Bits", "Role in mbo/hash", "SMHasher3 result", "Failures"]
    aligns = ["l", "r", "l", "l", "l"]
    rows = []
    for algo in algorithms:
        entry = _measured_entry(algo["smhasher"], measured)
        if entry["verdict"] == "PASS":
            result, failures = "**PASS**", "none"
        else:
            passed, total = entry.get("passed"), entry.get("total")
            if passed is None or total is None or passed >= total:
                raise SystemExit(f"{algo['smhasher']}: FAIL without a valid passed/total score")
            result = f"{passed}/{total}"
            failures = ", ".join(entry.get("failures") or []) or "none"
        rows.append([f"`{algo['algo']}`", str(algo["bits"]), algo["role"], result, failures])
    return _md_table(headers, rows, aligns)


def _bold_if(text, condition):
    return f"**{text}**" if condition else text


def _bold_if_yes(text):
    return f"**{text}**" if text.lower() == "yes" else text


def render_overview_table(algorithms, measured, overview_order):
    """Render the "## Algorithm overview" table: manual columns from the JSON, the
    `SMHasher3` PASS/FAIL derived from the measured verdict (so it cannot disagree
    with the Results table). Rows follow the overview's own curated order."""
    headers = ["Algorithm", "Bits", "SMHasher3", "Seeded", "Streaming", "Starlark", "NOTICE", "Available via"]
    aligns = ["l", "r", "l", "l", "l", "l", "l", "l"]
    by_name = {algo["smhasher"]: algo for algo in algorithms}
    rows = []
    for name in overview_order:
        algo = by_name[name]
        entry = _measured_entry(name, measured)
        verdict = _bold_if(entry["verdict"], entry["verdict"] == "PASS")
        starlark = _bold_if_yes(algo["starlark"])
        streaming = _bold_if_yes(algo["streaming"])
        rows.append([
            f"`{algo['algo']}`", str(algo["bits"]), verdict,
            algo["seeded"], streaming, starlark, algo["notice"], algo["available_via"],
        ])
    return _md_table(headers, rows, aligns)


def _extract_bundle(path, dest):
    """Extract a bundle .tgz into dest (hardened on py>=3.12; our own data)."""
    with tarfile.open(path, "r:gz") as tar:
        try:
            tar.extractall(dest, filter="data")
        except TypeError:
            tar.extractall(dest)  # noqa: S202 - our own bundle, older Python


def _results_from_bundle(path):
    """Canonical results from a data bundle .tgz. The raw google/benchmark JSON is
    the source of truth, so we RE-DISTILL it with the current tool - a bundle
    measured before the latency/throughput split still yields today's schema (e.g.
    latency tables from old data) with no re-packing. The bundle's own
    machine/source context is kept (from its stored results.json); we fall back to
    the stored results.json only if the bundle carries no raw."""
    with tempfile.TemporaryDirectory() as tmp:
        _extract_bundle(path, tmp)
        stored_path = os.path.join(tmp, "results.json")
        stored = _load_json(stored_path) if os.path.exists(stored_path) else {}
        raws = sorted(f for f in os.listdir(tmp) if "raw" in f and ".json" in f)
        if not raws:
            return stored
        buckets, reps, best_k = _distill_buckets(_load_json(os.path.join(tmp, raws[-1])))
        ctx = dict(stored.get("context") or {})
        ctx.setdefault("measurement", {})
        ctx["measurement"] = {**ctx["measurement"], "reps": reps, "best_k": best_k}
        return {"context": ctx, **buckets}


def _load_dataset(path):
    """Canonical results from either form: a data bundle (.tgz -> its packed
    results.json) or a results JSON directly (.json[.gz]). Lets every command
    take a bundle or a bare dataset interchangeably."""
    return _results_from_bundle(path) if path.endswith(".tgz") else _load_json(path)


def _one_path(candidates, what):
    """Exactly one non-empty path from `candidates`, else a clear CLI error. Lets
    a command accept its input positionally OR via a flag, interchangeably."""
    chosen = [path for path in candidates if path]
    if len(chosen) != 1:
        raise SystemExit(f"provide exactly one {what}: a positional path or its flag")
    return chosen[0]


def _resolve_dataset(args):
    """The single canonical dataset for a command, given positionally or via
    --results/--bundle (all interchangeable, auto-detected by extension)."""
    return _load_dataset(_one_path([args.dataset, args.results, args.bundle], "dataset"))


_SMH_LOG_RE = re.compile(r"^(\d{8}_\d{6})_smhasher_(.+)\.log(\.gz)?$")


def _measured_from_bundle(path):
    """Re-parse a bundle's per-algorithm SMHasher3 logs into a measured map
    {registered-name: entry} with the CURRENT parser. The bundle's stored
    smhasher.json is ignored (it predates parser fixes, so it is score-less); the
    LOGS are the measured truth. The newest log per name wins, and legacy / short
    log names are normalized to their registered form (see _SMH_NAME_ALIASES)."""
    latest = {}  # name -> (stamp, text)
    with tempfile.TemporaryDirectory() as tmp:
        _extract_bundle(path, tmp)
        for fname in sorted(os.listdir(tmp)):
            match = _SMH_LOG_RE.match(fname)
            if not match:
                continue
            stamp, name, gz = match.group(1), match.group(2), match.group(3)
            opener = gzip.open if gz else open
            with opener(os.path.join(tmp, fname), "rt") as handle:
                text = handle.read()
            if name not in latest or stamp >= latest[name][0]:
                latest[name] = (stamp, text)
    measured = {}
    for name, (_, text) in latest.items():
        measured[_SMH_NAME_ALIASES.get(name, name)] = _parse_smhasher(text)
    return measured


def _fill_missing_measured(measured):
    """Overlay the hardcoded stopgap (`_MISSING_MEASURED`) for any name the bundle
    lacks valid data for (absent, or an ERROR from a bad-name/crashed run). Warns
    on every substitution so the stopgap is never silent. Returns a new map."""
    out = dict(measured)
    for name, stub in _MISSING_MEASURED.items():
        current = out.get(name)
        if current is None or current.get("verdict") == "ERROR":
            print(
                f"WARNING: {name}: no valid measurement in the bundle; using the hardcoded stopgap "
                f"(delete _MISSING_MEASURED[{name!r}] once a bundle carries real data).",
                file=sys.stderr,
            )
            out[name] = {**stub, "score": f"{stub['passed']} / {stub['total']}", "returncode": 0, "error": None}
    return out


def _render_charts(full, stem, charts_dir, subtitle):
    """Write one labeled SVG per section that has chart data (latency = ns-vs-length,
    throughput = GiB/s-vs-upper-bound); return [(tag, filename), ...] in layout order."""
    written = []
    for section in _sections(full):
        data = section["chart"]
        if not data:
            continue
        name = f"{stem}_{section['tag']}.svg"
        y_label, x_label = ("GiB / s", "max length") if section["kind"] == "throughput" else ("ns / op", "key length")
        _svg_plot(
            data, _order(list(data), section["order"]), f"mbo/hash - {section['title']}",
            os.path.join(charts_dir, name), section["relabel"], subtitle=subtitle, y_label=y_label, x_label=x_label,
        )
        written.append((section["tag"], name))
    return written


def _bundle_stem(ctx):
    """Chart-file stem for a machine: '<slug>_<compiler>'."""
    return f"{_platform_slug(ctx)}_{_slug(ctx.get('compiler') or 'cc')}"


def _add_dataset_arg(parser):
    """Give a command one canonical-dataset input, accepted three interchangeable
    ways: a positional path, or --results / --bundle (all auto-detect bundle vs
    JSON by extension). Keeping the flags means existing invocations still work."""
    parser.add_argument("dataset", nargs="?", help="dataset path: a bundle .tgz or a results.json[.gz] (auto-detected)")
    parser.add_argument("--results", help="canonical results JSON (.gz ok); same as passing it positionally")
    parser.add_argument("--bundle", help="data bundle .tgz; same as passing it positionally")


def dispatch_help(_, parser):
    parser.print_help()
    return 0


def dispatch_run(args, stamp):
    raw = _run_benchmark(args.mode, args.reps, args.min_time, args.warmup, args.config, args.copt, args.host_copt)
    if args.raw:
        raw_path = _timestamped(args.raw, stamp)
        opener = gzip.open if raw_path.endswith(".gz") else open
        with opener(raw_path, "wt") as handle:
            json.dump(raw, handle)
        print(f"wrote {raw_path}", file=sys.stderr)
    results = distill(raw, args.mode)
    context = results.setdefault("context", {})
    context["config"] = args.config
    context["copt"] = args.copt
    context["host_copt"] = args.host_copt
    _warn_context(results)
    if args.out:
        out_path = _timestamped(args.out, stamp)
        _dump_canonical(results, out_path)
        print(f"wrote {out_path}", file=sys.stderr)
    if args.tables or not args.out:
        print(render_tables(results))
    return 0


def dispatch_store(args, stamp):
    results = distill(_load_json(args.raw), args.mode)
    _warn_context(results)
    out_path = _timestamped(args.out, stamp)
    _dump_canonical(results, out_path)
    print(f"wrote {out_path}", file=sys.stderr)
    return 0


def dispatch_tables(args, stamp):
    print(render_tables(_resolve_dataset(args)))
    return 0


def dispatch_compare(args, stamp):
    print(render_compare(_load_dataset(args.a), _load_dataset(args.b)))
    return 0


def dispatch_plot(args, stamp):
    results = _resolve_dataset(args)
    base, ext = os.path.splitext(_timestamped(args.out, stamp))
    linear_y = args.scale == "linear-log"
    # `--kind` picks latency (ns-vs-length) and/or throughput (GiB/s-vs-bound)
    # sections; one SVG per section, suffixed by its tag.
    for section in _sections(results):
        data = section["chart"]
        if not data or (args.kind != "all" and section["kind"] != args.kind):
            continue
        y_label, x_label = ("GiB / s", "max length") if section["kind"] == "throughput" else ("ns / op", "key length")
        _svg_plot(
            data, _order(list(data), section["order"]), f"mbo/hash - {section['title']}",
            f"{base}_{section['tag']}{ext}", section["relabel"], linear_y=linear_y, y_label=y_label, x_label=x_label,
        )
    return 0


def dispatch_smhasher(args, stamp):
    algos = [a.strip() for a in args.algos.split(",") if a.strip()]
    names = _resolve_smhasher_names(algos)
    results = {
        "context": {**_provenance_context(), "smhasher": {"names": names, "smhasher3": args.smhasher3}},
        "smhasher": run_smhasher(args.smhasher3, names, args.raw_dir, stamp, args.jobs),
    }
    _warn_context(results)
    for name, entry in results["smhasher"].items():
        score = f" ({entry['score']})" if entry.get("score") else ""
        if entry["verdict"] == "ERROR":
            why = f" - {entry.get('error') or 'measurement error'}"
        else:
            why = f" - failed: {', '.join(entry['failures'])}" if entry["failures"] else ""
        print(f"  {name}: {entry['verdict']}{score}{why}")
    errored = [n for n, r in results["smhasher"].items() if r["verdict"] == "ERROR"]
    failed = [n for n, r in results["smhasher"].items() if r["verdict"] == "FAIL"]
    passed = len(names) - len(failed) - len(errored)
    tail = "".join(
        f"; {label}: {', '.join(bad)}" for label, bad in (("ERROR", errored), ("FAIL", failed)) if bad
    )
    print(f"SMHasher3: {passed}/{len(names)} PASS" + tail)
    if args.out:
        out_path = _timestamped(args.out, stamp)
        _dump_canonical(results, out_path)
        print(f"wrote {out_path}", file=sys.stderr)
    return 1 if (failed or errored) else 0

def dispatch_bundle(args, stamp):
    results = _load_json(args.results)
    ctx = results.get("context", {})
    slug = _platform_slug(ctx)
    cores = ctx.get("num_cpus", "?")
    compiler = _slug(ctx.get("compiler") or "cc")
    sha = ((ctx.get("source") or {}).get("git_sha") or "nogit")[:8]
    # Flat: the filename already carries the full machine identity, so no subdir.
    os.makedirs(args.data_dir, exist_ok=True)
    dest = os.path.join(args.data_dir, f"{slug}_{cores}c_{compiler}_{sha}_{stamp}.tgz")
    with tarfile.open(dest, "w:gz") as tar:
        tar.add(args.results, arcname="results.json")  # canonical: chart + tables + verify
        for path in args.include:
            if path and os.path.exists(path):
                tar.add(path, arcname=os.path.basename(path))
    print(dest)  # stdout: bundle path (for scripting)
    print(f"wrote {dest}", file=sys.stderr)
    return 0


def dispatch_publish(args, stamp):
    os.makedirs(args.charts_dir, exist_ok=True)
    rel = os.path.relpath(args.charts_dir, os.path.dirname(os.path.abspath(args.readme)))
    blocks = []  # (label, bundle_path, section) - sorted by label below, not by input order
    for bundle_path in args.bundles:
        full = _results_from_bundle(bundle_path)  # re-distilled from the bundle's raw (see B)
        ctx = full.get("context", {})
        label = _machine_label(ctx)
        charts = dict(_render_charts(full, _bundle_stem(ctx), args.charts_dir, label))  # tag -> filename
        # `### {label}` heads the block; each section is its chart (if any)
        # immediately followed by its table, in layout order.
        parts = [f"### {label}", "", f"<!-- {label}; {_agg_label(ctx)} -->"]
        for section in _sections(full):
            if section["tag"] in charts:
                parts += ["", f"![mbo/hash {section['title']}, {label}]({rel}/{charts[section['tag']]})"]
            parts += ["", f"#### {section['heading']}", "", section["table"]]
        blocks.append((label, bundle_path, "\n".join(parts)))
        print(f"published {label}", file=sys.stderr)
    # Order sections (and the manifest) by the generated header, so the README
    # is stable regardless of the order bundles were passed on the command line.
    blocks.sort(key=lambda block: block[0])
    manifest = "<!-- bundles: " + " ".join(os.path.relpath(b) for _, b, _ in blocks) + " -->"
    region = "\n".join([_PERF_BEGIN, manifest, "", "\n\n".join(s for _, _, s in blocks), _PERF_END])
    text = open(args.readme).read()
    if _PERF_BEGIN not in text or _PERF_END not in text:
        raise SystemExit(f"markers not found in {args.readme}; add a {_PERF_BEGIN} ... {_PERF_END} region")
    text = text[: text.index(_PERF_BEGIN)] + region + text[text.index(_PERF_END) + len(_PERF_END) :]
    with open(args.readme, "w") as handle:
        handle.write(text)
    print(f"wrote perf section into {args.readme} ({len(args.bundles)} machine(s))", file=sys.stderr)
    return 0


def dispatch_verify(args, stamp):
    match = re.search(r"<!-- bundles: (.*?) -->", open(args.readme).read())
    if not match:
        raise SystemExit(f"no bundle manifest in {args.readme}; run `publish` first")
    bundles = match.group(1).split()
    mismatches = []
    for bundle_path in bundles:
        full = _results_from_bundle(bundle_path)  # re-distilled from the bundle's raw (see B)
        ctx = full.get("context", {})
        with tempfile.TemporaryDirectory() as tmp:
            for _, name in _render_charts(full, _bundle_stem(ctx), tmp, _machine_label(ctx)):
                committed = os.path.join(args.charts_dir, name)
                if not os.path.exists(committed) or not filecmp.cmp(os.path.join(tmp, name), committed, shallow=False):
                    mismatches.append(name)
    if mismatches:
        print(f"VERIFY FAILED: committed charts differ from the bundle data: {', '.join(sorted(set(mismatches)))}", file=sys.stderr)
        return 1
    print(f"VERIFY OK: committed charts match all {len(bundles)} bundle(s)", file=sys.stderr)
    return 0

def dispatch_quality(args, stamp):
    algorithms, overview_order = _load_algorithms()
    measured = _fill_missing_measured(_measured_from_bundle(_one_path([args.bundle_pos, args.bundle], "bundle")))
    text = open(args.readme).read()
    regions = [
        (_OVERVIEW_BEGIN, _OVERVIEW_END, render_overview_table(algorithms, measured, overview_order)),
        (_SMH_BEGIN, _SMH_END, render_results_table(algorithms, measured)),
    ]
    new = text
    for begin, end, body in regions:
        if begin not in new or end not in new:
            raise SystemExit(f"markers not found in {args.readme}; add a {begin} ... {end} region")
        block = "\n".join([begin, "", body, "", end])
        new = new[: new.index(begin)] + block + new[new.index(end) + len(end) :]
    if args.check:
        if new != text:
            print(f"VERIFY FAILED: the generated tables in {args.readme} are stale; run `quality`", file=sys.stderr)
            return 1
        print("VERIFY OK: the generated overview + Results tables match hash_algorithms.json + the bundle", file=sys.stderr)
        return 0
    if new != text:
        with open(args.readme, "w") as handle:
            handle.write(new)
        print(f"wrote the overview + Results tables into {args.readme}", file=sys.stderr)
    else:
        print(f"the generated tables are already current in {args.readme}", file=sys.stderr)
    return 0


def dispatch_consistency(args, stamp):
    bundles = args.bundles or sorted(
        os.path.join(args.data_dir, f) for f in os.listdir(args.data_dir) if f.endswith(".tgz")
    )
    if not bundles:
        raise SystemExit(f"no bundles found (looked in {args.data_dir})")
    # Group by the source git SHA embedded in the bundle filename
    # (<slug>_<cores>c_<compiler>_<sha8>_<stamp>.tgz). SMHasher3 verdicts are a
    # property of the algorithm, not the machine, so bundles at the same SHA
    # must report identical measurements - a difference means a broken run.
    by_sha = {}
    for bundle in bundles:
        match = re.search(r"_([0-9a-fA-F]{8})_\d{8}_\d{6}\.tgz$", os.path.basename(bundle))
        sha = match.group(1) if match else os.path.basename(bundle)
        by_sha.setdefault(sha, []).append((bundle, _measured_from_bundle(bundle)))

    def _key(entry):
        if entry is None:
            return None
        return (entry["verdict"], entry.get("passed"), entry.get("total"), tuple(entry.get("failures") or []))

    problems = []
    for sha, group in sorted(by_sha.items()):
        if len(group) < 2:
            print(f"consistency: SHA {sha}: only 1 bundle, nothing to cross-check", file=sys.stderr)
            continue
        ref_bundle, ref = group[0]
        names = sorted(set().union(*(set(meas) for _, meas in group)))
        for bundle, meas in group[1:]:
            for name in names:
                if _key(ref.get(name)) != _key(meas.get(name)):
                    problems.append(
                        f"SHA {sha}: {name} differs between {os.path.basename(ref_bundle)} "
                        f"and {os.path.basename(bundle)}: {_key(ref.get(name))} vs {_key(meas.get(name))}"
                    )
    for problem in problems:
        print(f"VERIFY FAILED: {problem}", file=sys.stderr)
    if problems:
        return 1
    print(
        f"VERIFY OK: {len(bundles)} bundle(s) in {len(by_sha)} source-SHA group(s) agree on all SMHasher3 measurements",
        file=sys.stderr,
    )
    return 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("help", help="show help")

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
    p_run.add_argument("--config", action="append", default=[], help="bazel --config for the benchmark build (e.g. `--config=clang`); works well with .user.bazelrc to pick the toolchain and the recorded compiler")
    p_run.add_argument("--copt", action="append", default=[], help="bazel --copt for the benchmark build (e.g. `--copt=-O3`); allows manual fine tuning of the compiler flags")
    p_run.add_argument("--host_copt", action="append", default=[], help="bazel --host_copt for the benchmark build (e.g. `--host_copt=-O3`); allows manual fine tuning of the host compiler flags")

    p_store = sub.add_parser("store", help="distill raw benchmark JSON to canonical results JSON")
    p_store.add_argument("--raw", required=True)
    p_store.add_argument("--out", required=True)
    p_store.add_argument("--mode", choices=["fast", "full"], default="full")

    p_tables = sub.add_parser("tables", help="render README markdown tables from a dataset (results JSON or .tgz bundle)")
    _add_dataset_arg(p_tables)

    p_compare = sub.add_parser("compare", help="compare two datasets (results JSON or .tgz bundles): per-case Δ%% + geomean")
    p_compare.add_argument("a", metavar="A", help="baseline dataset: results.json[.gz] or a bundle .tgz")
    p_compare.add_argument("b", metavar="B", help="new dataset: results.json[.gz] or a bundle .tgz")

    p_plot = sub.add_parser("plot", help="render SVG ns-vs-length curves from a dataset (results JSON or .tgz bundle)")
    _add_dataset_arg(p_plot)
    p_plot.add_argument("--out", required=True)
    p_plot.add_argument(
        "--kind",
        choices=["throughput", "latency", "all"],
        default="throughput",
        help="which curves to render: 'throughput' (64+128, default), 'latency' (now dense enough for log-log), or 'all'",
    )
    p_plot.add_argument(
        "--scale",
        choices=["log-log", "linear-log"],
        default="log-log",
        help="axis scaling: 'log-log' (default) or 'linear-log' (linear y / log x, to read absolute ns gaps). x is always log",
    )

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
    p_bundle.add_argument("--results", required=True, help="canonical results JSON (drives chart + tables); its context names the machine")
    p_bundle.add_argument("--include", nargs="*", default=[], help="extra files to pack (raw.json.gz, smhasher.json, logs)")
    p_bundle.add_argument("--data-dir", default="mbo/hash/measurements/data")

    p_publish = sub.add_parser("publish", help="render the labeled per-machine perf section (charts + tables) into the README from selected bundles")
    p_publish.add_argument("--bundles", nargs="+", required=True, help="the .tgz bundles to feature, in display order")
    p_publish.add_argument("--charts-dir", default="mbo/hash/measurements/charts")
    p_publish.add_argument("--readme", default="mbo/hash/README.md")

    p_verify = sub.add_parser("verify", help="re-render charts from the README's bundle manifest and diff the committed charts")
    p_verify.add_argument("--readme", default="mbo/hash/README.md")
    p_verify.add_argument("--charts-dir", default="mbo/hash/measurements/charts")

    p_quality = sub.add_parser("quality", help="render the Algorithm-overview + SMHasher3 Results tables into the README from hash_algorithms.json (manual) and a measured bundle")
    p_quality.add_argument("--readme", default="mbo/hash/README.md")
    p_quality.add_argument("bundle_pos", nargs="?", metavar="BUNDLE", help="data bundle .tgz (same as --bundle)")
    p_quality.add_argument("--bundle", help="data bundle .tgz whose per-algorithm SMHasher3 logs are re-parsed for verdict/score/failures")
    p_quality.add_argument("--check", action="store_true", help="verify the README tables match instead of writing (exit 1 on drift)")

    p_consistency = sub.add_parser("consistency", help="verify all data bundles from the same source SHA report identical SMHasher3 measurements (quality is machine-independent)")
    p_consistency.add_argument("--bundles", nargs="+", help="bundles to compare (default: all data/*.tgz)")
    p_consistency.add_argument("--data-dir", default="mbo/hash/measurements/data")

    args = parser.parse_args(argv)

    # One stamp per invocation, so all files a run writes share it. Every
    # written artifact is prefixed `YYYYMMDD_HHMMSS_` so nothing is overwritten
    # and the filename records when it was produced.
    stamp = _timestamp()
    return {
        "help": dispatch_help,
        "run": dispatch_run,
        "store": dispatch_store,
        "tables": dispatch_tables,
        "compare": dispatch_compare,
        "plot": dispatch_plot,
        "smhasher": dispatch_smhasher,
        "bundle": dispatch_bundle,
        "publish": dispatch_publish,
        "verify": dispatch_verify,
        "quality": dispatch_quality,
        "consistency": dispatch_consistency,
    }.get(args.command, dispatch_help)(args, stamp)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
