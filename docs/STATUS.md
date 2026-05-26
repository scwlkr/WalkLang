# WalkLang Status

Current WalkLang version: `v5.14.1`.

Current architecture direction on 2026-05-26: `docs/SYSTEMS_COMPILER_PORT_PLAN.md`
is now the accepted execution contract for porting WalkLang from the current
reference compiler to the permanent systems architecture: C++ compiler core, C
runtime and platform layer, C backend, optional assembly leaf routines, and no
final Go or JavaScript implementation footprint. Phase 1, the conformance
oracle, is complete. Phase 2, runtime extraction, is complete. Phase 3, the C++
skeleton, is complete. Phase 4, lexer, parser, and AST construction, is
complete. Phase 5, semantic checking, is complete. The next porting step is
Phase 6, the C backend.

`v5.14.1` is the single project version for the compiler, tooling, docs,
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

Current release state: `v5.14.1` publishes the systems compiler port contract
in `docs/SYSTEMS_COMPILER_PORT_PLAN.md` and keeps `walktop` under
`tools/walktop/` as a real WalkLang project and standalone native command.
`walktop` supports `--once`, `--frames 5`, and
`--fixture tools/walktop/testdata/basic`, uses OS commands for live data, uses
deterministic fixture mode for tests, renders a compact color-forward dashboard
through `term` APIs, and is installed by the normal local install flow beside
`walk`. Native builds now link generated C with the external Walk runtime source
under `runtime/`; release artifacts include a runtime source archive, and source
install copies that runtime beside the installed compiler. Source install and
release artifact generation now also build the current-host `walk-cpp`
candidate, which is not yet a replacement for the Go reference compiler.

Systems compiler port state on 2026-05-26: Phase 1 now records the current
reference compiler as an executable conformance oracle under
`tests/conformance/`. The manifest covers 20 pass fixtures, 52 fail fixtures, 4
compatibility fixtures, 3 generated-C snapshot fixtures, and 4 `walktop` fixture
groups. The runner records expected behavior with `WALK_REF=... --record`,
verifies it with `--verify`, and can compare a future `WALK_CANDIDATE` against
the same expected artifacts. `scripts/stress-compatibility.sh` now invokes the
conformance verifier before the older compatibility stress checks. Phase 2 now
extracts the generated helper layer into `runtime/walk_runtime.h`,
`runtime/walk_runtime.c`, and host platform files under `runtime/platform/`.
Generated C includes `walk_runtime.h`, calls stable `walk_rt_*` helpers, and
keeps `walk emit-c` inspectable through a link comment instead of embedding the
runtime body in every emitted file. Phase 3 adds the permanent C++ compiler
skeleton under `compiler/`, a top-level `Makefile`, command dispatch for the
planned CLI surface, `WALK_VERSION` embedding, source loading support,
deterministic diagnostics, and a C++ unit-test harness. Phase 4 adds the C++
lexer, indentation handling, parser, AST arena, and `walk-cpp check
--parse-only`. Phase 5 adds the C++ semantic checker under `compiler/sema/`,
including deterministic scope lookup, type rules, builtin signatures, module
loading, export checks, warnings, and exact fail diagnostic parity for normal
`walk-cpp check`. The C++ candidate builds as `build/walk-cpp`; `version`,
`help`, `check --parse-only`, and normal source-file `check` are implemented so
far, and unported command surfaces still return a `not ported in this phase`
diagnostic without delegating to the Go reference compiler.

Phase 1 conformance oracle verification on 2026-05-26:

```text
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk-ref ./cmd/walk passed
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --record passed
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --verify passed
WALK_BIN=$PWD/build/walk-ref scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
go test -count=1 ./... passed
conformance summary: 20 pass, 52 fail, 25 native executions, 4 compat, 3 snapshot, and 4 walktop fixture groups
```

Phase 2 runtime extraction verification on 2026-05-26:

```text
go test -count=1 ./... passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk ./cmd/walk passed
./build/walk version reported v5.14.1
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
scripts/release.sh v5.14-runtime <temp>/release produced 5 walk artifacts, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
wc -l <temp>/release/SHA256SUMS reported 7
shasum -a 256 -c SHA256SUMS passed for all 7 artifacts
the Darwin arm64 walk release artifact reported v5.14-runtime
the Darwin arm64 walktop release artifact passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v5.14.1 refreshed the local walk, runtime source, and walktop install
installed walk built examples/hello.walk from a temp directory using ~/.local/lib/walk/runtime
```

