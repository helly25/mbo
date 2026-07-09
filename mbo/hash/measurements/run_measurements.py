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

"""One-shot authoritative measurement run for mbo/hash.

Orchestrates the three `hash_benchmark_report.py` steps into a single command:
the full performance sweep, the ns-vs-length chart, and the SMHasher3 quality
battery (run in parallel). It saves committed-size artifacts under
`mbo/hash/measurements/` and prints exactly what to commit.

Run this from a CLEAN `main` checkout: the stored provenance is only tagged
authoritative when the tree is clean `main`, and the committed dataset is meant
to be reproducible from a known revision.

The performance sweep runs first and ALONE - it is sub-nanosecond and SMHasher3
would contend for CPU cores and skew it - then the batteries start. The
batteries are independent and their pass/fail verdicts do not depend on CPU
load (only SMHasher3's own Speed sub-test would, and that number is unused), so
they run `--jobs` at a time; this only trades cores for wall-clock.

SMHasher3 is built and run inside a container (`build_smhasher3.sh`), so its
Linux binary is invoked via `docker run` here rather than natively.

Examples:
    # Everything, batteries 4-at-a-time (from repo root, clean main):
    mbo/hash/measurements/run_measurements.py --jobs 4
    # In-house family only, sequential:
    mbo/hash/measurements/run_measurements.py --algos mumbo,jumbo,dumbo --jobs 1
    # Refresh perf + chart only:
    mbo/hash/measurements/run_measurements.py --skip-smhasher
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys


def newest(pattern):
    matches = glob.glob(pattern)
    if not matches:
        raise FileNotFoundError(pattern)
    return max(matches, key=os.path.getmtime)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--algos", default="all", help="SMHasher3 algorithms (default 'all'; e.g. mumbo,jumbo,dumbo)")
    parser.add_argument("--jobs", type=int, default=4, help="SMHasher3 batteries to run concurrently (default 4)")
    parser.add_argument("--reps", type=int, default=9, help="benchmark repetitions (default 9)")
    parser.add_argument(
        "--workdir",
        default=os.path.expanduser("~/.cache/mbo-hash-smh"),
        help="SMHasher3 build tree (must be under a Docker-shared path, i.e. $HOME)",
    )
    parser.add_argument("--image", default="gcc:13", help="container image the binary runs in (matches build_smhasher3.sh)")
    parser.add_argument("--skip-perf", action="store_true", help="skip the performance sweep + chart")
    parser.add_argument("--skip-smhasher", action="store_true", help="skip the SMHasher3 battery")
    args = parser.parse_args(argv)

    repo = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True, check=True
    ).stdout.strip()
    os.chdir(repo)
    meas = "mbo/hash/measurements"
    data = os.path.join(meas, "data")
    report = [sys.executable, os.path.join(meas, "hash_benchmark_report.py")]
    os.makedirs(data, exist_ok=True)

    branch = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True, check=True
    ).stdout.strip()
    dirty = bool(subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True, check=True).stdout.strip())
    if branch != "main" or dirty:
        print(
            f"WARNING: not a clean 'main' checkout (on '{branch}'{', dirty' if dirty else ''}); "
            "the dataset will be recorded as NON-authoritative - commit it only from clean 'main'.",
            file=sys.stderr,
        )

    if not args.skip_perf:
        print(">>> [perf] full performance sweep (runs solo for clean numbers)", file=sys.stderr)
        with open(os.path.join(data, "README_tables.md"), "w") as tables:
            subprocess.run(
                [*report, "run", "--mode", "full", "--reps", str(args.reps),
                 "--raw", os.path.join(data, "raw.json.gz"), "--out", os.path.join(data, "results.json"), "--tables"],
                stdout=tables, check=True,
            )
        shutil.copy(newest(os.path.join(data, "*_results.json")), os.path.join(meas, "hash_benchmark_results.json"))
        print(">>> [perf] rendering ns-vs-length chart", file=sys.stderr)
        subprocess.run(
            [*report, "plot", "--results", os.path.join(meas, "hash_benchmark_results.json"),
             "--out", os.path.join(data, "hash_throughput.svg")],
            check=True,
        )
        shutil.copy(newest(os.path.join(data, "*_hash_throughput.svg")), os.path.join(meas, "hash_throughput.svg"))

    if not args.skip_smhasher:
        print(f">>> [smhasher] building SMHasher3 (workdir {args.workdir})", file=sys.stderr)
        subprocess.run([os.path.join(meas, "build_smhasher3.sh"), args.workdir], check=True)
        # The binary is a container build; invoke it via `docker run` (its tree
        # mounted at /src), which run_smhasher treats as a command prefix.
        tree = os.path.join(args.workdir, "smhasher3")
        wrapper = f"docker run --rm -v {tree}:/src -w /src {args.image} ./build/SMHasher3"
        print(f">>> [smhasher] battery (algos={args.algos}, jobs={args.jobs}); ~12 min per algorithm", file=sys.stderr)
        subprocess.run(
            [*report, "smhasher", "--smhasher3", wrapper, "--algos", args.algos, "--jobs", str(args.jobs),
             "--out", os.path.join(data, "smhasher.json"), "--raw-dir", data],
            check=False,
        )
        for log in glob.glob(os.path.join(data, "*_smhasher_*.log")):
            subprocess.run(["gzip", "-f", log], check=False)

    print(
        "\n".join(
            [
                "",
                "=== done ===",
                "Committed-size artifacts (raw ~316 KB gzip, canonical ~100 KB, SVG ~16 KB, each SMHasher log ~20 KB gzip):",
                f"  {meas}/hash_benchmark_results.json   canonical per-case perf stats (text, diffable)",
                f"  {meas}/hash_throughput.svg           ns-vs-length chart (embed in README.md)",
                f"  {data}/*_raw.json.gz                 full raw benchmark JSON (compare.py U-tests)",
                f"  {data}/*_smhasher.json               SMHasher3 pass/fail + failing families",
                f"  {data}/*_smhasher_*.log.gz           per-algorithm SMHasher3 logs",
                f"  {data}/README_tables.md              rendered perf tables to paste into README.md",
                "",
                "data/ is .gitignored; an authoritative commit is:",
                f"  git add {meas}/hash_benchmark_results.json {meas}/hash_throughput.svg",
                f"  git add -f {data}/*_raw.json.gz {data}/*_smhasher.json {data}/*_smhasher_*.log.gz",
                "Embed the chart in README.md with:",
                "  ![mbo/hash throughput](measurements/hash_throughput.svg)",
            ]
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
