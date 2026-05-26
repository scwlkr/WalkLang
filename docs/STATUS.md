# WalkLang Status

Current WalkLang version: `v5.13.1`.

`v5.13.1` is the single project version for the compiler, tooling, docs,
release artifacts, and implemented language surface. Feature maturity is
described with status labels instead of separate version lines.

Current feature status:

```text
stable
  core syntax, diagnostics, modules, tests, project tooling, and standard-library helpers

draft
  do:, defer:, io, parse, process, file, dir, path, json, term, http, and html

experimental
  structs, methods, and simple generic functions
```

Current release state: `v5.13.1` aligns current-facing docs, CI, examples,
compatibility fixtures, generated docs routes, visible test names, and
developer scripts around one project version. Draft `defer:` cleanup,
recoverable-result policy docs, package collection roots, and first-class build
modes remain implemented and covered by focused tests.

Last full release verification on 2026-05-26:

```text
go test -count=1 ./... passed
scripts/check-docs-site.sh passed after staging generated docs
git diff --cached --check passed
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk' passed
current-facing version/path sweep passed outside release notes, migration history, deprecated historical docs, and generated search history
go build -trimpath -ldflags "-X main.version=v5.13.1" -o build/walk ./cmd/walk passed
./build/walk version reported v5.13.1
./build/walk check --warnings=error tests/pass/walk_tests.walk passed
compatibility stress script passed
scripts/build-docs-site.sh passed
scripts/release.sh v5.13.1 <temp>/release produced 5 platform artifacts plus SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 5 artifacts
the Darwin arm64 release artifact reported v5.13.1
scripts/install-local.sh v5.13.1 refreshed the local walk install
walk version reported v5.13.1
```

Known local state: two Hangman playground files have pre-existing unstaged
edits and are not part of the version-alignment cleanup.

Version-language cleanup on 2026-05-26: current-facing docs now use `v5.13.1`
as the single project version. Historical version numbers remain only in
release notes and migration history. `CONTEXT.md` records the resolved terms
**Project Version**, **Feature Status**, and **Historical Version Reference**.

Version-language cleanup verification on 2026-05-26:

```text
go test ./cmd/walk -run 'TestStableCompatibilitySuite|TestCurrentReleaseDocsArePresent' -count=1 passed
go test ./scripts -run TestSidebarUsesTopicNavigationInsteadOfVersionMilestones -count=1 passed
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
go test -count=1 ./... passed
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
current-facing source and generated-page search found no old language-version wording outside release notes and migration history
```

Developer-facing path cleanup on 2026-05-26: active examples, compatibility
fixtures, the compatibility stress script, current docs source pages, generated
docs routes, and visible test names now use purpose-based names instead of old
milestone labels. Release notes and migration history kept their historical
version references unchanged.

Developer-facing path cleanup verification on 2026-05-26:

```text
go test ./cmd/walk ./internal/format ./internal/checker ./scripts -run 'TestStableCompatibilitySuite|TestCurrentReleaseDocsArePresent|TestExamplesAreTestableFixtures|TestInExpression|TestFormatter|TestShadowingProducesWarning|TestSidebarUsesTopicNavigationInsteadOfVersionMilestones' -count=1 passed
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/check-docs-site.sh passed
go test -count=1 ./... passed
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk' passed
git diff --cached --check passed
active-code search found no old developer-facing milestone path or test-name labels outside release notes, migration history, and deprecated docs
```

Next: keep new docs, examples, scripts, fixtures, and CI references aligned to
the single Project Version whenever the next release slice starts.
