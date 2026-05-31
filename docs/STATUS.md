# WalkLang Status

Current WalkLang version: `v6.3.1`.

Current architecture direction on 2026-05-26: `docs/SYSTEMS_COMPILER_PORT_PLAN.md`
is the accepted execution contract for the systems architecture: C++ compiler
core, C runtime and platform layer, C backend, optional assembly leaf routines,
and no final Go or JavaScript implementation footprint. Phases 1 through 12 are
complete. The systems compiler port is complete.

`v6.3.1` keeps the completed C++/C systems compiler release line and the
`v6.3.0` bounded string tooling, then adds TinyChain under
`examples/tinychain/` as a tested blockchain-style showcase project. TinyChain
uses structs, arrays of structs, module exports, string interpolation, project
tests, and a deterministic proof loop while documenting the next language tools
the example wants: real hashing, remainder, struct-array append, multiline
arrays, stable persistence, byte arrays, and command arguments. The repo-local
`walk` binary builds from C++ sources, generated programs link with the Walk C
runtime, the former Go reference implementation and Go module metadata are
removed, JavaScript source files are removed, and the current language surface
is verified against the recorded conformance oracle. Feature maturity is
described with status labels instead of separate version lines.

Current feature status:

```text
stable
  core syntax, diagnostics, modules, tests, project tooling, and standard-library helpers

draft
  do:, defer:, io, parse, process, file, dir, path, json, map, term, http, and html

experimental
  structs, methods, and simple generic functions

standard platform
  CLI standard-platform slice with `walktop` as the first official standalone
  WalkLang-built tool
```

Current release state: `v6.3.1` is the current public release. It proves
`walk` builds from C++/C, the docs site is static HTML/CSS without JavaScript
assets, repo-local install/release packaging no longer depends on Go, and the
PicoNet-driven string/map/numeric tooling plus the TinyChain example project
work through native execution. `walktop` remains under `tools/walktop/` as a
real WalkLang project and standalone native command.

Conformance state on 2026-05-27: `tests/conformance/` preserves the recorded
oracle and current post-port fixture coverage.
The manifest covers 20 pass fixtures, 60 fail fixtures, 4 compatibility
fixtures, 12 runtime-module fixtures, 3 generated-C snapshot fixtures, and 4
`walktop` fixture groups. `make conformance` now builds the active C++/C
`build/walk` binary and verifies it against those recorded artifacts.

Health check on 2026-05-31: `v6.3.0` remains the current public release and the
repo-local compiler health is intact. `make test WALK_VERSION=dev-status-check`
passed, `make conformance WALK_VERSION=dev-status-check` passed with 20 pass
fixtures, 60 fail fixtures, 37 native executions, 4 compatibility fixtures, 12
runtime module fixtures, 3 snapshot fixtures, and 4 `walktop` fixture groups,
`scripts/build-docs-site.sh` passed, `scripts/check-docs-site.sh` passed, and
`git diff --check` passed. The practical next step is a narrow stability slice:
choose one user-visible helper or workflow from `docs/ROADMAP.md`, update the
contract docs first, add positive and negative proof, verify through the public
compiler, and only then release.

v6.3.1 release verification on 2026-05-31:

```text
make clean passed
make walk WALK_VERSION=v6.3.1 passed
./build/walk version reported v6.3.1
examples/tinychain ../../build/walk check --warnings=error passed
examples/tinychain ../../build/walk test passed with 5 tests
examples/tinychain ../../build/walk run src/main.walk printed chain valid: true and the language gap list
examples/tinychain ../../build/walk build --warnings=error passed and produced build/tinychain
examples/tinychain ./build/tinychain printed chain valid: true and the language gap list
make test WALK_VERSION=v6.3.1 passed
make conformance WALK_VERSION=v6.3.1 passed with 20 pass fixtures, 60 fail fixtures, 37 native executions, 4 compat fixtures, 12 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
scripts/release.sh v6.3.1 dist/v6.3.1 passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c dist/v6.3.1/SHA256SUMS passed for all 3 artifacts when run from dist/v6.3.1 as shasum -a 256 -c SHA256SUMS
dist/v6.3.1/walk-v6.3.1-darwin-arm64 version reported v6.3.1
NO_COLOR=1 dist/v6.3.1/walktop-v6.3.1-darwin-arm64 --once --fixture tools/walktop/testdata/basic passed
scripts/install-local.sh v6.3.1 passed, installed walk/runtime/walktop
walk version reported v6.3.1
command -v walktop returned /Users/shanewalker/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed
installed walk run --warnings=error examples/tinychain/src/main.walk printed chain valid: true and the language gap list
git ls-files '*.go' 'go.mod' 'go.sum' '*.js' returned empty
```

