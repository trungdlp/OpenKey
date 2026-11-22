# Merge policy and CI

GitHub auto-merge is enabled for this repository. Enable it on an individual PR with **Enable auto-merge**, or `gh pr merge <number> --repo trungdlp/OpenKey --auto --squash`. GitHub waits until the merge requirements are satisfied. Repository setup does not automatically enroll every PR in auto-merge.

`master` requires a pull request, resolved review conversations, and an up-to-date branch with these passing GitHub Actions checks:

| Required check | Protects |
| --- | --- |
| `regressions (ubuntu-latest)` | Engine and native sender regressions with ASan/UBSan |
| `regressions (macos-latest)` | The same regressions with macOS sanitizers |
| `regressions (windows-latest)` | Regressions and native update installer tests |
| `coverage (macos-latest)` | >80% production-line coverage overall and per measured file |
| `coverage (windows-latest)` | The same coverage gate, including the update installer |
| `macos-build` | Native macOS Release app compilation |
| `windows-build` | Native Windows Release compilation for x86 and x64 |

The protection also applies to administrators, requires linear history, and blocks force pushes and branch deletion. No approving review is mandatory, so the repository owner can merge their own PR after CI passes. Squash merge uses the PR title; use Conventional Commits titles. Auto-merge still waits for queued checks and may require **Update branch** when `master` advances.

Workflows run once per PR update and on pushes to `master`. New commits cancel the previous run for the same PR. Each push to `master` has a separate concurrency group. Jobs have explicit time limits. Windows artifact attestation runs after a push to `master`; it is not a required PR check and does not request write permissions on fork PRs.

Required check names are repository settings, not inferred from these YAML files. Keep the branch-protection check names in sync when renaming jobs or matrix entries. See [the test documentation](../tests/README.md) for coverage scope and local checks.
