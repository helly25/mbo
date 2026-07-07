# mbo/hash - fast, constexpr-safe, non-cryptographic hashing

Spec-based, fast, constexpr-compatible, Apache-licensed, no-nonsense hash
implementations. Algorithm reference and API listing: see the
[repository README](../../README.md). Quality (SMHasher3) and performance
measurements for all algorithms: below.

## Offerings

Three entry points, split by contract:

- **`hash.h` / `:hash_cc` - deterministic hashing.** `GetHash64` /
  `GetHash128` / `GetHash32<Algo>`, the `Hasher<Algo>` container functor, and
  `Streamer<Algo>` incremental hashing - all constexpr-safe and fully
  reproducible for a given library version. Use for hash tables (heterogeneous
  string lookup), tokenization/interning, compile-time hashing
  (`static_assert`, switch-on-hash), and cross-process consistency within one
  build. Values are not a persistence or wire format.
- **`hash_mangle.h` / `:hash_mangle_cc` - deliberately unstable hashing.**
  `GetHash` / `MangledHasher<Algo>`: `GetHash64` XORed with one build-selected
  constant, so values do not compare across independently configured builds.
  Use when hash values must not quietly become load-bearing (persisted tables,
  golden values, cross-build protocols) - the instability is the feature.
  Still constexpr; semantics and design rationale in the build-seed mangle
  section, the flags in the Configuration section below.
- **`hash_extra.h` / `:hash_extra_cc` - NOTICE-bearing algorithms.** Canonical
  rapidhash, xxh3, and xxh64 transcriptions, for interop with externally
  defined values and for comparison. Shipping a binary that links this target
  requires shipping the repository-root [NOTICE](../../NOTICE).

## Principles

- **Canonical or honest**: third-party algorithms (rapidhash, XXH64/XXH3,
  MurmurHash3, SipHash, FNV-1a) are transcriptions producing the exact
  published reference values on every platform, pinned by reference vectors
  and differential tests against the reference libraries. The in-house `mumbo`
  algorithm is documented with its measured quality and performance data
  (below) and its design iterations.
- **constexpr-safe single path**: compile-time and run-time evaluation always
  agree; streaming (where provided) equals the one-shot value by contract.
- **Apache-2.0 with clean attribution**: transcription notices live in the
  repository-root [NOTICE](../../NOTICE); [LICENSE](../../LICENSE) stays pure
  Apache-2.0. No crypto-library
  dependencies - digests and hashes are spec-frozen pure functions that we
  verify against official vectors instead of trusting an unverifiable supply
  chain (see [mbo/digest/README.md](../digest/README.md) for the full argument).
- **Not cryptographic**: values are neither stable across versions nor safe
  against adversaries (SipHash with a secret seed is the DoS-resistant
  option). For message digests (SHA-2, MD5 interop, ...) see
  [mbo/digest](../digest/README.md).

## Algorithm overview

| Algorithm   | Widths | Available via                     | NOTICE                  | Seeded | Streaming | SMHasher3      |
| ----------- | ------ | --------------------------------- | ----------------------- | ------ | --------- | -------------- |
| `mumbo`     | 64     | `hash.h` (default 64/32)          | none (in-house)         | yes    | yes       | PASS           |
| `jumbo`     | 128    | `hash.h` (default 128)            | none (in-house)         | yes    | yes (64)  | PASS           |
| `murmur3`   | 64/128 | `hash.h`                          | none (public domain)    | yes    | no        | FAIL (123)     |
| `siphash`   | 64     | `hash.h`                          | none (CC0)              | keyed  | yes       | PASS (186)     |
| `fnv1a`     | 64     | `hash.h`                          | none (public domain)    | yes    | no        | FAIL (7!)      |
| `dumbo`     | 64     | `hash.h`                          | none (in-house)         | no     | no        | n/a (legacy)   |
| `rapidhash` | 64     | `hash_extra.h` + `:hash_extra_cc` | **MIT - ship NOTICE**   | yes    | no        | PASS           |
| `xxh64`     | 64     | `hash_extra.h` + `:hash_extra_cc` | **BSD-2 - ship NOTICE** | yes    | yes       | FAIL (181)     |
| `xxh3`      | 64/128 | `hash_extra.h` + `:hash_extra_cc` | **BSD-2 - ship NOTICE** | yes    | no        | FAIL (166/162) |