v6.3.0 release verification on 2026-05-27:

```text
make clean passed
make walk WALK_VERSION=v6.3.0 passed
./build/walk version reported v6.3.0
./build/walk test tests/pass/walk_tests.walk passed
./build/walk run tests/pass/stdlib.walk printed the new string.slice/string.prefix outputs
make test WALK_VERSION=v6.3.0 passed
make conformance WALK_VERSION=v6.3.0 passed with 20 pass fixtures, 60 fail fixtures, 37 native executions, 4 compat fixtures, 12 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
scripts/release.sh v6.3.0 dist/v6.3.0 passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c dist/v6.3.0/SHA256SUMS passed for all 3 artifacts when run from dist/v6.3.0 as shasum -a 256 -c SHA256SUMS
dist/v6.3.0/walk-v6.3.0-darwin-arm64 version reported v6.3.0
dist/v6.3.0/walktop-v6.3.0-darwin-arm64 passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v6.3.0 passed, installed walk/runtime/walktop
walk version reported v6.3.0
command -v walktop returned /Users/shanewalker/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed
installed walk run --warnings=error tests/pass/stdlib.walk printed the new string.slice/string.prefix outputs
git ls-files '*.go' 'go.mod' 'go.sum' '*.js' returned empty
```

v6.2.0 release verification on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v6.2.0 passed
./build/walk version reported v6.2.0
make test passed after rebuilding the dev test binary
make conformance WALK_VERSION=v6.2.0 passed with 20 pass fixtures, 55 fail fixtures, 37 native executions, 4 compat fixtures, 12 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
normal-sampling recipe from docs/STDLIB.md passed ./build/walk check --warnings=error
installed walk run --warnings=error <temp>/args.walk -- alpha 'two words' printed 2, 2, alpha, and two words
scripts/release.sh v6.2.0 dist/v6.2.0 passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c dist/v6.2.0/SHA256SUMS passed for all 3 artifacts
dist/v6.2.0/walk-v6.2.0-darwin-arm64 version reported v6.2.0
dist/v6.2.0/walktop-v6.2.0-darwin-arm64 passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v6.2.0 passed, installed walk/runtime/walktop, and verified walktop fixture mode
walk version reported v6.2.0
command -v walktop returned /Users/shanewalker/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed
git ls-files '*.go' 'go.mod' 'go.sum' '*.js' returned empty
```

v6.1.0 release verification on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v6.1.0 passed
./build/walk version reported v6.1.0
make test passed
make conformance WALK_VERSION=v6.1.0 passed with 20 pass fixtures, 52 fail fixtures, 37 native executions, 4 compat fixtures, 12 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/release.sh v6.1.0 dist/v6.1.0 passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c dist/v6.1.0/SHA256SUMS passed for all 3 artifacts
dist/v6.1.0/walk-v6.1.0-darwin-arm64 version reported v6.1.0
dist/v6.1.0/walktop-v6.1.0-darwin-arm64 passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v6.1.0 passed, installed walk/runtime/walktop, and verified walktop fixture mode
walk version reported v6.1.0
command -v walktop returned /Users/shanewalker/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed
git ls-files '*.go' 'go.mod' 'go.sum' '*.js' returned empty
```

