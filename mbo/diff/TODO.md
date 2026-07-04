# mbo/diff roadmap

Gap analysis vs GNU diffutils and git's xdiff, 2026-07-04. The tooling is
functionally complete for its purpose (golden testing plus human review):
three engines (`myers` default, `naive`, `direct`) x four output formats
(`unified`, `context`, `normal`, `side-by-side`), POSIX exit codes, full
CLI/bzl parity, and a self-auditing engine x format test matrix
(`diff_cli_sh_test.sh`) that fails by name on any untested combination.
Remove items when done.

## Easy

- [ ] **`-q`/`--brief`**: report only whether files differ (the exit code
      contract already exists; this just suppresses the output).
- [ ] **`--label`/`-L`**: override the header file names beyond
      `--file_header_use`.
- [ ] **stdin via `-`**: treat the file name `-` as standard input.
- [ ] **Binary detection**: print `Binary files X and Y differ` instead of
      diffing lines containing NUL bytes (today control bytes pass through).

## Medium

- [ ] **ed-script (`diff -e`) and RCS output formats**: the last two classic
      formats. The per-chunk emitter dispatch in `internal/output.cc` takes
      new formats without structural work; the test matrix forces expected
      outputs per engine.
- [ ] **Hunk sliding (git's indent heuristic)**: cosmetic post-pass moving
      hunk boundaries to visually pleasing positions; changes golden outputs.

## Hard / needs design

- [ ] **Patience / histogram engine variants**: alternative `Algorithm`
      values on top of the token infrastructure; useful where unique-line
      anchoring reads better than minimal scripts.
- [ ] **Wide-glyph / tab aware side-by-side columns**: column arithmetic by
      display width instead of bytes (GNU expands tabs; wcwidth for CJK).

## Non-goals (for the record)

- Recursive directory diff (`-r`): this is a two-file differ.
- GNU `-y` tab-stop padding byte-parity: we implement the well-defined
  `--expand-tabs` layout instead (local BSD and GNU diff differ here anyway).
- CLI flags for `max_diff_chunk_length` (naive-only safety valve) and
  `time_format`: intentionally library-only.
- Intra-line word/char highlighting: delta/difftastic territory.