Notes: `fnv1a` is the algorithm family many `std::hash` implementations use
(e.g. MSVC) - included as the familiar baseline. `siphash` is a keyed PRF:
the DoS-resistant choice when the seed is a secret. `dumbo` is the legacy
pre-0.13 hash, kept for comparison only. Linking `:hash_extra_cc` requires
shipping the repository-root [NOTICE](../../NOTICE) (see "Third-party
components" in the [repository README](../../README.md)).

## Build-seed mangle (`hash_mangle.h` / `:hash_mangle_cc`)

`mbo::hash::GetHash` and `MangledHasher<Algo>` equal `GetHash64` XORed with
ONE build-selected constant, so values deliberately do not compare across
independently configured builds - precomputed tables or persisted values
cannot silently become load-bearing. Everything stays constexpr: the constant
is generated into a header by folding the module's own version (from
`MODULE.bazel` via `native.module_version()` - no duplicated version
declaration anywhere) with two custom Bazel flags (see the
Configuration section below). Folding the version in means every release
rotates the constant by construction - at zero marginal cost, since a release
recompiles all dependents anyway - so "values are not stable across library
versions" holds by construction even under default flags.

Neither the version nor the raw seed string ever reaches a C++ action key:
both fold to a bucket inside the header-generation rule, so build/remote
caches see at most `N + 1` header variants and converge no matter how often
the seed rotates (per user, per release, or never - a `.bazelrc` one-liner
either way). Only targets depending on `:hash_mangle_cc` rebuild on rotation;
`hash.h` / `:hash_cc` users are never touched. One constant applies per
program: linking objects compiled under different flag values is an ODR
violation (within a single Bazel build consistency is structural).

### Design: constexpr rules out ASLR, buckets bound the churn

The mangle wants ASLR-style entropy - values that shift outside anyone's
control, so nothing can quietly start depending on them. Three constraints
shape the implementation:

1. **constexpr is non-negotiable, which rules out runtime entropy.** Every
   entry point participates in constant evaluation, including `GetHash`. True
   ASLR (absl-style: mixing in the address of a global) or any startup-time
   random seed cannot appear in a constant expression - adopting one would
   split the API into a constexpr unmangled half and a runtime mangled half.
   The entropy must be a compile-time constant, so it can only be injected at
   build time.

2. **Build-time entropy must not defeat caching.** A naive build-time seed
   (hashing `__DATE__`/`__TIME__`, as an earlier iteration did) takes a new
   value on every compile: every rotation is a cold miss for build and remote
   caches, and per-TU evaluation can even hand two translation units of one
   binary different constants (an ODR violation). Both problems stem from
   unbounded seed values entering the compiler's inputs.

3. **Churn must be bounded and adjustable.** Hence the two-flag design: the
   library version and the seed string are folded to one of `N` buckets
   inside the header-generation rule, and only the resulting constant reaches
   C++ action keys - a raw value (version, user name, date) never does.
   Caches therefore hold at most `N + 1`
   variants of the mangle-dependent build graph (objects, links, cached test
   results), no matter what rotates through the seed flag.
   `--//mbo/hash:mangle_seed_buckets` is the dial between entropy and cache
   footprint: `0` is reproducible (no entropy, no churn), `1` is one pinned
   constant (distinct from `GetHash64`, zero churn), larger `N` buys more
   variation at proportional cache cost. The `:hash_cc` / `:hash_mangle_cc`
   target split completes the containment: plain hash users sit entirely
   outside the churn.