v6.0.0 systems compiler final release verification on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v6.0.0 passed
./build/walk version reported v6.0.0
make test passed
make conformance passed with 20 pass fixtures, 52 fail fixtures, 36 native executions, 4 compat fixtures, 11 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
make walk WALK_VERSION=v6.0.0 passed again after conformance and relinked build/walk back to v6.0.0
scripts/build-docs-site.sh passed
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/release.sh v6.0.0 dist/v6.0.0 passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c dist/v6.0.0/SHA256SUMS passed for all 3 artifacts
dist/v6.0.0/walk-v6.0.0-darwin-arm64 version reported v6.0.0
dist/v6.0.0/walktop-v6.0.0-darwin-arm64 passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v6.0.0 passed, installed walk/runtime/walktop, and verified walktop fixture mode
walk version reported v6.0.0
command -v walktop returned /Users/shanewalker/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed
git ls-files '*.go' 'go.mod' 'go.sum' returned empty
git ls-files '*.js' returned empty
```

Phase 11 pre-removal parity proof on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v6.0.0-port-candidate passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk-ref ./cmd/walk passed before Go removal
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --check passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --native passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --runtime-modules passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --project passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --package passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --tooling passed
```

Phase 11 removal verification on 2026-05-26:

```text
make clean passed
make walk WALK_VERSION=v6.0.0-port-candidate passed
make test WALK_VERSION=v6.0.0-port-candidate passed
make conformance WALK_VERSION=v6.0.0-port-candidate passed with 20 pass fixtures, 52 fail fixtures, 36 native executions, 4 compat fixtures, 11 runtime module fixtures, 3 snapshot fixtures, and 4 walktop fixture groups
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
make release VERSION=v6.0.0-port-candidate OUT=<temp>/release passed and produced current-host walk, runtime, walktop, and SHA256SUMS artifacts
shasum -a 256 -c <temp>/release/SHA256SUMS passed
WALK_INSTALL_DIR=<temp>/bin scripts/install-local.sh v6.0.0-port-candidate passed, installed walk/runtime/walktop, and verified walktop fixture mode
git ls-files '*.go' 'go.mod' 'go.sum' returned empty
git ls-files '*.js' returned empty
find . -path ./.git -prune -o \( -name '*.go' -o -name 'go.mod' -o -name 'go.sum' -o -name '*.js' \) -print returned empty
```

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

Phase 6 C backend verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
make walk WALK_VERSION=v5.14.1-phase6-dev passed
./build/walk-cpp emit-c --warnings=error tests/pass/hello.walk -o build/phase6-hello.c matched tests/snapshots/hello.c
./build/walk-cpp run --warnings=error tests/pass/hello.walk printed hello, 3, and true
./build/walk-cpp test --warnings=error tests/pass/walk_tests.walk passed with 2 tests
cd tools/walktop && ../../build/walk-cpp test --warnings=error passed with 4 tests
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --emit-c passed with 3 snapshot fixtures
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --native passed with 20 pass fixtures, 3 compat fixtures, 2 walktop fixtures, and 25 native executions
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh passed and reported compatibility stress ok; the script explicitly skipped later-phase fmt/project lifecycle checks because walk-cpp still advertises those commands as not ported
go test -count=1 ./... passed
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
scripts/release.sh v5.14-cpp-backend <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp backend artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-backend and emitted C matching tests/snapshots/hello.c
git diff --check passed
git diff --cached --check passed
language-accounting impact checked: Phase 6 adds active C++ IR and C backend source and tests while Go and JavaScript remain until their later removal phases
```

Phase 7 CLI/project/package verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
make walk WALK_VERSION=v5.14.1-phase7-dev passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk-ref ./cmd/walk passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --project passed and reported conformance project: ok
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --package passed and reported conformance package: ok
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh passed and reported compatibility stress ok with project lifecycle checks active
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed after staging generated docs
WALK_RELEASE_BUILD_BIN=build/walk-cpp scripts/release.sh v5.14-cpp-project-package <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp project/package artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-project-package and passed project/package conformance smoke checks
WALK_INSTALL_DIR=<temp>/bin WALK_BUILD_BIN=build/walk-cpp scripts/install-local.sh v5.14-cpp-project-package passed and installed temp walk, walk-cpp, runtime source, and walktop without touching the normal local install
the temp installed walk-cpp reported v5.14-cpp-project-package and passed project/package conformance smoke checks with WALK_RUNTIME_DIR pointed at the temp runtime install
git diff --check passed
git diff --cached --check passed
language-accounting impact checked: Phase 7 adds active C++ project/package workflow source and tests while Go and JavaScript remain until their later removal phases
```

