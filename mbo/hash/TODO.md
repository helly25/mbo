# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

## Medium

- [ ] **mh is WIP - harden, optimize, replace, or rename**: the remaining
      SMHasher3 core-round failures (BIC / Sparse / Bitflip) require
      strengthening the absorb round, i.e. trading some throughput; the
      alternative is replacing the algorithm outright. Either way the `mh`
      name may change. The seed families were addressed by seed finalization;
      `rapidhash` is the default since then (see SMHASHER3.md).

## Hard / needs design

(empty)

## Future

- Streaming for `xxh3` and `murmur3` (reference streaming semantics exist;
  deferred), and possibly a canonical-values streaming benchmark.

## Future (separate library)

- **mbo/digest**: constexpr, spec-frozen message digests pinned against
  official vectors (SHA-2 256/512, SHA-3, BLAKE2b/3, HMAC; legacy MD5 + SHA-1
  for interop, loudly marked broken/deprecated). Deliberately named _digest_,
  not "crypto": AEAD, signatures, key exchange, RNG, and TLS are permanent
  non-goals (actual cryptography needs maintained implementations and
  side-channel expertise). Shares the streaming API designed for mbo/hash.

## Non-goals (decided)

- A build option excluding the notice-bearing transcriptions (rapidhash MIT,
  xxHash BSD-2): the obligations are satisfied by shipping NOTICE; a flag
  would flip the default algorithm per build configuration (same footgun
  class rejected for auto-MBO_HASH_MANGLE) and double the CI matrix. The
  granular path already exists - the algorithm headers are self-contained, so
  notice-free consumers can include only the attribution-free set (mh,
  simple, fnv1a, murmur3, siphash); per-algorithm cc_library targets can be
  added compatibly if such a consumer ever materializes.

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
