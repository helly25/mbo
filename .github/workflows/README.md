# Workflow maintenance

GitHub Actions and reusable workflows should normally use readable version
references, such as `@v6` or `@v3.0.1`. Do not replace these references with
commit SHAs solely as a blanket hardening measure.

A commit-SHA pin is appropriate only when a specific reason requires an exact
revision. Document that reason next to the reference so maintainers can judge
whether the pin is still necessary when updating the workflow.