Phase 8 runtime module parity verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
go build -trimpath -ldflags "-X main.version=dev" -o build/walk-ref ./cmd/walk passed
make walk passed and built build/walk-cpp
go test -count=1 ./... passed
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --record passed with 11 runtime module fixtures and 36 native executions
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --runtime-modules passed and reported conformance runtime modules: 11 fixtures ok
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh passed and reported compatibility stress ok
WALK_RELEASE_BUILD_BIN=build/walk-cpp scripts/release.sh v5.14-cpp-runtime-modules <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp runtime-module artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
WALK_INSTALL_DIR=<temp>/bin WALK_BUILD_BIN=build/walk-cpp scripts/install-local.sh v5.14-cpp-runtime-modules passed and installed temp walk, walk-cpp, runtime source, and walktop without touching the normal local install
the temp installed walk-cpp reported v5.14-cpp-runtime-modules and passed runtime-module conformance with WALK_RUNTIME_DIR pointed at the temp runtime install
```

Phase 9 tooling parity verification on 2026-05-26:

```text
make test passed and reported C++ compiler tests passed
make walk WALK_VERSION=v5.14.1-phase9-dev passed
./build/walk-cpp version reported v5.14.1-phase9-dev
./build/walk-cpp help listed docs, debug-map, lsp, repl, and sitegen as ported commands
./build/walk-cpp fmt tests/pass/hello.walk printed formatted Walk source
./build/walk-cpp docs --strict generated Markdown and JSON API docs for examples/stable.walk
./build/walk-cpp debug-map generated JSON symbols for examples/stable.walk
printf '+ 1 2\n:quit\n' | ./build/walk-cpp repl printed 3 through the C++ compile/native execution path
scripts/build-docs-site.sh passed using build/walk-cpp
scripts/check-docs-site.sh passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --tooling passed and reported formatter, docs, debug-map, LSP, and REPL ok
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh passed and reported compatibility stress ok
WALK_RELEASE_BUILD_BIN=build/walk-cpp scripts/release.sh v5.14-cpp-tooling <temp>/release produced 5 walk artifacts, 1 current-host walk-cpp tooling artifact, 1 runtime source archive, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-tooling and listed docs, debug-map, lsp, repl, and sitegen as ported commands
WALK_INSTALL_DIR=<temp>/bin WALK_BUILD_BIN=build/walk-cpp scripts/install-local.sh v5.14-cpp-tooling passed and installed temp walk, walk-cpp, runtime source, and walktop without touching the normal local install
the temp installed walk-cpp reported v5.14-cpp-tooling and passed tooling conformance
WALK_BUILD_BIN=build/walk-cpp scripts/install-local.sh v5.14.1 refreshed the normal local walk, walk-cpp, runtime source, and walktop install
the installed walk and walk-cpp reported v5.14.1
WALK_REF=~/.local/bin/walk WALK_CANDIDATE=~/.local/bin/walk-cpp WALK_RUNTIME_DIR=~/.local/lib/walk/runtime tests/conformance/run.sh --tooling passed
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic passed after the local refresh
```

Phase 10 standard platform parity verification on 2026-05-26:

```text
make walk WALK_VERSION=v5.14-cpp-platform passed
cd tools/walktop && ../../build/walk-cpp check --warnings=error passed
cd tools/walktop && ../../build/walk-cpp test --warnings=error passed with 4 tests
./build/walk-cpp build --mode release --warnings=error tools/walktop/src/main.walk -o build/walktop passed
NO_COLOR=1 ./build/walktop --once --fixture tools/walktop/testdata/basic matched the deterministic dashboard
NO_COLOR=1 ./build/walktop --once passed live local OS-command mode
scripts/install-local.sh v5.14-cpp-platform refreshed the local walk, walk-cpp, runtime source, and walktop install through the default C++ walktop build driver
walk version reported v5.14-cpp-platform
walk-cpp version reported v5.14-cpp-platform
command -v walktop reported ~/.local/bin/walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic matched the deterministic dashboard
NO_COLOR=1 walktop --once passed live local OS-command mode
scripts/release.sh v5.14-cpp-platform <temp>/release produced 5 walk artifacts, 1 runtime source archive, 1 current-host walk-cpp artifact, 1 current-host walktop artifact built by walk-cpp, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the current-host walk-cpp release artifact reported v5.14-cpp-platform
the current-host walktop release artifact passed --once --fixture tools/walktop/testdata/basic
make test passed and reported C++ compiler tests passed
go test -count=1 ./... passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk-ref ./cmd/walk passed
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --check passed with 20 pass fixtures, 52 fail fixtures, 4 compat fixtures, 2 walktop fixtures, and ok
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --native passed with 20 pass fixtures, 3 compat fixtures, 11 runtime module fixtures, 2 walktop fixtures, and 36 native executions
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh passed and reported compatibility stress ok
```

Last release verification on 2026-05-26:

```text
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh passed
go test -count=1 ./... passed
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk ./cmd/walk passed
./build/walk version reported v5.14.1
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh passed and reported compatibility stress ok
scripts/release.sh v5.14.1 <temp>/release produced 5 walk artifacts, 1 runtime source archive, 1 current-host walk-cpp artifact, 1 current-host walktop artifact, and SHA256SUMS
shasum -a 256 -c SHA256SUMS passed for all 8 artifacts
the Darwin arm64 walk release artifact reported v5.14.1
the Darwin arm64 walk-cpp release artifact reported v5.14.1
the Darwin arm64 walktop release artifact passed --once --fixture tools/walktop/testdata/basic
scripts/install-local.sh v5.14.1 refreshed the local walk, walk-cpp, runtime source, and walktop install
walk version reported v5.14.1
walk-cpp version reported v5.14.1
WALK_REF=~/.local/bin/walk WALK_CANDIDATE=~/.local/bin/walk-cpp WALK_RUNTIME_DIR=~/.local/lib/walk/runtime tests/conformance/run.sh --runtime-modules passed with 11 runtime module fixtures
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
Phase 10 standard-platform parity worktree.

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

