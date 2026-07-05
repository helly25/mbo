# mbo/hash - fast, constexpr-safe, non-cryptographic hashing

Spec-based, fast, constexpr-compatible, Apache-licensed, no-nonsense hash
implementations. Algorithm reference and API listing: see the
[repository README](../../README.md); design decisions and roadmap:
[TODO.md](TODO.md). Quality (SMHasher3) and performance measurements for all
algorithms: below.

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
| `mumbo`     | 64     | `hash.h` (default 64/32/mangle)   | none (in-house)         | yes    | yes       | PASS           |
| `jumbo`     | 128    | `hash.h` (default 128)            | none (in-house)         | yes    | yes (64)  | PASS           |
| `murmur3`   | 64/128 | `hash.h`                          | none (public domain)    | yes    | no        | (measuring)    |
| `siphash`   | 64     | `hash.h`                          | none (CC0)              | keyed  | yes       | (measuring)    |
| `fnv1a`     | 64     | `hash.h`                          | none (public domain)    | yes    | no        | (measuring)    |
| `simple`    | 64     | `hash.h`                          | none (in-house)         | no     | no        | n/a (legacy)   |
| `rapidhash` | 64     | `hash_extra.h` + `:hash_extra_cc` | **MIT - ship NOTICE**   | yes    | no        | PASS           |
| `xxh64`     | 64     | `hash_extra.h` + `:hash_extra_cc` | **BSD-2 - ship NOTICE** | yes    | yes       | FAIL (181)     |
| `xxh3`      | 64/128 | `hash_extra.h` + `:hash_extra_cc` | **BSD-2 - ship NOTICE** | yes    | no        | FAIL (166/162) |

