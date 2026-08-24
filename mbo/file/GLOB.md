# mbo glob syntax and shared implementation plan

This document defines the required outcome of PR
[#371](https://github.com/helly25/mbo/pull/371). The complete outcome is delivered in that single PR
so xff can adopt it in one follow-up dependency-update PR without retaining a parallel glob parser.

## Goal

Provide one fast, deterministic, locale-independent glob implementation built around the features
RE2 can express efficiently. The syntax takes useful POSIX pattern-matching behavior, deliberately
adds path globstar and SHGLOB alternatives, and deliberately omits locale and general subexpression
negation features that RE2 cannot represent faithfully.

Matching behavior, generated RE2, documentation, benchmarks, fuzzing, and the demonstrated xff
adoption path are part of the same change.

## Semantic contract

The glob syntax is independent of the process locale. Ranges and named classes have the stable
definitions documented by RE2. Locale-sensitive collation, collating symbols, equivalence classes,
and locale-sensitive case folding are unsupported.

`/` is always the path separator on every platform. Component-local operators never consume it.
Repeated separators are normalized consistently. The historical exact `[/]` spelling remains a
documented mbo compatibility spelling for a separator; no other bracket expression consumes `/`.

| Feature                      | Required behavior                                               |
| ---------------------------- | --------------------------------------------------------------- |
| Literal                      | Match itself, escaped for RE2 as necessary                      |
| Backslash                    | Escape the following glob character                             |
| `?`                          | Match one non-`/` character                                     |
| `*`                          | Match zero or more non-`/` characters                           |
| Complete-component `**`      | Match zero or more complete path components                     |
| Embedded star run            | Behave as one component-local `*`                               |
| Positive bracket expression  | Match one listed non-`/` character                              |
| Negated bracket expression   | Match one non-`/` character not listed                          |
| Named class                  | Use the documented RE2 ASCII class with `/` removed             |
| GLOB braces                  | Remain literal                                                  |
| SHGLOB braces                | Provide nested alternatives, including empty alternatives       |
| General negative extglob     | Unsupported because RE2 has no general subexpression complement |
| Lookahead and lookbehind     | Unsupported                                                     |
| Collating/equivalence syntax | Unsupported                                                     |

Negated bracket expressions are supported. They are not general negative lookahead: `[!a]` becomes
the equivalent of `[^/a]`, while a construct such as `!(foo|bar)` is outside this RE2-native syntax.

## Globstar contract

A run of exactly two or more unescaped stars crosses directories only when it occupies a complete
path component. In other positions it reduces to an ordinary component-local `*`.

`foo/**/bar` matches all of:

```text
foo/bar
foo/a/bar
foo/a/b/bar
foo/a/b/c/bar
```

The implementation and tests must define all boundary forms explicitly:

```text
**
**/bar
foo/**/bar
foo/**
foo**bar
***
foo/***/bar
```

The selected behavior for trailing `foo/**` must be identical in mbo and xff and documented as to
whether the base path itself is included. Escaped stars and repeated separators require explicit
tests.

## Bracket-expression implementation

Parse a bracket expression into a structural representation before emitting RE2. Do not allocate a
temporary `std::string` for each token and do not erase previously emitted output to repair a range.

For the ASCII portion, a compact 256-bit membership set permits cheap literal, range, named-class,
union, subtraction, and inversion operations. Remove `/` after forming the positive set and before
applying component-local negation semantics. Emit normalized contiguous RE2 ranges monotonically.

The implementation must correctly handle:

- leading `]`;
- leading and trailing literal `-`;
- escaped members;
- multiple ranges;
- slash at either range endpoint;
- ranges crossing slash;
- mixed literals, ranges, and named classes;
- classes that become empty after slash removal;
- ascending, descending, malformed, and unterminated ranges;
- named classes that otherwise include slash, including `ascii`, `graph`, `print`, and `punct`.

Unsupported or malformed syntax returns a precise status. It must never be silently passed through to
RE2 with changed meaning.

## Public API and xff adoption

The pure translator must be public rather than confined to `file_internal`. It must support the two
consumer modes without requiring another parser:

```cpp
enum class GlobSyntax {
  kGlob,
  kShGlob,
};

struct Glob2Re2Options {
  GlobSyntax syntax = GlobSyntax::kGlob;
  bool allow_globstar = true;
  RE2::Options re2_options;
};
```

The exact API may follow repository naming conventions, but it must let xff map its `GLOB` and
`SHGLOB` grammars directly. xff may keep gitignore-specific preprocessing for negation, anchoring,
and directory-only rules. It must not need to retain wildcard, bracket, escape, globstar, or brace
translation.

Before PR 371 is complete, validate a local xff dependency override against the PR branch. A local
xff adaptation must remove or bypass its parallel translator, use mbo for both GLOB and SHGLOB, and
pass the complete xff test suite. Record the tested xff commit and commands in the PR description.

## Performance requirements

Parsing is single-pass apart from bounded nested SHGLOB alternatives. Reserve output once, append
monotonically, and avoid allocation per input character or bracket member. Compile a pattern once for
repeated matching and preserve RE2's linear-time matching behavior.

Add benchmarks covering:

- literals and ordinary component wildcards;
- bracket-heavy patterns;
- globstar at leading, middle, and trailing positions;
- nested SHGLOB alternatives;
- translation and RE2 compilation;
- repeated matching.

Compare with the previous mbo implementation, xff's current translator, and `fnmatch()` under the C
locale where the syntax overlaps. Material regressions require explanation or correction.

## Validation requirements

Use table-driven conformance tests containing the input pattern, generated RE2, matching paths, and
non-matching paths. Port or reproduce xff's complete GLOB and SHGLOB semantic suite in mbo.

For the overlapping portable subset, compare results with `fnmatch(..., FNM_PATHNAME)` under the C
locale. Test intentional extensions and differences separately, including exact `[/]`, globstar, and
SHGLOB braces.

Fuzzing must cover parser termination, malformed input, generated RE2 validity, slash exclusion,
option combinations, nesting limits, and equivalence between the specified glob behavior and emitted
RE2.

PR 371 is complete only when mbo's full test suite, pre-commit, benchmarks, fuzz regressions, CI, and
the local xff adoption test pass.
