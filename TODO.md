# Project TODO

This file is the single source of truth for repository fixes and improvements.
Each change is implemented and reviewed in its own pull request. An item is
checked in the pull request that completes it, so merging that pull request
also updates this file to the completed state.

## Correctness

- [x] Make `Required<T>` replacement and generic operations correct.
  - Construct replacement values before destroying the current value so a
    throwing user constructor leaves the wrapper unchanged.
  - Require non-throwing relocation for replacement without assignment or
    nullable storage.
  - Fix hashing and heterogeneous wrapper comparison when instantiated.
  - Derive comparison exception specifications from the wrapped operation.
- [x] Make `OptionalDataOrRef` lifetime and exception handling safe.
  - Preserve its null, owned-data, and borrowed-reference states with automatic
    lifetime management.
  - Handle self-move and return `*this` from null assignment.
  - Use operation-dependent exception specifications and leave a valid empty
    state after failed replacement construction.
  - Cover all state transitions, aliasing, and throwing construction.
  - Document mbo's exception-free builds and exception-enabled public-header
    compatibility contract.
  - PR: [#359](https://github.com/helly25/mbo/pull/359).
- [x] Make single- and double-quote parsing options independent.
  - Honor each quote-enable option without coupling it to the other.
  - Treat disabled quote characters as ordinary unquoted input.
  - Cover all four option combinations with separator-sensitive assertions.
  - PR: [#349](https://github.com/helly25/mbo/pull/349).
- [x] Correct hexadecimal escape parsing for alphabetic digits.
  - Map lowercase and uppercase `A`-`F` to values 10-15.
  - Cover mixed-case, C++23 braced, byte-boundary, and overflow inputs.
  - PR: [#347](https://github.com/helly25/mbo/pull/347).
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

- [x] Avoid an unnecessary Bazel configuration switch while generating the
      clang-tidy compilation database.
  - Build the extractor with the same `clang-tidy` configuration used by its
    internal analysis query and the preceding CI build.
  - Retain the extractor's intentional feature-only transition, which disables
    parameter files, layering checks, and header-parsing actions for `aquery`.
  - PR: [#354](https://github.com/helly25/mbo/pull/354).
- [x] Update the compile-command extractor to deduplicate exec actions before source probing.
  - Avoid misleading missing-generated-source warnings on cold-cache clang-tidy runs.
  - Preserve exec-only generated sources while preferring equivalent target-configuration actions.
  - PR: [#352](https://github.com/helly25/mbo/pull/352).
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

- [x] Unify coverage ratings, enforcement, and presentation, adopting the
      applicable final state of
      [xff PR #639](https://github.com/helly25/xff/pull/639),
      [xff PR #641](https://github.com/helly25/xff/pull/641), and
      [xff PR #642](https://github.com/helly25/xff/pull/642).
  - Derive the low, medium, and high presentation bands and the independently
    selected enforced rating from one validated policy model.
  - Preserve justified per-category exceptions and expose every effective
    policy, reason, and changed-line policy in the published JSON.
  - Apply metric-specific colours to the detailed LCOV report and render its
    global policy matrix below the report details.
  - Render the compact report as a dense, colour-backed measurement and policy
    matrix with explicit `GOOD`, `OK`, and `BAD` states and JSON links.
  - PR: [#361](https://github.com/helly25/mbo/pull/361).
- [x] Make `coverage_policy.json` the single source of truth for enforcement
      and LCOV presentation, adopting the applicable final state of
      [xff PR #631](https://github.com/helly25/xff/pull/631),
      [xff PR #633](https://github.com/helly25/xff/pull/633), and the
      coverage-related changes from
      [xff PR #634](https://github.com/helly25/xff/pull/634) and
      [xff PR #636](https://github.com/helly25/xff/pull/636).
  - Define explicit per-metric enforcement minima and health targets, and
    generate `genhtml`'s medium and high limits from them. Equal minimum and
    target values must form one pass/fail boundary without a yellow band.
  - Pin a verified modern LCOV release that honors metric-specific thresholds,
    accepting only the documented GCC/LLVM producer inconsistencies.
  - Replace the generic `genhtml` legend with a policy-derived, metric-specific
    legend below the summary table. Add depth-independent navigation from every
    LCOV page to the report overview and retained-report index. Keep the compact
    report enforcement-only: values at or above the minimum are `OK`, and values
    below it fail.
  - Use the generated configuration throughout the reusable main, PR, and
    release coverage path, with validation and unit tests for generation, HTML
    augmentation, status rendering, and every metric.
  - Keep category floors and health targets inherited and composable so an
    exception names only the metrics it changes.
  - PR: [#360](https://github.com/helly25/mbo/pull/360).
- [x] Exercise the hash-internal runtime utilities directly ([PR #356](https://github.com/helly25/mbo/pull/356)).
  - Cover every supported length for `LoadTail` and `LoadSmall` with
    volatile-derived runtime inputs.
  - Verify both lanes and the folded result of the 128-bit multiplication helpers.
- [x] Adopt the shared helly25 contributor and agent guidance, C++ and shell
      styles, and machine-enforceable style checks.
  - Adapt the final xff policy set through PR #612 to mbo's human-led workflow.
  - Keep project-specific conventions and omit xff-specific CLI rules.
  - PR: [#350](https://github.com/helly25/mbo/pull/350).
- [x] Publish durable, indexed coverage reports for main, pull requests, and releases.
  - Group the detailed LCOV tree by the same module categories as the policy report.
  - Retain source/run metadata and prevent stale workflow completions from replacing newer reports.
  - Provide a browsable index with report source, completion time, commit, workflow run, and metrics.
  - PR: [#351](https://github.com/helly25/mbo/pull/351).
- [x] Correct existing README and contributing-guide spelling errors and add a
      lightweight documentation spell-checking hook.
  - PR: [#335](https://github.com/helly25/mbo/pull/335).
- [x] Add fuzz targets for glob conversion, string parsing, INI parsing, diff
      inputs/options, and digest checksum-file parsing.
  - [x] Glob conversion. PR: [#336](https://github.com/helly25/mbo/pull/336).
  - [x] String parsing. PR: [#337](https://github.com/helly25/mbo/pull/337).
  - [x] INI parsing. PR: [#338](https://github.com/helly25/mbo/pull/338).
  - [x] Diff inputs/options. PR: [#339](https://github.com/helly25/mbo/pull/339).
  - [x] Digest checksum-file parsing. PR: [#340](https://github.com/helly25/mbo/pull/340).
- [x] Add explicit UBSan coverage alongside ASan in CI.
  - PR: [#341](https://github.com/helly25/mbo/pull/341).
- [x] Add coverage reporting focused on uninstantiated templates and malformed
      input branches.
  - PR: [#343](https://github.com/helly25/mbo/pull/343).
- [x] Make `mbo/testing` meet the general function-coverage minimum.
  - Count the behaviorally covered `CapacityIs` description bodies once rather
    than once for every compiler-generated container specialization.
  - Remove the category-specific function-coverage override.
  - PR: [#353](https://github.com/helly25/mbo/pull/353).
- [x] Make `mbo/types` meet the general branch-coverage minimum.
  - Exclude compiler-expanded branch pairs for the behaviorally covered
    `tstring` search fold expressions.
  - Remove the category-specific branch-coverage override.
  - PR: [#355](https://github.com/helly25/mbo/pull/355).
- [x] Make `mbo/hash` meet the general branch-coverage minimum.
  - Directly test the hash-internal runtime utilities before excluding GCC's
    duplicated records for compile-time and non-native-endian alternatives.
  - Remove the category-specific branch-coverage override.
  - PR: [#357](https://github.com/helly25/mbo/pull/357).
- [x] Make `mbo/log` meet the general function-coverage minimum.
  - Count behaviorally identical template and generic-lambda specializations
    once per source definition.
  - Exclude only quick-exit functions whose process cannot flush coverage data.
  - Remove the category-specific function-coverage override.
  - PR: [#358](https://github.com/helly25/mbo/pull/358).
