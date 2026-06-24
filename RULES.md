# Rules

Some rules for the code layout and its development.

> **C++ coding style** (formatting, naming, namespaces, headers, macros, idioms, and
> the GoogleTest conventions) lives in [`STYLE_CPP.md`](STYLE_CPP.md). This file keeps
> the project-level rules that are not C++ coding style.

- Everything is under Apache 2 license, see file `LICENSE`.
- All sources must be unix-text files: https://en.wikipedia.org/wiki/Text_file
  - Lines end in {LF}.
  - The files are either empty or end in {LF}.
- API changes that are not backwards compatible should not occur in minor version changes.
- Undocumented and private/internal APIs may be changed in any way at any time.
- All exported library code is in the directory `mbo`.
  - Directory `mbo` has no actual library rules, but may have test rules.
- All public / exported code must:
  - be tested (see [`STYLE_CPP.md`](STYLE_CPP.md) for the GoogleTest conventions),
  - have a documentation.
- All shell files follow https://google.github.io/styleguide/shellguide.html but use shfmt which diverges slightly.
  - The common shebang line is `#!/usr/bin/env bash` followed by the license.
