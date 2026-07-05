# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

## Medium

- [ ] **mh2 end-game decisions**: `mh2` (widening-multiply redesign) passes
      SMHasher3 188/188 in both widths - the only clean native 128 we have
      measured - and leads the mixed-length latency and bulk-throughput
      benchmarks (see README.md). Open decisions: (a) keep or drop the
      original `mh` (it loses everywhere except tiny-key one-shot
      throughput and has streaming, which mh2 does not yet); (b) whether
      `mh2` takes over the 64-bit and/or 128-bit defaults from
      rapidhash/xxh3 (which would make the defaults NOTICE-free and reopen
      the hash_extra split); (c) final name; (d) streaming for mh2.

## Hard / needs design

(empty)

## Future

- Streaming for `xxh3` and `murmur3` (reference streaming semantics exist;
  deferred), and possibly a canonical-values streaming benchmark.

## Done (separate library)

- **mbo/digest**: implemented per the charter in
  [mbo/digest/README.md](../digest/README.md) - SHA-2 (224/256/384/512,
  512/224, 512/256), SHA-3, SHAKE128/256, BLAKE2b (incl. native keying),
  BLAKE3 (XOF/keyed/derive-key), HMAC, SHA-1 + MD5 interop, and the `digest`
  CLI binary. Remaining ideas (unscheduled): cSHAKE/KMAC, HKDF.

## Non-goals (decided)

- A build option excluding the notice-bearing transcriptions (rapidhash MIT,
  xxHash BSD-2): the obligations are satisfied by shipping NOTICE; a flag
  would flip the default algorithm per build configuration (same footgun
  class rejected for auto-MBO_HASH_MANGLE) and double the CI matrix. The
  granular path already exists - the algorithm headers are self-contained, so
  notice-free consumers can include only the attribution-free set (mh,
  simple, fnv1a, murmur3, siphash); per-algorithm cc_library targets can be
  added compatibly if such a consumer ever materializes. A
  `//mbo/hash:hash_extra_cc` split was also considered and rejected: the
  compliance-relevant algorithms (rapidhash, xxh3) are the constexpr defaults
  baked into hash.h - no registration mechanism can inject a default at link
  time under constant evaluation - so a split could only move the courtesy
  entries, which carry no obligation to begin with.

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
