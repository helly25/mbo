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
import subprocess
import sys
import time
import threading


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
    parser.add_argument("--config", action="append", default=[], help="bazel --config for the benchmark build (e.g. `--config=clang`); works well with .user.bazelrc to pick the toolchain and the recorded compiler")
    parser.add_argument("--copt", action="append", default=[], help="bazel --copt for the benchmark build (e.g. `--copt=-O3`); allows manual fine tuning of the compiler flags")
    parser.add_argument("--host_copt", action="append", default=[], help="bazel --host_copt for the benchmark build (e.g. `--host_copt=-O3`); allows manual fine tuning of the host compiler flags")
    parser.add_argument("--name", help="Optional name to save in the context")
    parser.add_argument("--filename_extra", help="Extra information appended to the generated base filename.")
    parser.add_argument(
        "--workdir",
        default=os.path.expanduser("~/.cache/mbo-hash-smh"),
        help="SMHasher3 build tree (must be under a Docker-shared path, i.e. $HOME)",
    )
    parser.add_argument("--image", default="gcc:13", help="container image the binary runs in (matches build_smhasher3.sh)")
    parser.add_argument("--skip-perf", action="store_true", help="skip the performance sweep + chart")
    parser.add_argument("--skip-smhasher", action="store_true", help="skip the SMHasher3 battery")
    parser.add_argument("--log", action="store_true", help="write a log file")
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

    log_file_obj = None
    if args.log:
        # 1. Open the log file
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        log_file_path = os.path.join(data, f"{timestamp}_benchmark.log")
        log_fd = os.open(log_file_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
        extras.append(log_file_path)

        # 2. Save original stdout/stderr file descriptors to restore later
        saved_stdout_fd = os.dup(1)
        saved_stderr_fd = os.dup(2)

        # 3. Create OS pipes for stdout and stderr
        r_out, w_out = os.pipe()
        r_err, w_err = os.pipe()

        # 4. Redirect FDs 1 and 2 to the write ends of the pipes
        os.dup2(w_out, 1)
        os.dup2(w_err, 2)

        # 5. Background thread to copy pipe output to BOTH original FD and log file
        def tee_pipe(pipe_read_fd, original_fd, log_fd):
            while True:
                data = os.read(pipe_read_fd, 1024)
                if not data:
                    break
                os.write(original_fd, data)
                os.write(log_fd, data)

        t1 = threading.Thread(target=tee_pipe, args=(r_out, saved_stdout_fd, log_fd), daemon=True)
        t2 = threading.Thread(target=tee_pipe, args=(r_err, saved_stderr_fd, log_fd), daemon=True)
        t1.start()
        t2.start()

    if not args.skip_perf:
        cfg = []
        if args.config:
            cfg.extend([f"--config={arg}" for arg in args.config])
        if args.copt:
            cfg.extend([f"--copt={arg}" for arg in args.copt])
        if args.host_copt:
            cfg.extend([f"--host_copt={arg}" for arg in args.host_copt])
        if args.name:
            cfg.extend([f"--name={args.name}"])
        print(">>> [perf] full performance sweep (runs solo for clean numbers)", file=sys.stderr)
        # A single FULL run: the dense curve AND (via readme_sizes in its context) the
        # curated README table are both extracted from it downstream by `publish`.
        subprocess.run(
            [*report, "run", "--mode", "full", "--reps", str(args.reps),
             "--raw", os.path.join(data, "raw.json.gz"), "--out", os.path.join(data, "results.json")] + cfg,
            check=True,
        )
        canonical = newest(os.path.join(data, "*_results.json"))
        extras.append(newest(os.path.join(data, "*_raw.json.gz")))

    if not args.skip_smhasher:
        print(f">>> [smhasher] building SMHasher3 (workdir {args.workdir})", file=sys.stderr)
        print(f">>> [smhasher] {os.path.join(meas, 'build_smhasher3.sh')} {args.workdir}", file=sys.stderr)
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

    # Flush Python buffers before restoring FDs
    sys.stdout.flush()
    sys.stderr.flush()

    if args.log:
        # Restore original FDs
        os.dup2(saved_stdout_fd, 1)
        os.dup2(saved_stderr_fd, 2)
        os.close(saved_stdout_fd)
        os.close(saved_stderr_fd)
        os.close(w_out)
        os.close(w_err)
        os.close(log_fd)

    bundle = None
    if canonical:
        cfg = []
        if args.filename_extra:
            cfg.extend([f"--filename_extra={args.filename_extra}"])
        print(">>> [bundle] packing the per-machine .tgz (canonical + raw + SMHasher)", file=sys.stderr)
        bundle = subprocess.run(
            [*report, "bundle", "--results", canonical, "--include", *extras, "--data-dir", data] + cfg,
            capture_output=True, text=True, check=True,
        ).stdout.strip()
    elif not args.skip_smhasher:
        print("NOTE: smhasher-only run has no perf canonical to key a bundle on; re-run with perf to bundle.", file=sys.stderr)

    report_py = os.path.join(meas, "hash_benchmark_report.py")
    print(
        "\n".join(
            [
                "",
                "=== done ===",
                "This run wrote NOTHING to the tree except the bundle below, so it stays",
                "authoritative and you can run other compilers/machines back-to-back.",
                "Commit the per-machine data bundle (Git LFS):",
            ]
            + ([f"  git add {bundle}"] if bundle else [])
            + [
                "Then render the README charts + tables from the machines you want to feature:",
                f"  {report_py} publish --bundles <b1.tgz> <b2.tgz> ...",
                "  git add mbo/hash/README.md mbo/hash/measurements/charts",
                "and re-verify the committed charts against their data any time:",
                f"  {report_py} verify",
            ]
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
