# mbo/hash roadmap

Gap analysis vs state of the art (SMHasher3, abseil, xxhash/wyhash ecosystems),
2026-07-03. Ordered by leverage within each tier. Remove items when done.

## Easy

## Medium

- [ ] **jumbo (128-bit) streaming**: the `HasStreaming` concept finalizes to
      `uint64_t`; extending streaming to 128-bit output needs a concept
      extension (`StreamFinalize128`?) - design when a consumer materializes.

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

- (RESOLVED - implemented differently) A build option excluding the
  notice-bearing transcriptions was rejected, and the once-rejected
  `hash_extra` split became viable when the in-house mumbo/jumbo family took
  the defaults: `//mbo/hash:hash_cc` is now notice-free and
  `//mbo/hash:hash_extra_cc` carries rapidhash/xxh64/xxh3 (with the NOTICE
  obligations) as an explicit opt-in.

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
