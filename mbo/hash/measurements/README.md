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
  hash_throughput_64.svg       # published 64-bit throughput chart (committed; verifiable from data/)
  hash_throughput_128.svg      # published 128-bit throughput chart (committed; verifiable from data/)
  data/<os-arch-cpu>/*.tgz     # per-machine measurement bundles, Git LFS (see "Data storage")
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

## Data storage, commit policy, contributions

Authoritative numbers cannot come from CI (shared, virtualized runners with CPU
scaling on - see "CI"), so measurements are contributed from real machines
(Apple M5 Pro, AMD Zen5, Intel, ...). Each run produces a fair amount of data:
the raw google/benchmark JSON gzips to ~123 KB (fast) / ~340 KB (full), the
distilled canonical JSON is ~38 KB / ~100 KB, plus the per-algorithm SMHasher3
logs. Across several machines that accumulates, so:

- **Per-machine bundle, Git LFS**
  (`data/<os>-<arch>-<cpu-brand>/<slug>_<cores>c_<compiler>_<gitsha8>_<stamp>.tgz`): one
  gzipped tarball per run holds the _whole_ dataset - the canonical
  `results.json`, the raw `*_raw.json.gz` (for `compare.py` U-tests), and the
  `smhasher.json` + per-algorithm logs. It is Git-LFS-tracked (`.gitattributes`),
  so the blobs stay out of the main pack; unpack to inspect or A/B. Keep the
  current bundle per machine (replace on re-measure); LFS history retains olds.
- **Published charts, plain git** (`hash_throughput_64.svg` / `_128.svg`): the
  human-facing output embedded in the hash README. Small, and - the point -
  _verifiable_: `hash_benchmark_report.py verify --bundle <tgz>` regenerates them
  from the bundled canonical data and fails on any mismatch, so every published
  figure is provably derived from a committed dataset and cannot be hand-edited
  (`_svg_plot` is deterministic: geometry keyed only off the data, no wall-clock).
  The README perf tables are likewise rendered from the canonical data (unpack a
  bundle and run `tables`).
- The loose run outputs in `data/` are staging only and git-ignored; `bundle`
  packs them into the `.tgz`. This directory is dev-only and `.bazelignore`'d, so
  it never enters the module or BCR (the release archive carries LFS pointer
  files here, which is harmless).

## Regenerate / contribute a machine

Run the one-shot runner on the machine you want to add, from the repo root and a
clean `main` checkout (so the provenance is authoritative). It runs the perf
sweep, renders + verifies the charts, runs the SMHasher3 battery, packs the
per-machine LFS bundle, and prints exactly what to commit:

```sh
# Everything, SMHasher3 batteries 4-at-a-time (perf runs first and alone):
mbo/hash/measurements/run_measurements.py --jobs 4
# Pick the toolchain (and thus the recorded compiler): --config clang uses the
# hermetic LLVM clang; omit --config for the native toolchain (gcc on Linux).
# Run once per compiler to compare them - the bundle filenames won't collide:
mbo/hash/measurements/run_measurements.py --config clang --jobs 4
# In-house family only, or perf/chart only:
mbo/hash/measurements/run_measurements.py --algos mumbo,jumbo,dumbo --jobs 1
mbo/hash/measurements/run_measurements.py --skip-smhasher
```

The performance sweep runs first and alone (SMHasher3 would contend for CPU and
skew the sub-ns numbers); the batteries are independent and their pass/fail is
load-independent, so `--jobs` runs several concurrently - trading cores for
wall-clock, not accuracy. SMHasher3 is built and run inside a container
(`build_smhasher3.sh`), so the runner invokes its Linux binary via `docker run`.

Or drive the steps individually (`bundle` packs a run; `verify` audits the
published charts against a bundle; unpack a bundle for its canonical JSON):

```sh
# Perf sweep -> staged canonical results.json + raw.json.gz (from the repo root):
python3 mbo/hash/measurements/hash_benchmark_report.py run \
    --reps 20 --warmup 0.05 \
    --raw mbo/hash/measurements/data/raw.json.gz \
    --out mbo/hash/measurements/data/results.json --tables

# Pack a run's staged outputs into the per-machine Git-LFS bundle:
python3 mbo/hash/measurements/hash_benchmark_report.py bundle \
    --results mbo/hash/measurements/data/<stamp>_results.json \
    --include mbo/hash/measurements/data/<stamp>_raw.json.gz \
             mbo/hash/measurements/data/<stamp>_smhasher.json

# Verify the committed charts match a bundle's data (unpacks, re-renders, diffs):
python3 mbo/hash/measurements/hash_benchmark_report.py verify \
    --bundle mbo/hash/measurements/data/<slug>/<bundle>.tgz

# Re-render the README tables / charts from a bundle's canonical JSON:
tar xzOf <bundle>.tgz results.json > /tmp/results.json
python3 mbo/hash/measurements/hash_benchmark_report.py tables --results /tmp/results.json
python3 mbo/hash/measurements/hash_benchmark_report.py plot --results /tmp/results.json --out /tmp/hash.svg
```

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

The in-house `mumbo`/`jumbo` and `dumbo` are registered by `smhasher3/mbohash.cpp`,
which `#include`s the ACTUAL `mbo/hash` headers (so the real implementation is
verified, not a transcription). `build_smhasher3.sh` copies the headers + plugin
into the tree and registers the source, so one script run yields a SMHasher3
that recognizes `mumbo-64` / `jumbo-128` and `dumbo-64`. Registration names live
in `_SMHASHER_NAMES` and match `SMHasher3 --list`.

Last verified run (2026-07): **mumbo-64/jumbo-128** and **dumbo-64** all PASS
188/188 (dumbo redesigned into a compact single-lane MUM hash).

## Output filenames

Loose staging files the tool writes are prefixed `YYYYMMDD_HHMMSS_` (local wall
clock, one stamp per invocation) so runs never overwrite each other. The
committed artifact is the per-machine bundle
`data/<os>-<arch>-<cpu-brand>/<slug>_<cores>c_<compiler>_<gitsha8>_<stamp>.tgz` - the slug
derived from the dataset's own `uname` + CPU brand, the `compiler` reported by the
benchmark binary (`clang-NN` / `gcc-NN`, so GCC and Clang builds on one machine
stay distinct), the SHA from its provenance - so a bundle is self-identifying and
collision-free across machines and toolchains.

## CI

The benchmark job in `.github/workflows/main.yml` runs **only on `main`** and
**only when something under `mbo/hash` changed** (dorny/paths-filter), with the
same precautions (9 reps, interleaving, warmup). It is `continue-on-error`, so a
noisy or failed benchmark never turns `main` red; results are an artifact per OS
for architecture/compiler shape comparison, not a gate.

## Open items

- **Only the reference machine (Apple M5 Pro) has a committed bundle so far, and
  it is perf-only.** Contribute other machines (AMD Zen5, Intel, ...) by running
  `run_measurements.py` there; and re-run with the SMHasher3 battery to add
  `smhasher.json` + logs to the M5 Pro bundle (the include-order fix makes that
  build reproducible now; the README SMHasher scores already stand from verified
  runs).
- `verify` audits the published _charts_ against a bundle; the README perf
  _tables_ are not yet auto-checked (they render from a bundle's `results.json`
  via `tables`). A marker-delimited tables region + a `verify` extension would
  close that.

Done: per-machine Git-LFS bundles (canonical + raw + SMHasher in one `.tgz`) with
a `verify` audit that regenerates the published charts from the bundled data;
this replaced committing loose canonical/raw files. The SMHasher3 build is
reproducible again (include-order fix).
