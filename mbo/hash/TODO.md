# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

- [ ] **Seed-avalanche test**: flip *seed* bits in the typed avalanche test
      (SMHasher-style); soften/skip for seedless algorithms (`simple`).
- [ ] **Sparse/structured-key tests**: distinctness over all-zero inputs of
      varying lengths, single-bit keys, and cyclic patterns - catches weak
      mixing that random-string collision tests hide.
- [ ] **Big-endian caveat**: document that BE is untested (both load paths
      byte-assemble there; claimed-correct but no CI runner exercises it).

## Medium

- [ ] **Mixed-length latency benchmark**: random length per iteration; the
      current per-length hot loops are branch-predictor-friendly and hide
      dispatch costs that table workloads pay.
- [ ] **SipHash** (`siphash::Algorithm`, SipHash-2-4 and/or -1-3): keyed,
      hash-flooding resistant - the security story we lack. ARX-only, so
      cleanly constexpr; pin against reference vectors.
- [ ] **wyhash/rapidhash-class algorithm**: small-key latency king. The
      constexpr 64x64->128 multiply now exists (`hash_internal::Mul128Fold64`,
      added for XXH3), so this is unblocked.
- [ ] **Differential fuzz target**: constexpr-vs-runtime path equality and
      canonical algorithms vs stored vectors (`rules_fuzzing`; new dep + CI).
- [ ] **Run SMHasher3 against `mh`** once, record results (external tool, not
      CI): the credibility bar for a novel construction.

## Hard / needs design

- [ ] **Streaming/incremental API** (`init/update/final` state per algorithm):
      xxhash/abseil parity for chunked input. Only with a concrete use case.

## Non-goals (decided)

- SIMD kernels (XXH3-vector, HighwayHash class): incompatible with the
  constexpr single-path design; a dual-path SIMD kernel would duplicate whole
  algorithms for maintenance cost we do not want.
- Typed/general-purpose hashing (`HashOf(values...)`): `absl::Hash` owns that
  space in this codebase; our niche is constexpr string hashing.
- True 128-bit strength from 64-bit-only algorithms: the `Hasher` fallback is
  documented best-effort (see hash.h).
