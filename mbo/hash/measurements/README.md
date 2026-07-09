# mbo/hash measurements

Dev-only tooling to run `//mbo/hash:hash_benchmark`, store the results with full
provenance, render the README performance tables, and plot ns-vs-length curves.
This is a **pure development dependency**: it is its own Bazel module
(`MODULE.bazel` here) and is listed in the repository-root `.bazelignore`, so it
is never part of the `helly25_mbo` module, its `//...` target universe, or any
published/BCR offering. Consumers of the library never pull any of this in
(including heavier plotting dependencies).

This is a living design doc - update it when the layout or policy changes.

## Why this exists

The `mbo/hash/README.md` numbers are sub-nanosecond and are taken on a shared
developer machine. A single median run is dominated by run-to-run contention
noise: a change smaller than ~0.2 ns is invisible, and medians of two runs can
even invert the true ordering. So we:

- Aggregate by the **mean of the k fastest repetitions** (`k = reps/3`, so 3
  of 9). Contention only ever slows a run down, so the low tail is the
  uncontended cost; taking the _mean of the fastest few_ rejects the
  single-sample noise a pure minimum would keep, while still excluding the
  contended runs the median includes.
- Apply google/benchmark's own precautions (below).
- Store the **full run context** so a number is always attributable to a
  specific machine, load, toolchain, and source revision.

## Layout

```text
mbo/hash/measurements/
  README.md                    # this design doc
  MODULE.bazel                 # separate dev module (isolates plotting deps)
  run_measurements.py          # one-shot authoritative runner (perf + chart + parallel smhasher)
  hash_benchmark_report.py     # run / store / tables / plot / smhasher (stdlib only)
  build_smhasher3.sh           # reproducible SMHasher3 build (clone + fixes + install plugin + container gcc)
  smhasher3/mbohash.cpp        # in-house mumbo/jumbo and dumbo SMHasher3 registration (includes the real headers)
  hash_benchmark_results.json  # canonical distilled results (committed provenance for the README tables)
  data/                        # complete raw datasets + SMHasher3 logs (see "Data storage")
```

The C++ benchmark itself (`mbo/hash/hash_benchmark.cc`,
`//mbo/hash:hash_benchmark`) stays in the main module - it is tightly coupled to
the hash internals and is already `testonly` + `tags = ["manual"]`, so it is
dev-only but not shipped. Only the orchestration/storage/reporting lives here.

## What is captured (provenance)

Every stored dataset records, from google/benchmark's `context` plus a few
augmentations, enough to know exactly what produced it:

- **date/time** of the measurement.
- **git revision** of the measured source and whether the tree was dirty - a
  number is meaningless without the exact code it measured.
- **host name**, **CPU count**, **CPU nominal MHz**, **CPU brand/model**, cache
  sizes.
- **OS / kernel** (uname).
- **compiler** and version (the benchmark is built `-c opt`).
- **load average** at measurement time (so a noisy run is identifiable).
- **`cpu_scaling_enabled`**: if frequency scaling is on (it is, on macOS), the
  absolute numbers are unreliable; the tool records it and warns. The min
  aggregate mitigates but does not eliminate this.
- benchmark **parameters**: repetitions, min-time, warmup-time, whether random
  interleaving was on, and the aggregate used (best-`k` mean) with its `k`.

## Methodology / precautions

- Build `-c opt`.
- **9 repetitions** by default: google/benchmark's `tools/compare.py` recommends
  at least 9 for its Mann-Whitney U-test, so two stored raw datasets can be
  compared for statistical significance (`compare.py benchmarks a.json b.json`).
- **`--benchmark_enable_random_interleaving`**: interleaves the repetitions of
  every case, spreading each case across the run so thermal drift and "first
  case runs cold" bias do not accrue to whichever benchmark ran first.
- **`--benchmark_min_warmup_time`**: warm caches and CPU frequency before timing.
- Aggregate by the **best-`k` mean** (see above); we also keep `best_cv` (spread
  of those `k`) plus the full-set min / median / mean per case for transparency.
- Fixed RNG seeds in the benchmark give identical key bytes per length across
  runs (so only timing varies). Throughput is hot-cache (the same key is reused
  in the loop) by design; the latency benchmark serializes a dependency chain
  over a fixed 1024-key set - the cost profile a hash table actually pays.

## Statistics stored per case

Per (width, algorithm, length): `best` (the headline mean of the fastest `k`),
`best_cv` (coefficient of variation **of those `k` samples** - how stable the
estimate is; a full-set CV would just re-report the contention we excluded),
plus `min`, `median`, `mean` (full-set, for context) and the repetition count.
The README tables use `best`; `best_cv` flags a case whose fast tail is itself
noisy and worth re-measuring.

## Data storage, compression, commit policy

Measured sizes (fast/18-size set; the full/51-size set is ~2.8x): raw
google/benchmark JSON is **1.1 MB** (fast) / **~3 MB** (full) - too large and
too noisy (per-iteration timings) to text-dump; gzip takes it to **123 KB** /
**~340 KB**. The distilled canonical JSON is **38 KB** (fast) / **~100 KB**
(full), and with sorted keys it diffs cleanly. So:

- **Canonical results** (`hash_benchmark_results.json`): distilled per-case
  stats + context. Small and human-diffable - **committed as text**; it is the
  provenance for the README tables and the input to the plotter. This is the
  answer to "just text-dump them": the _canonical_ JSON, yes; the _raw_, no.
