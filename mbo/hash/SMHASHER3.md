# SMHasher3 results for mbo/hash algorithms

[SMHasher3](https://gitlab.com/fwojcik/smhasher3) is the research-grade hash
test battery; passing it is the community bar for a production-quality
general-purpose hash. All results below are **our own measurements on one
rig** (same build, container, flags, and machine - see Methodology), so the
numbers are directly comparable.

## Results

| Algorithm   | Bits | Role in mbo/hash              | SMHasher3 result | Failures                                                                                                                                                                      |
| ----------- | ---: | ----------------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `rapidhash` |   64 | default (64/mangle)           | PASS - 188 / 188 | none                                                                                                                                                                          |
| `mh`        |   64 | in-house, WIP                 | FAIL - 183 / 188 | BIC [3, 11, 15], Sparse [9/4], Bitflip [4]                                                                                                                                    |
| `xxh64`     |   64 | canonical algorithm           | FAIL - 181 / 188 | SeedBlockLen [15, 19, 21, 26, 29, 30], SeedBIC [8]                                                                                                                            |
| `xxh3`      |   64 | 64-bit sibling of the default | FAIL - 166 / 188 | BIC [3, 8, 11], Sparse [20/3], PerlinNoise [2], Bitflip [8], SeedZeroes [1280, 8448], SeedSparse [2, 3]                                                                       |
| `mh`        |  128 | in-house, WIP                 | FAIL - 163 / 188 | BIC [3, 11, 15], Zeroes, Sparse [9/4], Permutation [4-byte keys, 10 variants], TwoBytes [20], Bitflip [4], SeedZeroes [1280, 8448], SeedBlockOffset [0-5]                     |
| `xxh3`      |  128 | default (128)                 | FAIL - 162 / 188 | BIC [3, 8, 15], Sparse [20/3], PerlinNoise [2], Bitflip [3, 4, 8], SeedZeroes [1280, 8448], SeedSparse [2, 3], SeedBlockLen [8, 12-16], SeedBlockOffset [0-5], SeedBIC [3, 8] |

Reading the results:

- SMHasher3 is substantially stricter than the original SMHasher: `xxh64` and
  `xxh3` pass the original battery, and most of their failures above are in
  the newer `Seed*` families (weak seed handling), which the original battery
  does not probe.
- `rapidhash` is the only clean pass, which - together with winning the
  mixed-length latency benchmark - is why it is the 64-bit and mangled
  `GetHash` default.
- Both 128-bit variants fail more of the battery than their 64-bit siblings:
  the wider output gives the statistics more surface to catch bias and lane
  correlation on. No tested 128-bit variant passes cleanly.
- `xxh3` (128) remains the 128-bit default regardless: it is the library's
  only native 128-bit algorithm with canonical values (the de-facto
  interchange format), its failures are again dominated by the `Seed*`
  families, and no cleaner 128-bit alternative exists in-library.
- `mh` places above the xxHash family and below `rapidhash` on this battery;
  its remaining failures are analyzed below.

## mh: remaining failure analysis

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

## Methodology (reproduction)

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
  `mbo-mh64` and `mbo-mh128`, seeded), matching `hash_mh.h` at the current
  commit.
- Full default battery per hash: `./SMHasher3 <name>` (~12 minutes each).
  Full logs are not committed; regenerate as above.