Notes: `fnv1a` is the algorithm family many `std::hash` implementations use
(e.g. MSVC) - included as the familiar baseline. `siphash` is a keyed PRF:
the DoS-resistant choice when the seed is a secret. `simple` is the legacy
pre-0.13 hash, kept for comparison only. Linking `:hash_extra_cc` requires
shipping the repository-root [NOTICE](../../NOTICE) (see "Third-party
components" in the [repository README](../../README.md)).

## Performance

Measured with `bazel run -c opt //mbo/hash:hash_benchmark` (Apple Silicon
arm64, clang; same build and machine for every row - absolute numbers vary by
hardware, the _relative_ picture is the point). Bold = fastest per size.
`BmHash64Latency` chains unpredictable key sizes through a serialized
dependency (what hash-table workloads feel); the throughput tables are
one-shot hashes of fixed sizes.

### Mixed-length latency (ns/hash, lower is better):

| max len | mumbo | rapidhash | xxh3 | xxh64    | murmur3 | siphash | fnv1a | simple   |
| ------: | ----- | --------- | ---- | -------- | ------- | ------- | ----- | -------- |
|      16 | 10.3  | 10.2      | 11.7 | 13.7     | 15.8    | 18.8    | 13.1  | **10.1** |
|      64 | 11.9  | 12.2      | 12.3 | **10.3** | 20.1    | 28.7    | 12.3  | 23.3     |
|    1024 | 27.2  | **26.7**  | 37.8 | 69.1     | 72.4    | 239     | 610   | 312      |

### 64-bit one-shot throughput (ns/op; last column GiB/s at 4 KiB):

| Algorithm | 1B       | 16B      | 64B      | 256B     | 1KiB     | 4KiB     | 4KiB GiB/s |
| --------- | -------- | -------- | -------- | -------- | -------- | -------- | ---------- |
| mumbo     | 2.37     | 2.42     | 3.90     | 9.32     | 23.9     | **83.0** | **46.1**   |
| rapidhash | 2.43     | 2.06     | **3.37** | **7.97** | **22.8** | 84.4     | 45.3       |
| xxh3      | 1.91     | **1.75** | 5.86     | 33.7     | 67.3     | 217      | 17.7       |
| xxh64     | 1.92     | 3.00     | 6.93     | 17.7     | 59.1     | 313      | 12.3       |
| murmur3   | 2.84     | 4.20     | 9.06     | 34.8     | 167      | 712      | 5.4        |
| siphash   | 6.69     | 12.2     | 32.3     | 122      | 499      | 1924     | 2.0        |
| fnv1a     | **0.52** | 7.07     | 41.3     | 298      | 1361     | 5864     | 0.65       |
| simple    | 0.92     | 2.50     | 10.5     | 69.9     | 457      | 1736     | 2.2        |

### 128-bit one-shot throughput (ns/op; native-128 algorithms only):

| Algorithm | 1B       | 16B      | 64B      | 256B     | 1KiB     | 4KiB    |
| --------- | -------- | -------- | -------- | -------- | -------- | ------- |
| jumbo     | 4.04     | **3.59** | 6.55     | **13.8** | **31.6** | **124** |
| xxh3      | **2.45** | 3.69     | **5.20** | 33.5     | 67.4     | 212     |
| murmur3   | 3.15     | 3.97     | 9.71     | 33.8     | 168      | 733     |

Reading the results: `mumbo` and `rapidhash` trade blows within a few percent
across the 64-bit board - rapidhash by a hair at 64-1024 bytes, mumbo leads
bulk (4 KiB) - while `jumbo` (the family's native 128) is the fastest 128-bit
hash from 16 bytes up
(1.7-2.4x xxh3-128 beyond 256 bytes); xxh3-128 keeps the tiny-key lead. Of
the classic algorithms, `fnv1a` wins the 1-byte corner (one multiply per
byte, no setup) but is 10-70x slower beyond small keys, and `siphash` pays
its PRF security in speed; neither is latency-competitive even at 16 bytes.

## Quality: SMHasher3

[SMHasher3](https://gitlab.com/fwojcik/smhasher3) is the research-grade hash
test battery; passing it is the community bar for a production-quality
general-purpose hash. All results below are **our own measurements on one
rig** (same build, container, flags, and machine - see Methodology), so the
numbers are directly comparable.

### Results

| Algorithm   | Bits | Role in mbo/hash                 | SMHasher3 result | Failures                                                                                                                                                                      |
| ----------- | ---: | -------------------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `mumbo`     |   64 | default (64/32/mangle/streaming) | PASS - 188 / 188 | none                                                                                                                                                                          |
| `rapidhash` |   64 | extra (`hash_extra_cc`)          | PASS - 188 / 188 | none                                                                                                                                                                          |
| `xxh3`      |   64 | extra (`hash_extra_cc`)          | FAIL - 166 / 188 | BIC [3, 8, 11], Sparse [20/3], PerlinNoise [2], Bitflip [8], SeedZeroes [1280, 8448], SeedSparse [2, 3]                                                                       |
| `xxh64`     |   64 | extra (`hash_extra_cc`)          | FAIL - 181 / 188 | SeedBlockLen [15, 19, 21, 26, 29, 30], SeedBIC [8]                                                                                                                            |
| `jumbo`     |  128 | default (128)                    | PASS - 188 / 188 | none                                                                                                                                                                          |
| `xxh3`      |  128 | extra (`hash_extra_cc`)          | FAIL - 162 / 188 | BIC [3, 8, 15], Sparse [20/3], PerlinNoise [2], Bitflip [3, 4, 8], SeedZeroes [1280, 8448], SeedSparse [2, 3], SeedBlockLen [8, 12-16], SeedBlockOffset [0-5], SeedBIC [3, 8] |

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
- The mumbo/jumbo family is the default in all forms; the extras remain
  available for
  canonical-value interop via `hash_extra.h` (`//mbo/hash:hash_extra_cc`,
  which carries the third-party NOTICE obligations - see the repository-root
  NOTICE).

### mumbo: the measured design iterations

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
4. v4 (64-bit re-verified PASS 188/188; the 128-bit battery result is
   recorded in the table above): the length moved from the seed into
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
- `rapidhash`, `XXH3-64`, `XXH-64`, and `XXH3-128` are SMHasher3's built-in
  registrations of the same reference algorithms our headers transcribe
  (transcriptions are vector- and differential-verified equal, so the results
  transfer; the built-in `XXH3-128` ran its NEON implementation on this rig,
  which produces the identical canonical values).
- `mumbo` was transcribed standalone into a plugin source (registered as
  `mumbo-64` and `jumbo-128`, seeded), matching `hash_mumbo.h` at the current
  commit.
- Full default battery per hash: `./SMHasher3 <name>` (~12 minutes each).
  Full logs are not committed; regenerate as above.
