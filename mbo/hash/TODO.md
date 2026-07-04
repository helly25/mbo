# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

## Medium

- [ ] **Run SMHasher3 against `mh`** once, record results (external tool, not
      CI): the credibility bar for a novel construction.

## Hard / needs design

- [ ] **Streaming/incremental API** (`init/update/final` state per algorithm):
      xxhash/abseil parity for chunked input. Only with a concrete use case.

## Future (separate library)

- **mbo/digest**: constexpr, spec-frozen message digests pinned against
  official vectors (SHA-2 256/512, SHA-3, BLAKE2b/3, HMAC; legacy MD5 + SHA-1
  for interop, loudly marked broken/deprecated). Deliberately named _digest_,
  not "crypto": AEAD, signatures, key exchange, RNG, and TLS are permanent
  non-goals (actual cryptography needs maintained implementations and
  side-channel expertise). Shares the streaming API designed for mbo/hash.

## Non-goals (decided)

- MD5 (and SHA-\*) as a library offering: MD5 looks cryptographic and is
  broken; both are out of scope. Our fast file-identity answer is XXH3(-128).
  Should digest interop ever become a real need, prefer an in-repo, constexpr,
  NIST-vector-verified transcription (the pipeline used for xxh3/rapidhash/
  siphash) over a crypto-library dependency: digests are spec-frozen pure
  functions, whereas e.g. BoringSSL is live-at-head, unversioned, and has a
  history of non-reproducible archives - unverifiable supply chain for exactly
  the code that most needs verifying.

- SIMD kernels (XXH3-vector, HighwayHash class): incompatible with the
  constexpr single-path design; a dual-path SIMD kernel would duplicate whole
  algorithms for maintenance cost we do not want.
- Typed/general-purpose hashing (`HashOf(values...)`): `absl::Hash` owns that
  space in this codebase; our niche is constexpr string hashing.
- True 128-bit strength from 64-bit-only algorithms: the `Hasher` fallback is
  documented best-effort (see hash.h).
