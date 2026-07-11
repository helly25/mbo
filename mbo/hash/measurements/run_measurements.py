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
    parser.add_argument("--config", help="bazel --config for the perf benchmark (e.g. clang, gcc); picks the toolchain + recorded compiler")
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

    canonical = None  # the run's distilled results.json (drives the charts and the bundle)
    extras = []  # extra files packed alongside the canonical in the per-machine bundle

    if not args.skip_perf:
        print(">>> [perf] full performance sweep (runs solo for clean numbers)", file=sys.stderr)
        with open(os.path.join(data, "README_tables.md"), "w") as tables:
            subprocess.run(
                [*report, "run", "--mode", "full", "--reps", str(args.reps),
                 "--raw", os.path.join(data, "raw.json.gz"), "--out", os.path.join(data, "results.json"), "--tables"]
                + (["--config", args.config] if args.config else []),
                stdout=tables, check=True,
            )
        canonical = newest(os.path.join(data, "*_results.json"))
        extras.append(newest(os.path.join(data, "*_raw.json.gz")))
        print(">>> [perf] rendering ns-vs-length charts (64-bit + 128-bit, log-log)", file=sys.stderr)
        subprocess.run([*report, "plot", "--results", canonical, "--out", os.path.join(data, "hash_throughput.svg")], check=True)
        for width in ("64", "128"):
            shutil.copy(
                newest(os.path.join(data, f"*_hash_throughput_{width}.svg")),
                os.path.join(meas, f"hash_throughput_{width}.svg"),
            )

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
        smh = glob.glob(os.path.join(data, "*_smhasher.json"))
        if smh:
            extras.append(max(smh, key=os.path.getmtime))
        extras.extend(sorted(glob.glob(os.path.join(data, "*_smhasher_*.log.gz"))))

    bundle = None
    if canonical:
        print(">>> [bundle] packing the per-machine .tgz (canonical + raw + SMHasher)", file=sys.stderr)
        bundle = subprocess.run(
            [*report, "bundle", "--results", canonical, "--include", *extras, "--data-dir", data],
            capture_output=True, text=True, check=True,
        ).stdout.strip()
        print(">>> [verify] published charts vs the bundle data", file=sys.stderr)
        subprocess.run([*report, "verify", "--bundle", bundle, "--charts-dir", meas], check=False)
    elif not args.skip_smhasher:
        print("NOTE: smhasher-only run has no perf canonical to key a bundle on; re-run with perf to bundle.", file=sys.stderr)

    print(
        "\n".join(
            [
                "",
                "=== done ===",
                "Commit the published charts (plain git) and the per-machine data bundle (Git LFS):",
                f"  git add {meas}/hash_throughput_64.svg {meas}/hash_throughput_128.svg",
            ]
            + ([f"  git add {bundle}"] if bundle else [])
            + [
                f"Refresh the README perf tables by pasting {data}/README_tables.md, and re-embed the charts:",
                "  ![mbo/hash 64-bit throughput](measurements/hash_throughput_64.svg)",
                "  ![mbo/hash 128-bit throughput](measurements/hash_throughput_128.svg)",
                "Anyone can re-verify the published charts against the committed data:",
                f"  {os.path.join(meas, 'hash_benchmark_report.py')} verify --bundle {bundle or '<data/<slug>/<bundle>.tgz>'}",
            ]
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