Phase 3 C++ skeleton verification on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v5.14-cpp-skeleton passed
./build/walk-cpp version reported v5.14-cpp-skeleton
./build/walk-cpp help listed the Phase 3 command surface
make test passed and reported C++ skeleton tests passed
go test -count=1 ./... passed
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed
scripts/release.sh v5.14-cpp-skeleton <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp skeleton artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
WALK_INSTALL_DIR=<temp>/bin scripts/install-local.sh v5.14-cpp-skeleton passed and installed temp walk, walk-cpp, runtime source, and walktop without touching the normal local install
git diff --check passed
language-accounting impact checked: active C++ source is now present under compiler/ and tests/cpp/ while Go remains as the reference compiler until the final removal phase
```

Phase 4 lexer/parser/AST verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
make walk WALK_VERSION=v5.14.1-phase4-dev passed
./build/walk-cpp check --parse-only tests/pass/hello.walk passed
./build/walk-cpp check --parse-only tests/fail/bad_indent.walk failed with a source-ranged syntax diagnostic
./build/walk-cpp check --parse-only tests/fail/top_break.walk failed with a source-ranged syntax diagnostic
./build/walk-cpp check tests/pass/hello.walk still returned the expected not-ported diagnostic without --parse-only
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --parse passed and reported 20 pass fixtures plus 2 syntax fail fixtures
go test -count=1 ./... passed
scripts/release.sh v5.14-cpp-parser <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp parser artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-parser and passed check --parse-only tests/pass/hello.walk
the current-host walktop release artifact passed --once --fixture tools/walktop/testdata/basic
WALK_INSTALL_DIR=<temp>/bin scripts/install-local.sh v5.14-cpp-parser passed and installed temp walk, walk-cpp, runtime source, and walktop without touching the normal local install
the temp installed walk-cpp passed check --parse-only tests/pass/hello.walk
language-accounting impact checked: Phase 4 adds active C++ lexer, parser, AST, and tests while Go and JavaScript remain until their later removal phases
```

Phase 5 semantic checker verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
make walk WALK_VERSION=v5.14.1-phase5-dev passed
./build/walk-cpp check --warnings=error tests/pass/hello.walk passed and printed ok
./build/walk-cpp check --warnings=error tests/fail/type_mismatch.walk failed with the recorded type diagnostic
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --check passed and reported 20 pass fixtures, 52 fail fixtures, 4 compat fixtures, 2 walktop fixtures, and ok
WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --fail-diagnostics passed and reported 52 fail fixtures ok
go test -count=1 ./... passed
scripts/release.sh v5.14-cpp-sema <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp semantic-checker artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-sema and passed check --warnings=error tests/pass/hello.walk
the current-host walktop release artifact passed --once --fixture tools/walktop/testdata/basic
language-accounting impact checked: Phase 5 adds active C++ semantic-checker source and tests while Go and JavaScript remain until their later removal phases
```

Last release verification on 2026-05-26:

```text
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed
go test -count=1 ./... passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk ./cmd/walk passed
./build/walk version reported v5.14.1
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/release.sh v5.14.1 <temp>/release produced 5 walk artifacts, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
wc -l <temp>/release/SHA256SUMS reported 7
shasum -a 256 -c SHA256SUMS passed for all 7 artifacts
the Darwin arm64 walk release artifact reported v5.14.1
the Darwin arm64 walktop release artifact passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v5.14.1 refreshed the local walk and walktop install
walk version reported v5.14.1
command -v walktop reported ~/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic matched the deterministic dashboard
NO_COLOR=1 walktop --once passed live OS-command mode on the local Darwin host
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk' passed
git diff --cached --check passed
```

Previous full release verification for `v5.14.0` on 2026-05-26:

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

Known local state: no unrelated playground edits are present in the current
Phase 5 semantic-checker worktree.

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

Version-language cleanup baseline on 2026-05-26: current-facing docs moved to
the single project version model during the `v5.13.1` cleanup. The current
project version is `v5.14.1`; historical version numbers remain in release
notes and migration history. `CONTEXT.md` records the resolved terms **Project
Version**, **Feature Status**, and **Historical Version Reference**.

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

Docs alignment cleanup on 2026-05-26: active docs now match the `v5.14.1`
project version, current verification scripts, current `route_ranker` smoke
output, current editor-tooling wording, planned-only matrix API status, and live
hosted-docs HTTPS behavior.

Hosted docs state checked on 2026-05-26:

```text
https://walklang.wlkrlabs.com/docs/ returned HTTP/2 200
http://walklang.wlkrlabs.com/docs/ returned 301 to https://walklang.wlkrlabs.com/docs/
```

Docs alignment verification on 2026-05-26:

```text
walk run playground/route_ranker.walk reported Market Mile / 28
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed
go test -count=1 ./... passed
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk' passed
```

Next: begin Phase 6 in `docs/SYSTEMS_COMPILER_PORT_PLAN.md` by porting typed
IR lowering and deterministic C code generation to C++.
