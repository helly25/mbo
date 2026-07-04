# SMHasher3 evaluation of `mbo::hash::mh`

One-off run recorded 2026-07-04 (see TODO.md). SMHasher3 is the research-grade
hash test battery (successor of SMHasher); passing it is the community bar for
a production-quality general-purpose hash.

## Methodology

- SMHasher3 @ gitlab.com/fwojcik/smhasher3, HEAD of 2026-07-04, built with
  gcc 13 in a linux/arm64 container (`-march=armv8-a+crc`; two upstream build
  fixes: a missing `<cstdlib>` include and avoiding `-march=native`, which
  emits SHA3 `eor3` unsupported by the container toolchain).
- `mbo::hash::mh` (as of PR #217, i.e. including the input premultiplication)
  transcribed standalone into `hashes/mbo_mh.cpp`, registered as `mbo-mh64`,
  64-bit output, seeded.
- Full default battery: `./SMHasher3 mbo_mh64` (~712 s).

## Result: FAIL — 147 / 188

```text
Failures:
    BIC                 : [3, 11, 15]
    Sparse              : [9/4]
    PerlinNoise         : [2]
    Bitflip             : [4]
    SeedBlockLen        : [8..31]
    SeedBlockOffset     : [0..5]
    SeedBIC             : [3, 8]
    SeedBitflip         : [3, 4, 8]
```

Sanity, thread-safety, zero-padding, and the majority of the avalanche /
collision batteries pass; the failures cluster in two areas:

1. **Seed handling (the bulk: SeedBlockLen 8..31, SeedBlockOffset, SeedBIC,
   SeedBitflip).** The seed enters the state almost raw (`h1 = seed`,
   `h2 = seed ^ const`, stripe accumulators derived by single ops), so
   structured seed/input combinations correlate. A cheap candidate fix is
   finalizing the seed through `Fmix64` before lane derivation.
2. **Core-round diffusion (BIC, Sparse 9/4, Bitflip, PerlinNoise).** One
   rotate-multiply per lane per absorb does not achieve full bit independence;
   these are the same class of weakness that the structured-key test caught
   (and PR #217 partially fixed via input premultiplication), now measured
   with research-grade sensitivity. Fixing these means strengthening the
   round itself, i.e. giving up part of mh's speed advantage.

## Interpretation

- `mh` is fine for its documented purpose: a fast, constexpr-safe,
  non-adversarial table hash with no stability guarantee.
- It is NOT SMHasher3-clean, unlike `rapidhash` and `xxh3` (both pass the
  full battery upstream, are canonical, and are already in this library --
  and rapidhash additionally wins the mixed-length latency benchmark).
- **Decision (2026-07-04): (b) + (c).** `rapidhash` is now
  `DefaultHashAlgorithm` (with 128-bit-native `xxh3` as `Default128HashAlgorithm`),
  and `mh`'s seed handling was hardened (seed finalized through `Fmix64` before
  lane derivation, targeting the Seed* families). Core-round hardening remains a
  TODO; `mh` stays available as a non-default algorithm.

The full log is not committed (10k+ lines); regenerate with the methodology
above.