- **Complete raw datasets** (`data/`): google/benchmark's full JSON (every
  repetition), `data/<date>-<host>-<gitsha>.json.gz`, **gzip-compressed**. Only
  needed for a `compare.py` U-test. Policy: commit the gzipped raw only for an
  **authoritative** dataset (a clean `main` checkout - see the workflow note),
  so history stays reproducible without accumulating a raw dump per dev push.
  Dev-branch raws stay git-ignored (`data/` ignores everything but `.gitkeep`).

## Regenerate

The one-shot runner does all three steps (perf sweep, chart, SMHasher3 battery)
and prints exactly what to commit. Run it from a clean `main` checkout so the
provenance is authoritative:

```sh
# Everything, SMHasher3 batteries 4-at-a-time (perf runs first and alone):
mbo/hash/measurements/run_measurements.py --jobs 4
# In-house family only, or perf/chart only:
mbo/hash/measurements/run_measurements.py --algos mumbo,jumbo,dumbo --jobs 1
mbo/hash/measurements/run_measurements.py --skip-smhasher
```

The performance sweep runs first and alone (SMHasher3 would contend for CPU and
skew the sub-ns numbers); the batteries are independent and their pass/fail is
load-independent, so `--jobs` runs several concurrently - trading cores for
wall-clock, not accuracy. SMHasher3 is built and run inside a container
(`build_smhasher3.sh`), so the runner invokes its Linux binary via `docker run`.

Or drive the steps individually:

```sh
# From the repository root (the script shells out to `bazel run` there):
python3 mbo/hash/measurements/hash_benchmark_report.py run \
    --reps 20 --warmup 0.05 \
    --raw mbo/hash/measurements/data/$(date +%Y%m%d)-raw.json \
    --out mbo/hash/measurements/hash_benchmark_results.json --tables

# Re-render the README tables from the committed canonical JSON (no bazel):
python3 mbo/hash/measurements/hash_benchmark_report.py tables \
    --results mbo/hash/measurements/hash_benchmark_results.json

# Plot ns-vs-length curves (dependency-free SVG):
python3 mbo/hash/measurements/hash_benchmark_report.py plot \
    --results mbo/hash/measurements/hash_benchmark_results.json --out /tmp/hash.svg

# SMHasher3 quality over all algorithms (needs a built SMHasher3, see above):
python3 mbo/hash/measurements/hash_benchmark_report.py smhasher \
    --smhasher3 /path/to/SMHasher3 --algos all --out mbo/hash/measurements/smhasher_results.json
```

Output filenames are timestamp-prefixed (see below), so `--raw` / `--out` land
as `data/YYYYMMDD_HHMMSS_...`.

## SMHasher3 quality (`smhasher`)

`hash_benchmark_report.py smhasher --algos all --smhasher3 <path>` runs the
SMHasher3 battery over a set of algorithms (default `all`, which **includes the
legacy `dumbo`**) and stores a pass/fail + failing-family summary as JSON with
the same provenance, plus each run's full log under `data/`.

It drives a **built SMHasher3 executable** (`--smhasher3`). `build_smhasher3.sh`
is the reproducible build: it clones SMHasher3 at the pinned commit, applies the
two documented fixes (missing `<cstdlib>` in `lib/AEStest.cpp`; replace
`-march=native`), and builds it with gcc in a container - matching the
methodology in `../README.md`. The third-party algorithms are SMHasher3
built-ins and work immediately.

The in-house `mumbo`/`jumbo`/`dumbo` are registered by `smhasher3/mbohash.cpp`,
which `#include`s the ACTUAL `mbo/hash` headers (so the real implementation is
verified, not a transcription). `build_smhasher3.sh` copies the headers + plugin
into the tree and registers the source, so one script run yields a SMHasher3
that recognizes `mumbo-64` / `jumbo-128` and `dumbo-64`. Registration names live
in `_SMHASHER_NAMES` and match `SMHasher3 --list`.

Last verified run (2026-07): **mumbo-64/jumbo-128** and **dumbo-64** all PASS
188/188 (dumbo redesigned into a compact single-lane MUM hash).

## Output filenames

Every file the tool writes is prefixed `YYYYMMDD_HHMMSS_` (local wall clock, one
stamp per invocation), so runs never overwrite each other and the filename
records when it was produced. The committed canonical results file is therefore
a specific timestamped file; `tables`/`plot` take an explicit `--results` path.

## CI

The benchmark job in `.github/workflows/main.yml` runs **only on `main`** and
**only when something under `mbo/hash` changed** (dorny/paths-filter), with the
same precautions (9 reps, interleaving, warmup). It is `continue-on-error`, so a
noisy or failed benchmark never turns `main` red; results are an artifact per OS
for architecture/compiler shape comparison, not a gate.

## Open items

- **SMHasher3 build pin is broken**: `build_smhasher3.sh` pins commit
  `6ab4343`, but `smhasher3/mbohash.cpp` was written against a newer SMHasher3
  API (`FLAG_IMPL_*`, `HashInfo::verification_LE`/`bits`); the pinned commit uses
  `FLAG_ENUM_IMPL_*` and the plugin does not compile there. Re-pin to a commit
  whose API matches the plugin (or port the plugin) so the `smhasher` mode can
  produce a committed `smhasher.json`. The README SMHasher scores stand (from
  the earlier verified runs on a matching tree); only the machine-readable
  record is blocked.
- Optional guard: a check that the README tables match the committed canonical
  JSON so they cannot silently drift (they match as of the latest authoritative
  run below).

Done: the authoritative perf dataset + gzipped raw are committed
(`hash_benchmark_results.json`, `data/*_raw.json.gz`), and the log-log
throughput curves (`hash_throughput_64.svg` / `_128.svg`) are committed and
embedded in the hash README.