## Configuration

All configuration lives on the mangle - the deterministic `hash.h` and
`hash_extra.h` entry points have no knobs. Set the flags on the command line
or in `.bazelrc`; prefix them with `@helly25_mbo` when the library is
consumed as a dependency (e.g. `--@helly25_mbo//mbo/hash:mangle_seed=...`).

- `--//mbo/hash:mangle_seed` (string, default `""`): any printable-ASCII
  string - user name, release tag, date - folded together with the library
  version to select the mangle constant.
- `--//mbo/hash:mangle_seed_buckets` (int, default `8`): the entropy/cache
  dial. `0` disables the mangle (`GetHash == GetHash64`, fully reproducible
  builds); `1` pins one stable constant across releases and seeds (`GetHash`
  stays distinct from `GetHash64`, zero churn); `N >= 2` bounds the variation
  to `N` constants at proportional cache footprint.

Example `.bazelrc` policies:

```sh
# Reproducible builds / golden tests: disable the mangle entirely.
build --//mbo/hash:mangle_seed_buckets=0

# Per-user variation on top of the per-release rotation.
build --//mbo/hash:mangle_seed=alice
```

Under default flags the constant still rotates once per release (the library
version is folded in). Non-Bazel builds fall back to the checked-in
`internal/hash_mangle_seed.h.in`, which carries the default-flag constant for
the current version; `//mbo/hash:hash_mangle_seed_default_test` keeps it
byte-identical to the generated header, and a version bump regenerates it
(command in the file's header comment).

## Abseil interop

The two frameworks compose rather than compete - pick by contract:
`absl::Hash` is per-process randomized and tuned for tiny in-process keys;
`mbo::hash` is canonical, cross-platform, constexpr, and streamable.

- **Containers**: `DefaultHasher` (any `Hasher<Algo>` / `MangledHasher<Algo>`)
  drops into `absl`/`std` hash containers as the `Hash` parameter for string
  keys, with heterogeneous `string_view` lookup - this replaces absl's native
  hashing for byte keys outright.
- **Injecting mbo values into absl combining**: compute the mbo hash - via
  `Streamer` for chunked content - and combine the resulting integer like any
  other field. In-repo precedent: `Hash128` and `mbo::types::tstring` do
  exactly this.

  ```cpp
  template<typename H>
  friend H AbslHashValue(H state, const Document& doc) {
    mbo::hash::Streamer<mbo::hash::DefaultHashAlgorithm> stream;
    for (std::string_view chunk : doc.chunks) {
      stream.Update(chunk);
    }
    return H::combine(std::move(state), stream.Finalize(), doc.id);
  }
  ```

  The mbo value itself stays deterministic; the surrounding absl hash remains
  per-process seeded (which is what a container wants). Use the mangled
  `GetHash` instead of `GetHash64` where the injected value itself must not be
  comparable across builds.

- **The other direction needs care**: folding `absl::HashOf(x)` into mbo
  values (e.g. via `CombineHashes`) imports absl's per-process randomization -
  the result is no longer stable across runs, let alone builds. Only do this
  for values that never leave the process.
- **Replacing `absl::Hash` for arbitrary types**: possible by design but a
  deliberate project, not a flag. `AbslHashValue` is framework-agnostic: any
  hash state implementing `combine` / `combine_contiguous` (plus unordered
  support) can execute every existing `AbslHashValue` overload, so a
  mumbo-backed state could swap the algorithm underneath all absl-hashable
  types. Worth it only when structured types need canonical or constexpr
  hashing; for byte keys the container functor above already does the job.

## Performance

Measured and rendered by `mbo/hash/measurements/hash_benchmark_report.py`
(Apple Silicon arm64, Apple clang, `-c opt`; see
[mbo/hash/measurements/](measurements/README.md)). Numbers are the **mean of the
3 fastest of 9 repetitions** with random interleaving and warmup: on a shared
machine the fast tail approximates the uncontended cost (contention only ever
adds time), and averaging the best few rejects a single-sample fluke while
staying far more reproducible than the median at sub-nanosecond scale. Bold =
fastest per length. Lengths straddle the dispatch-tier and SSO boundaries (7/8
the fully-unrolled `<= 8` path, 15/16 the `<= 16` path and libstdc++ SSO cap, 22
the libc++ SSO cap, 47/48 and 63/64 the short-chain steps). The tool's full mode
sweeps a denser exponential curve.

### 64-bit one-shot throughput (ns/op, mean of the 3 fastest of 9 reps; lower is better)

| Length | mumbo     | rapidhash | xxh3     | xxh64 | murmur3 | siphash24 | fnv1a    | dumbo    |
| -----: | --------- | --------- | -------- | ----- | ------- | --------- | -------- | -------- |
|     1B | 2.01      | 1.95      | 1.86     | 1.94  | 2.80    | 6.60      | **0.51** | 0.82     |
|     3B | 2.34      | 2.01      | 1.89     | 2.58  | 3.01    | 6.75      | 1.05     | **0.94** |
|     7B | 2.26      | 1.78      | 1.71     | 2.84  | 3.10    | 6.84      | 2.74     | **1.12** |
|     8B | 2.13      | 1.73      | 1.64     | 2.28  | 3.11    | 9.22      | 3.29     | **1.40** |
|    11B | 2.05      | 1.75      | 1.64     | 3.24  | 3.64    | 9.34      | 4.56     | **1.51** |
|    15B | 1.99      | 1.73      | **1.62** | 3.73  | 3.67    | 9.35      | 6.22     | 1.95     |
|    16B | 1.97      | 1.74      | **1.71** | 2.97  | 4.01    | 12.15     | 6.54     | 2.25     |
|    19B | 2.82      | **1.98**  | 2.41     | 3.96  | 4.57    | 12.35     | 7.65     | 2.49     |
|    22B | 2.79      | **1.98**  | 2.41     | 3.90  | 4.51    | 12.32     | 9.35     | 2.92     |
|    27B | 2.83      | **2.02**  | 2.40     | 4.75  | 5.27    | 15.54     | 11.79    | 3.50     |
|    32B | 2.85      | **2.00**  | 2.47     | 5.12  | 5.62    | 18.58     | 15.35    | 4.28     |
|    47B | 2.94      | **2.43**  | 3.86     | 8.63  | 6.59    | 22.04     | 27.69    | 6.49     |
|    48B | 2.90      | **2.47**  | 3.84     | 6.83  | 7.13    | 25.48     | 28.49    | 6.72     |
|    63B | 3.34      | **2.94**  | 3.90     | 10.42 | 8.30    | 28.99     | 40.53    | 9.38     |
|    64B | 3.45      | **2.91**  | 3.86     | 6.88  | 8.93    | 32.29     | 41.32    | 9.83     |
|   256B | 8.85      | **7.43**  | 27.86    | 16.27 | 33.95   | 122.1     | 290.2    | 68.68    |
|    1Ki | 23.83     | **22.74** | 59.35    | 55.78 | 165.9   | 481.9     | 1344     | 388.8    |
|    4Ki | **83.70** | 84.99     | 179.3    | 230.8 | 707.9   | 1929      | 5576     | 1733     |

### 128-bit one-shot throughput (ns/op, mean of the 3 fastest of 9 reps; native-128 algorithms only)

| Length | jumbo     | xxh3     | murmur3 |
| -----: | --------- | -------- | ------- |
|     1B | 3.12      | **2.34** | 2.78    |
|     8B | 3.01      | **2.23** | 3.04    |
|    16B | 3.04      | **2.74** | 4.03    |
|    22B | 3.94      | **3.62** | 4.58    |
|    32B | 3.96      | **3.61** | 5.64    |
|    47B | **4.60**  | 5.36     | 6.63    |
|    64B | 6.26      | **5.11** | 8.96    |
|   256B | **10.61** | 29.73    | 33.92   |
|    1Ki | **32.74** | 60.90    | 166.7   |
|    4Ki | **124.5** | 180.3    | 710.6   |

### Mixed-length latency (ns/hash, mean of the 3 fastest of 9 reps; lower is better)

Each hash result selects the next key, serializing the dependency chain and
defeating the size-dispatch branch predictor - the cost profile a hash table
actually pays (as opposed to the hot, size-predictable throughput loop above).

| max len | mumbo | rapidhash | xxh3  | xxh64     | murmur3 | siphash24 | fnv1a | dumbo    |
| ------: | ----- | --------- | ----- | --------- | ------- | --------- | ----- | -------- |
|      16 | 9.75  | 10.15     | 11.07 | 13.78     | 15.26   | 18.80     | 13.06 | **9.40** |
|      64 | 11.87 | 11.67     | 12.50 | **10.03** | 19.46   | 27.85     | 11.59 | 23.54    |
|    1024 | 27.39 | **27.06** | 38.95 | 67.08     | 68.53   | 233.5     | 610.3 | 302.0    |

Reading the results: `rapidhash` leads small keys, but after the if-ladder load
path (see the design iterations) `mumbo` sits ~1.97-2.0 ns through 16 bytes -
within ~0.2-0.3 ns of rapidhash across the inline-`std::string` range (1.97 ns
at 16 B, 1.99 ns at the 15 B libstdc++ SSO cap). The remaining gap is the
17-64 B short-chain tier (mumbo ~2.8-3.4 ns vs rapidhash ~1.98-2.9), the
sequential 16-byte MUM chain; mumbo retakes the 4 KiB bulk (83.7 ns) and is the
fastest strong algorithm in the latency chain at 16 B and the leader at 1 KiB,
so the throughput deficit does not carry into the dependency-bound case. That
small-key gap is mumbo's deliberate price: the two-multiply finalizer that earns
the clean 188/188 in BOTH widths. For 128-bit, `xxh3` leads to 32 bytes but
`jumbo` pulls decisively ahead from 47 bytes up (1.5-2.9x beyond 256 B) and is
the only SMHasher3-clean native 128 on the rig. `fnv1a` and `dumbo` win the
1-8 byte corner (minimal setup) but collapse past small keys, and `siphash`
pays its PRF security throughout.

### Performance across platforms

The CI benchmark job measures every push on two architectures (mean of 3
repetitions; values `ubuntu-latest` x86_64 gcc / `macos-26` arm64 Apple
clang, ns/op, from the PR #235 run). Shared runners are noisy - these numbers
are for architecture/compiler _shape_ comparisons, not absolutes; entries
marked `*` are gcc constant-folding artifacts on fixed-size lanes.

Mixed-length latency:

| max len | mumbo       | rapidhash       | xxh3         | xxh64       | murmur3     | siphash   | fnv1a       | dumbo       |
| ------: | ----------- | --------------- | ------------ | ----------- | ----------- | --------- | ----------- | ----------- |
|      16 | 7.5 / 9.8   | 8.1 / 10.9      | 7.7 / 12.0   | 12.6 / 14.1 | 12.4 / 16.1 | 15 / 22   | 12.5 / 13.2 | 20.4 / 9.8  |
|      64 | 9.3 / 11.2  | 10.1 / 11.4     | 3.6* / 12.6  | 25.4 / 21.4 | 15.7 / 20.1 | 22 / 35   | 0.9* / 38.1 | 69.9 / 26.7 |
|    1024 | 41.4 / 29.2 | **18.9 / 27.4** | 111.5 / 44.2 | 120 / 74.6  | 105 / 78.4  | 216 / 282 | 581 / 570   | 837 / 374   |

64-bit one-shot:

|  size | mumbo           | rapidhash       | xxh3          | xxh64       |
| ----: | --------------- | --------------- | ------------- | ----------- |
|   16B | 3.1 / 3.0       | 3.4 / **2.3**   | **1.7** / 2.6 | 5.4 / 3.5   |
|  256B | 23.2 / 12.5     | **13.6 / 9.5**  | 121.8 / 40.4  | 33.6 / 19.5 |
| 4 KiB | 275 / **103.0** | **183** / 102.6 | 749 / 210     | 445 / 308   |

128-bit one-shot:

|  size | jumbo           | xxh3          | murmur3     |
| ----: | --------------- | ------------- | ----------- |
|   16B | 9.3 / 4.4       | **3.2 / 3.7** | 9.0 / 8.9   |
|  256B | **19.6 / 13.3** | 128.5 / 43.8  | 47.9 / 53.6 |
| 4 KiB | **184 / 127**   | 683 / 214     | 721 / 863   |

Cross-platform reading: the mumbo/rapidhash near-tie holds on both
architectures (rapidhash leads x86_64-gcc bulk; they tie on arm64), `jumbo`
is the fastest 128-bit hash from 256 bytes up on both platforms, and the
xxh3 mid-size dip plus the fnv1a/siphash/dumbo profiles reproduce
everywhere.

## Quality: SMHasher3

[SMHasher3](https://gitlab.com/fwojcik/smhasher3) is the research-grade hash
test battery; passing it is the community bar for a production-quality
general-purpose hash. All results below are **our own measurements on one
rig** (same build, container, flags, and machine - see Methodology), so the
numbers are directly comparable.

### Results

| Algorithm   | Bits | Role in mbo/hash          | SMHasher3 result | Failures                                                                                                                                                                      |
| ----------- | ---: | ------------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `fnv1a`     |   64 | `hash.h`                  | FAIL - 7 / 186   | nearly every family: Avalanche, BIC, Sparse, Cyclic, Permutation, Text, TwoBytes, Bitflip, PerlinNoise, and the complete Seed* cluster                                        |
| `mumbo`     |   64 | default (64/32/streaming) | PASS - 188 / 188 | none                                                                                                                                                                          |
| `rapidhash` |   64 | extra (`hash_extra_cc`)   | PASS - 188 / 188 | none                                                                                                                                                                          |
| `siphash`   |   64 | `hash.h` (keyed PRF)      | PASS - 186 / 186 | none                                                                                                                                                                          |
| `xxh3`      |   64 | extra (`hash_extra_cc`)   | FAIL - 166 / 188 | BIC [3, 8, 11], Sparse [20/3], PerlinNoise [2], Bitflip [8], SeedZeroes [1280, 8448], SeedSparse [2, 3]                                                                       |
| `xxh64`     |   64 | extra (`hash_extra_cc`)   | FAIL - 181 / 188 | SeedBlockLen [15, 19, 21, 26, 29, 30], SeedBIC [8]                                                                                                                            |
| `jumbo`     |  128 | default (128)             | PASS - 188 / 188 | none                                                                                                                                                                          |
| `murmur3`   |  128 | `hash.h`                  | FAIL - 123 / 188 | BIC, Zeroes, Permutation, and the complete Seed* cluster (11 families)                                                                                                        |
| `xxh3`      |  128 | extra (`hash_extra_cc`)   | FAIL - 162 / 188 | BIC [3, 8, 15], Sparse [20/3], PerlinNoise [2], Bitflip [3, 4, 8], SeedZeroes [1280, 8448], SeedSparse [2, 3], SeedBlockLen [8, 12-16], SeedBlockOffset [0-5], SeedBIC [3, 8] |

Reading the results:

- SMHasher3 is substantially stricter than the original SMHasher: `xxh64` and
  `xxh3` pass the original battery, and most of their failures above are in
  the newer `Seed*` families (weak seed handling), which the original battery
  does not probe.
- `mumbo` and `rapidhash` are the only clean passes; `jumbo` (the
  mumbo family's native 128, "mumbo jumbo") is the only clean 128-bit result
  we have measured on this rig - every other tested
  128-bit variant fails more of the battery than its 64-bit sibling, because
  the wider output gives the statistics more surface to catch bias and lane
  correlation on.
- Of the classics: `siphash` (a keyed PRF) is clean, as security designs must
  be; `murmur3` (2011) fails the modern battery broadly; and `fnv1a` - the
  algorithm family behind many `std::hash` implementations - passes 7 of 186
  tests. Numbers worth remembering when defaulting to `std::hash`.
- The mumbo/jumbo family is the default in all forms; the extras remain
  available for
  canonical-value interop via `hash_extra.h` (`//mbo/hash:hash_extra_cc`,
  which carries the third-party NOTICE obligations - see the repository-root
  NOTICE).

### mumbo: the measured design iterations

Lineage first: the library's original hash was `dumbo` (originally named
`simple`; the legacy pre-0.13 `GetHash`, still shipped as `mbo::hash::dumbo`
for comparison only). Its
intended replacement `mh` (never released) accumulated hardening rounds -
sparse-key collision fixes, seed hardening - but its SMHasher3 failure list
never fully cleared, and rapidhash held the default in the interim. The
lessons from that failure analysis fed a clean-sheet widening-multiply (MUM)
redesign under the working name `mh2`, which is where the measured iterations
below begin (v1-v3). On reaching 188/188 in both widths it was renamed
`mumbo` (MUM + mbo), `mh` was dropped entirely, and mumbo/jumbo took the
defaults; v4 landed with that rename.

`mumbo` reached the clean pass in four measured iterations (each step:
benchmark plus both SMHasher3 batteries):

1. v1 (175/188): MUM core with unrolled small-key loads. All failures in
   sparse/short-key families - the finalizer folded the widening product
   early and then multiplied against a constant, leaking quasi-linear deltas.
2. v2 (177/188): the finalizer keeps BOTH product halves and mixes them
   against each other. Remaining failures: for <= 8-byte keys the data sat in
   only one product operand, so permuted keys correlate pairwise through the
   shared seed operand.
3. v3 (188/188 both widths): data loads into both product operands (products
   quadratic in the data), the length folded into the seed, distinct
   bulk-chain initializers.
4. v4 (re-verified PASS 188/188 in both widths): the length moved from the
   seed into
   the finalizer's product operands - equally protective, but known only at
   finalize, which is what makes streaming possible; the 128-bit lane seeds
   derive from secret pairs distinct from the 64-bit chain, so no lane ever
   equals the 64-bit hash. The table above reflects the latest completed
   batteries.

### Methodology (reproduction)

- SMHasher3 @ gitlab.com/fwojcik/smhasher3, commit `6ab4343` (2026-03-26),
  built with gcc 13 in a linux/arm64 container, `-march=armv8-a+crc` (two build fixes
  needed: a missing `<cstdlib>` include in `lib/AEStest.cpp`, and replacing
  `-march=native` in CMakeLists.txt, which emits SHA3 `eor3` instructions the
  container toolchain rejects).
- `rapidhash`, `XXH3-64`, `XXH-64`, `XXH3-128`, `FNV-1a-64`,
  `MurmurHash3-128`, and `SipHash-2-4` are SMHasher3's built-in
  registrations of the same reference algorithms our headers transcribe
  (transcriptions are vector- and differential-verified equal, so the results
  transfer; the built-in `XXH3-128` ran its NEON implementation on this rig,
  which produces the identical canonical values).
- `mumbo` was transcribed standalone into a plugin source (registered as
  `mumbo-64` and `jumbo-128`, seeded), matching `hash_mumbo.h` at the current
  commit.
- Full default battery per hash: `./SMHasher3 <name>` (~12 minutes each).
  Full logs are not committed; regenerate as above.
