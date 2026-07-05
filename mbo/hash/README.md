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
  and differential tests against the reference libraries. The in-house `mh`
  algorithm is documented with its measured strengths and weaknesses.
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

## Performance

Measured with `bazel run -c opt //mbo/hash:hash_benchmark` (Apple Silicon
arm64, clang; same build and machine for every row - absolute numbers vary by
hardware, the _relative_ picture is the point). Bold = fastest per size.
`BmHash64Latency` chains unpredictable key sizes through a serialized
dependency (what hash-table workloads feel); the throughput tables are
one-shot hashes of fixed sizes.

### Mixed-length latency (ns/hash, lower is better):

| max len | rapidhash | mh2      | xxh3 | mh   | xxh64    |
| ------: | --------- | -------- | ---- | ---- | -------- |
|      16 | 10.2      | **10.1** | 11.2 | 12.7 | 13.7     |
|      64 | 12.2      | 13.6     | 13.0 | 23.9 | **10.3** |
|    1024 | **28.0**  | 29.4     | 54.5 | 44.1 | 69.1     |

### 64-bit one-shot throughput (ns/op; last column GiB/s at 4 KiB):

| Algorithm | 1B       | 16B      | 64B      | 256B     | 1KiB     | 4KiB     | 4KiB GiB/s |
| --------- | -------- | -------- | -------- | -------- | -------- | -------- | ---------- |
| rapidhash | 2.16     | 1.85     | **3.10** | **7.97** | **24.5** | 109      | 35.5       |
| mh2       | 2.52     | 2.93     | 3.95     | 9.22     | 25.8     | **90.8** | **42.1**   |
| xxh3      | 1.91     | 1.75     | 5.86     | 29.2     | 65.8     | 216      | 17.7       |
| mh        | **1.42** | **1.73** | 8.15     | 17.4     | 57.7     | 232      | 16.5       |
| xxh64     | 1.92     | 3.00     | 6.93     | 17.7     | 59.1     | 313      | 12.3       |

### 128-bit one-shot throughput (ns/op; native-128 algorithms only):

| Algorithm | 1B       | 16B      | 64B      | 256B     | 1KiB     | 4KiB    |
| --------- | -------- | -------- | -------- | -------- | -------- | ------- |
| xxh3-128  | **2.45** | **3.69** | **5.20** | 30.4     | 64.8     | 193     |
| mh2-128   | 4.13     | 4.13     | 9.04     | **11.9** | **35.2** | **138** |
| mh-128    | 3.36     | 3.95     | 6.37     | 15.5     | 58.3     | 252     |

Reading the results: `rapidhash` and `mh2` split the 64-bit crown - rapidhash
leads the 64-1024-byte throughput sizes by a hair, mh2 leads bulk (4 KiB) and
the <= 16-byte latency class (the hash-table hot path). `mh2`-128 is the
fastest 128-bit hash in the library from 256 bytes up (1.4-2.5x xxh3-128);
xxh3-128 keeps the small-key 128 lead. `mh` only leads tiny-key one-shot
throughput.

## Quality: SMHasher3

