# Ordered lookup microbenchmarks

`ordered_lookup_baseline.json.gz` records the benchmark-only parent result for the change proposed
in mbo pull request 411. It contains 648 cases with nine repetitions each, built with `-c opt` on
an 18-core Apple M5 Pro (Mac17,9, 64 GB) and measured on 2026-09-04.

The matrix covers `LimitedSet` and `LimitedMap`; capacities 2, 4, 8, 16, 17, and 32; ascending
`std::less` and `CompareLess` plus descending `std::greater`; `index_of`, `lower_bound`, and
`upper_bound`; and keys before, at the first element, between middle elements, at a middle element,
at the last element, and after the container.

The result was produced with:

```sh
bazel build -c opt //mbo/container:ordered_lookup_benchmark
bazel-bin/mbo/container/ordered_lookup_benchmark \
  --benchmark_repetitions=9 \
  --benchmark_min_time=0.005s \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=ordered_lookup_baseline.json \
  --benchmark_out_format=json
gzip -9 ordered_lookup_baseline.json
```

The uncompressed JSON has SHA-256
`3f208c0cb043d44c0d30b16cd569e6d31055625ce9a7e1b74421e32a41614167`.

Google Benchmark could not read `hw.cpufrequency` or set thread affinity on macOS. Those limitations
apply equally to the baseline and comparison runs; comparisons must use repeated samples and treat
noise statistically rather than relying on a single timing.
