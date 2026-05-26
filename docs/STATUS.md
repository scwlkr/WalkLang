# WalkLang Status

Current WalkLang version: `v5.14.0`.

`v5.14.0` is the single project version for the compiler, tooling, docs,
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

standard platform
  CLI standard-platform slice with `walktop` as the first official standalone
  WalkLang-built tool
```

Current release state: `v5.14.0` ships `walktop` under `tools/walktop/` as a
real WalkLang project and standalone native command. `walktop` supports
`--once`, `--frames 5`, and `--fixture tools/walktop/testdata/basic`, uses OS
commands for live data, uses deterministic fixture mode for tests, renders a
compact color-forward dashboard through `term` APIs, and is installed by the
normal local install flow beside `walk`.

Last full release verification on 2026-05-26:

```text
go test -count=1 ./... passed
go build -trimpath -ldflags "-X main.version=v5.14.0" -o build/walk ./cmd/walk passed
./build/walk version reported v5.14.0
./build/walk check --warnings=error tests/pass/walk_tests.walk passed
cd tools/walktop && ../../build/walk check --warnings=error passed
cd tools/walktop && ../../build/walk test --warnings=error passed with 4 tests
./build/walk build --mode release --warnings=error tools/walktop/src/main.walk -o build/walktop passed
NO_COLOR=1 ./build/walktop --once --fixture tools/walktop/testdata/basic matched the deterministic dashboard
NO_COLOR=1 ./build/walktop --frames 5 --fixture tools/walktop/testdata/basic produced 40 dashboard lines
NO_COLOR=1 ./build/walktop --once passed live OS-command mode on the local Darwin host
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed
scripts/release.sh v5.14.0 <temp>/release produced 5 walk artifacts, 1 current-host walktop artifact, and SHA256SUMS
wc -l <temp>/release/SHA256SUMS reported 6
shasum -a 256 -c SHA256SUMS passed for all 6 artifacts
the Darwin arm64 walk release artifact reported v5.14.0
the Darwin arm64 walktop release artifact passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v5.14.0 refreshed the local walk and walktop install
walk version reported v5.14.0
command -v walktop reported ~/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic matched the deterministic dashboard
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk' passed
git diff --cached --check passed
```

Known local state: two Hangman playground files have pre-existing unstaged
edits and are not part of the version-alignment cleanup.

Standard platform implementation on 2026-05-26: WalkLang's intended product
shape is recorded as one general-use language with one standard platform in one
monorepo. `CONTEXT.md` records the standard-platform vocabulary, `docs/adr/`
records the monorepo decision, and `docs/STANDARD_PLATFORM.md` records the
CLI-first implementation path. The first standard platform area is CLI. The
first official standalone WalkLang-built tool is `walktop`, a release-quality
system monitor installed by the normal local WalkLang install flow.

Focused standard platform verification on 2026-05-26:

```text
go test ./cmd/walk -run TestWalktop -count=1 passed
../../build/walk test --warnings=error in tools/walktop passed 4 WalkLang tests
NO_COLOR=1 ./build/walktop --once --fixture tools/walktop/testdata/basic passed
NO_COLOR=1 ./build/walktop --once passed live local OS-command mode
```

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

Next: continue standard-platform expansion after the CLI proof app, with future
areas gated by the same proof parity rules.
