# mbo/hash - fast, constexpr-safe, non-cryptographic hashing

Spec-based, fast, constexpr-compatible, Apache-licensed, no-nonsense hash
implementations. Algorithm reference and API listing: see the
[repository README](../../README.md); design decisions and roadmap:
[TODO.md](TODO.md); quality evaluation of the in-house algorithm:
[SMHASHER3.md](SMHASHER3.md).

## Principles

- **Canonical or honest**: third-party algorithms (rapidhash, XXH64/XXH3,
  MurmurHash3, SipHash, FNV-1a) are transcriptions producing the exact
  published reference values on every platform, pinned by reference vectors
  and differential tests against the reference libraries. The in-house `mh`
  algorithm is documented with its measured strengths and weaknesses.
- **constexpr-safe single path**: compile-time and run-time evaluation always
  agree; streaming (where provided) equals the one-shot value by contract.
- **Apache-2.0 with clean attribution**: transcription notices live in the
  repository-root NOTICE; LICENSE stays pure Apache-2.0. No crypto-library
  dependencies - digests and hashes are spec-frozen pure functions that we
  verify against official vectors instead of trusting an unverifiable supply
  chain (see mbo/digest/README.md for the full argument).
- **Not cryptographic**: values are neither stable across versions nor safe
  against adversaries (SipHash with a secret seed is the DoS-resistant
  option). For message digests (SHA-2, MD5 interop, ...) see
  [mbo/digest](../digest).