GitHub Linguist recognition prep on 2026-05-26: WalkLang now has local,
copy-ready recognition prep files without opening an upstream PR. The staged
metadata records `.walk` as the extension, `WalkLang` as the language name,
`source.walk` as the TextMate scope, and `#000088` as the language color.
Representative `.walk` samples live under `samples/WalkLang/`; draft upstream
snippets live under `linguist/`; and `.gitattributes` has a future-facing
WalkLang classification override. The upstream PR remains intentionally held
until public GitHub usage is broad enough for Linguist review.

GitHub Linguist prep verification on 2026-05-26:

```text
./build/walk check --warnings=error samples/WalkLang/route_ranker.walk passed
./build/walk check --warnings=error samples/WalkLang/pipeline_status.walk passed
ruby YAML validation for linguist/languages.yml and linguist/grammars.yml passed
scripts/build-docs-site.sh passed
scripts/check-docs-site.sh reported the expected generated STATUS page update plus pre-existing icon asset drift from icon_WalkLang.svg
git diff --check -- .gitattributes docs/GITHUB_LINGUIST.md docs/STATUS.md linguist/languages.yml linguist/grammars.yml samples/WalkLang passed
```

Next: post-release maintenance should keep the C++/C compiler, C runtime,
static docs surface, release artifacts, local install flow, and GitHub language
accounting aligned with the v6 systems architecture.
