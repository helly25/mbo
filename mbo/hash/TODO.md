# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

## Medium

- [ ] **Run SMHasher3 against `mh`** once, record results (external tool, not
      CI): the credibility bar for a novel construction.

- [ ] **XXH3-128** (`XXH3_128bits`): the modern fast file-checksum format;
      natural extension of the existing XXH3-64 machinery, would be the third
      128-bit-native algorithm.

## Hard / needs design

- [ ] **Streaming/incremental API** (`init/update/final` state per algorithm):
      xxhash/abseil parity for chunked input. Only with a concrete use case.

## Non-goals (decided)

- MD5 (and SHA-\*): looks cryptographic, is broken (MD5) or out of scope; for
  legacy digest interop use a maintained crypto library (BoringSSL is in the
  BCR). Our fast file-identity answer is XXH3(-128).

- SIMD kernels (XXH3-vector, HighwayHash class): incompatible with the
  constexpr single-path design; a dual-path SIMD kernel would duplicate whole
  algorithms for maintenance cost we do not want.
- Typed/general-purpose hashing (`HashOf(values...)`): `absl::Hash` owns that
  space in this codebase; our niche is constexpr string hashing.
- True 128-bit strength from 64-bit-only algorithms: the `Hasher` fallback is
  documented best-effort (see hash.h).
