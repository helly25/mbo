# Project TODO

This file is the single source of truth for repository fixes and improvements.
Each change is implemented and reviewed in its own pull request. An item is
checked in the pull request that completes it, so merging that pull request
also updates this file to the completed state.

## Correctness

- [x] Fix undefined behavior in `mbo/json/json.h` iterator assignment.
  - Replace placement construction over live iterator objects with ordinary
    variant assignment and default same-type special members where possible.
  - Cover copy, move, mutable-to-const, self, array, and object assignments.
  - PR: [#326](https://github.com/helly25/mbo/pull/326).
- [x] Reject truncated glob ranges without accessing an empty pattern.
  - Cover `[`, `[-`, `[a-`, negative ranges, and trailing escapes.
  - PR: [#327](https://github.com/helly25/mbo/pull/327).
- [x] Fix main-branch cache cleanup prefix matching.
  - Pass the prefix to `jq` without relying on expansion inside single quotes.
  - Add a regression test with representative cache keys.
  - PR: [#328](https://github.com/helly25/mbo/pull/328).

## File API robustness and portability

- [x] Make `GetContents` handle failed seeks and non-seekable inputs safely.
  - Check `seekg`/`tellg` before converting the size.
  - Read in binary mode so byte counts remain consistent across platforms.
  - Add failure-path tests where practical.
  - PR: [#329](https://github.com/helly25/mbo/pull/329).
- [x] Make `GetMaxLines` distinguish EOF from an I/O failure.
  - Return an error when the underlying read fails.
  - Add regression coverage.
  - PR: [#330](https://github.com/helly25/mbo/pull/330).
- [x] Make `NormalizePath` portable across native path character types.
  - Avoid constructing `std::string_view` directly from `path.c_str()`.
  - Add or document Windows-oriented behavior and tests.
  - PR: [#331](https://github.com/helly25/mbo/pull/331).

## Build, CI, and release engineering

- [x] Reduce external dependency warning noise in default builds.
  - Treat external headers consistently as system headers or apply narrowly
    scoped external-warning suppression.
  - Verify project warnings remain errors.
  - PR: [#332](https://github.com/helly25/mbo/pull/332).
- [x] Document the repository's GitHub Actions versioning policy.
  - Prefer readable version references and require a documented reason for any
    commit-SHA pin.
  - PR: [#333](https://github.com/helly25/mbo/pull/333).
- [x] Replace `mktemp -u` in release preparation with a safely created
      temporary resource and cleanup trap.
  - Exercise the release archive preparation path locally.
  - PR: [#334](https://github.com/helly25/mbo/pull/334).

## Documentation and quality coverage

- [x] Correct existing README and contributing-guide spelling errors and add a
      lightweight documentation spell-checking hook.
  - PR: [#335](https://github.com/helly25/mbo/pull/335).
- [ ] Add fuzz targets for glob conversion, string parsing, INI parsing, diff
      inputs/options, and digest checksum-file parsing.
  - [x] Glob conversion. PR: [#336](https://github.com/helly25/mbo/pull/336).
  - [x] String parsing. PR: [#337](https://github.com/helly25/mbo/pull/337).
  - [x] INI parsing. PR: [#338](https://github.com/helly25/mbo/pull/338).
  - [ ] Diff inputs/options. PR: pending.
  - [ ] Digest checksum-file parsing. PR: pending.
- [ ] Add explicit UBSan coverage alongside ASan in CI.
  - PR: pending.
- [ ] Add coverage reporting focused on uninstantiated templates and malformed
      input branches.
  - PR: pending.
- [ ] Add public-header API compatibility checking before releases.
  - PR: pending.
