# Workflow maintenance

GitHub Actions and reusable workflows should normally use readable version
references, such as `@v6` or `@v3.0.1`. Do not replace these references with
commit SHAs solely as a blanket hardening measure.

A commit-SHA pin is appropriate only when a specific reason requires an exact
revision. Document that reason next to the reference so maintainers can judge
whether the pin is still necessary when updating the workflow.

## Coverage publishing

The coverage cell always uploads its LCOV data, policy summary, and browsable HTML as the
`coverage-report` run artifact. Successful reports with a stable review identity are also served by
GitHub Pages:

- `https://helly25.github.io/mbo/coverage/main/` is replaced by each successful `main` run;
- `https://helly25.github.io/mbo/coverage/pr<NUMBER>/` is replaced by each successful run associated
  with that pull request.

[`coverage_pages.yml`](coverage_pages.yml) is a separate `workflow_run` publisher so branch code
never receives `contents: write`, `pages: write`, or an OIDC token. mbo tests branch pushes rather
than `pull_request` events, and GitHub can also omit the pull-request array when a run is retried, so
the trusted publisher resolves every non-main commit through GitHub's commit-to-PR API. A branch
without an associated pull request remains artifact-only.

The complete retained site lives on the generated `coverage-pages` branch; each serialized
publisher replaces only its selected directory before deploying the whole site. That branch is the
sole exclusion from the repository's verified-signature rule because GitHub Actions creates its
generated commits. Source, review, and release branches remain covered by the rule. Do not broaden
the exclusion or give the test workflow write access.
