# Ordered lookup microbenchmarks

`ordered_lookup_baseline.json.gz` records the benchmark-only parent result for the change proposed
in mbo pull request 411. It contains 648 cases with nine repetitions each, built with `-c opt` on
an 18-core Apple M5 Pro (Mac17,9, 64 GB) and measured on 2026-09-04.

The matrix covers `LimitedSet` and `LimitedMap`; capacities 2, 4, 8, 16, 17, and 32; ascending
`std::less` and `CompareLess` plus descending `std::greater`; `index_of`, `lower_bound`, and
`upper_bound`; and keys before, at the first element, between middle elements, at a middle element,
at the last element, and after the container.

Each reported case is the arithmetic mean of its three lowest CPU-time samples out of the nine
repetitions, matching this repository's established microbenchmark aggregation. The raw repetitions
are retained so the aggregation and comparison can be reproduced.

The result was produced with:

```sh
bazel build -c opt //mbo/container:ordered_lookup_benchmark
bazel-bin/mbo/container/ordered_lookup_benchmark \
  --benchmark_repetitions=9 \
  --benchmark_min_time=0.005s \
  --benchmark_out=ordered_lookup_baseline.json \
  --benchmark_out_format=json
gzip -9 ordered_lookup_baseline.json
```

The uncompressed JSON has SHA-256
`dc52c9ff0bca3dff0e586e40d265201e1173ea62aa2dc6249761a90f170c6741`.

Google Benchmark could not read `hw.cpufrequency` or set thread affinity on macOS. Those limitations
apply equally to the baseline and comparison runs; comparisons must use repeated samples and treat
noise statistically rather than relying on a single timing.

## Bounds-unrolling candidate

`ordered_lookup_candidate.json.gz` contains the identically configured raw run from the bounds
unrolling candidate in pull request 411. Its uncompressed SHA-256 is
`5c7ea7391fe92d944674ac42ac4abd6d8d8b7230a397637617caa335d3ea198b`.

`ordered_lookup_candidate_best3.tsv` records the baseline and candidate best-3-of-9 means for all
648 cases. The candidate does not meet the no-regression requirement: 333 cases regress, including
228 by more than 5% and 199 by more than 10%. Lower-bound first/middle cases at capacities 8 and 16
show repeated large losses, with the worst measured case regressing by 151.2%. The implementation
must not proceed on these results.