[SMHasher3](https://gitlab.com/fwojcik/smhasher3) is the research-grade hash
test battery; passing it is the community bar for a production-quality
general-purpose hash. All results below are **our own measurements on one
rig** (same build, container, flags, and machine - see Methodology), so the
numbers are directly comparable.

### Results

| Algorithm   | Bits | Role in mbo/hash              | SMHasher3 result | Failures                                                                                                                                                                      |
| ----------- | ---: | ----------------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `mh`        |   64 | in-house, WIP                 | FAIL - 183 / 188 | BIC [3, 11, 15], Sparse [9/4], Bitflip [4]                                                                                                                                    |
| `mh2`       |   64 | in-house, EXPERIMENTAL        | PASS - 188 / 188 | none                                                                                                                                                                          |
| `rapidhash` |   64 | default (64/mangle)           | PASS - 188 / 188 | none                                                                                                                                                                          |
| `xxh3`      |   64 | 64-bit sibling of the default | FAIL - 166 / 188 | BIC [3, 8, 11], Sparse [20/3], PerlinNoise [2], Bitflip [8], SeedZeroes [1280, 8448], SeedSparse [2, 3]                                                                       |
| `xxh64`     |   64 | canonical algorithm           | FAIL - 181 / 188 | SeedBlockLen [15, 19, 21, 26, 29, 30], SeedBIC [8]                                                                                                                            |
| `mh`        |  128 | in-house, WIP                 | FAIL - 163 / 188 | BIC [3, 11, 15], Zeroes, Sparse [9/4], Permutation [4-byte keys, 10 variants], TwoBytes [20], Bitflip [4], SeedZeroes [1280, 8448], SeedBlockOffset [0-5]                     |
| `mh2`       |  128 | in-house, EXPERIMENTAL        | PASS - 188 / 188 | none                                                                                                                                                                          |
| `xxh3`      |  128 | default (128)                 | FAIL - 162 / 188 | BIC [3, 8, 15], Sparse [20/3], PerlinNoise [2], Bitflip [3, 4, 8], SeedZeroes [1280, 8448], SeedSparse [2, 3], SeedBlockLen [8, 12-16], SeedBlockOffset [0-5], SeedBIC [3, 8] |

Reading the results:

- SMHasher3 is substantially stricter than the original SMHasher: `xxh64` and
  `xxh3` pass the original battery, and most of their failures above are in
  the newer `Seed*` families (weak seed handling), which the original battery
  does not probe.
- `rapidhash` and `mh2` are the only clean passes; rapidhash predates mh2 and
  is (currently) the 64-bit and mangled `GetHash` default.
- `mh2` (128) is the only clean 128-bit pass we have measured on this rig -
  every other tested 128-bit variant (xxh3-128, mh-128) fails more of the
  battery than its 64-bit sibling, because the wider output gives the
  statistics more surface to catch bias and lane correlation on.
- `xxh3` (128) is (currently) the 128-bit default: canonical values, the
  de-facto interchange format. Whether `mh2` takes over the defaults is an
  open decision (see TODO.md).
- `mh` places above the xxHash family and below `rapidhash` on this battery;
  its remaining failures are analyzed below.
- `mh2` is the in-house widening-multiply ("MUM") redesign; it reached the
  clean pass in three measured iterations: v1 (175/188; all failures in
  sparse/short-key families - the finalizer folded the widening product
  early and multiplied against a constant, leaking quasi-linear deltas), v2
  (177/188; finalizer keeps both product halves), v3 (188/188 both widths;
  small keys load the data into BOTH product operands so every product is
  quadratic in the data, the length is folded into the seed so it modulates
  every product multiplicatively, and the bulk chains start from distinct
  secrets).

### mh: remaining failure analysis

`mh`'s 64-bit failures form one cluster - core-round diffusion. One
rotate-multiply per lane per absorb does not reach full bit independence
(BIC), which sparse keys and single-bit flips expose. Fixing this means
strengthening the absorb round at a throughput cost; that work (or
replacing/renaming the algorithm) is tracked in [TODO.md](TODO.md) under the
"mh is WIP" item. An earlier, larger failure set in the seed-handling
families was eliminated by finalizing the seed through `Fmix64` before lane
derivation - `mh`'s seed now avalanches properly.

The native 128-bit variant additionally exposes **lane correlation**: both
lanes absorb the identical premultiplied block and differ only in the
absorb operation (xor vs add), rotation, and multiplier, and the second
lane's initial state is just `seed ^ constant`. Short-key and seed
differences therefore stay correlated across the two output halves, which
the 128-bit-only failure families (Zeroes, Permutation, TwoBytes,
SeedZeroes, SeedBlockOffset) detect. The 64-bit fold cross-mixes the lanes
and hides exactly this - which is why `mh` (64) does not show them. Lane
decorrelation is part of the same WIP item.

`mh` remains fine for its documented purpose: a fast, constexpr-safe,
non-adversarial table hash with no value-stability guarantee.

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
- `mh` was transcribed standalone into `hashes/mbo_mh.cpp` (registered as
  `mbo-mh64`/`mbo-mh128` and `mbo-mh2-64`/`mbo-mh2-128`, seeded), matching
  `hash_mh.h` / `hash_mh2.h` at the current commit.
- Full default battery per hash: `./SMHasher3 <name>` (~12 minutes each).
  Full logs are not committed; regenerate as above.
