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

- **mbo/digest**: charter and scope in [mbo/digest/README.md](../digest/README.md)
  (SHA-1/224/256 and wider SHA-2, SHA-3, BLAKE2b/3, HMAC, MD5 for interop;
  permanent non-goals AEAD/signatures/KEX/RNG/TLS). Shares the streaming API
  designed for mbo/hash. Next up.

## Non-goals (decided)

- A build option excluding the notice-bearing transcriptions (rapidhash MIT,
  xxHash BSD-2): the obligations are satisfied by shipping NOTICE; a flag
  would flip the default algorithm per build configuration (same footgun
  class rejected for auto-MBO_HASH_MANGLE) and double the CI matrix. The
  granular path already exists - the algorithm headers are self-contained, so
  notice-free consumers can include only the attribution-free set (mh,
  simple, fnv1a, murmur3, siphash); per-algorithm cc_library targets can be
  added compatibly if such a consumer ever materializes.

- Message digests in _this_ library: MD5/SHA-1/SHA-2 etc. belong to
  [mbo/digest](../digest/README.md) (spec-based, constexpr, vector-pinned,
  Apache-licensed - see its README for scope and for the stance against
  BoringSSL-style dependencies).

- SIMD kernels (XXH3-vector, HighwayHash class): incompatible with the
  constexpr single-path design; a dual-path SIMD kernel would duplicate whole
  algorithms for maintenance cost we do not want.
- Typed/general-purpose hashing (`HashOf(values...)`): `absl::Hash` owns that
  space in this codebase; our niche is constexpr string hashing.
- True 128-bit strength from 64-bit-only algorithms: the `Hasher` fallback is
  documented best-effort (see hash.h).
