# Agent and contributor rules - mbo

These rules apply to every contributor, human or automated. The repository is human-led: optimize
changes and explanations for human maintainers, and use automation to enforce mechanical rules.
[`STYLE_CPP.md`](STYLE_CPP.md) and [`STYLE_SH.md`](STYLE_SH.md) are canonical for their languages;
[`CONTRIBUTING.md`](CONTRIBUTING.md) describes the contribution flow.

Build and test with `bazel test //...`. Sanitizer configurations and the supported Bazel/compiler
matrix are exercised by CI. Run `pre-commit run --all-files` before proposing repository-wide
mechanical changes, or `pre-commit run --files FILE...` for a focused change.

## Pull request descriptions

Every pull request description has two layers, in this order:

1. a short, human-readable explanation of the outcome and why it matters;
2. a literal `## AG;DR` heading followed by the implementation details, reasoning, validation,
   portability notes, and known limitations needed by reviewers and future maintainers.

Update the description whenever a commit is added so the detailed section describes the complete
current change. Keep the human-readable section stable unless the outcome or motivation changes.

## Tests

- Use a `struct` fixture and `TEST_F`; do not add bare `TEST` cases.
- Use `EXPECT_THAT` / `ASSERT_THAT` and matchers. GoogleTest comparison macros are forbidden.
- Use `mbo::testing::EqualsText` for multi-line text and the mbo status matchers/macros for
  `absl::Status` and `absl::StatusOr`.
- Name typed and parameterized cases from their values or types; do not leave numbered suites.

See [`STYLE_CPP.md`](STYLE_CPP.md) for the complete rules and examples.

## Documentation

Keep GitHub-flavored Markdown tables vertically aligned: each `|` in a column lines up in the raw
file and delimiter alignment markers are preserved. Run
[`tools/align_markdown_tables.py`](tools/align_markdown_tables.py); pre-commit runs it for Markdown.

User-visible behavior, options, output, and build requirements are documented and tested in the
same change. Generated documentation must continue to derive from its existing source of truth.
