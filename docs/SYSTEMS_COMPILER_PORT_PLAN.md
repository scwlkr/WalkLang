# WalkLang Systems Compiler Port Plan

Status: accepted architecture direction

Document role: execution contract

Owner: scwlkr

Last updated: 2026-05-26

## Purpose

This document defines the complete port from the current reference compiler to
the permanent systems-language architecture for WalkLang.

The final architecture is:

```text
Walk source
-> C++ compiler core
-> typed Walk IR
-> C backend
-> C runtime and platform ABI layer
-> native executable
```

The final repository language posture is:

```text
compiler core: C++
runtime and platform layer: C
optional target-specific leaf routines: assembly
generated backend output: C
website/docs surface: HTML and CSS
build/release glue: minimal shell and Makefile only where needed
```

The current Go compiler is the reference implementation and conformance oracle
for the port. It is not the permanent compiler identity. The port is complete
only when the repository no longer contains Go compiler source, Go module
metadata, or JavaScript site assets, and the C++ compiler preserves the current
WalkLang feature surface through real tests and native execution.

## Non-Negotiable Final State

The port is complete only when all of these are true:

1. `walk` is built from C++ and C source.
2. The compiler frontend, semantic checker, package/project system, formatter,
   docs generator, debug map, REPL, LSP server, and release tooling no longer
   depend on Go.
3. The Walk runtime is a C library with a stable internal ABI used by generated
   C.
4. Generated C remains deterministic, inspectable, snapshot-tested, and native
   compiler friendly.
5. `walktop` builds from WalkLang source through the C++ compiler and ships as
   a standalone native command.
6. All current stable, draft, and experimental WalkLang features are either
   preserved or explicitly redesigned in this document before implementation.
   The default requirement is preservation.
7. No `.go`, `go.mod`, or `go.sum` files remain in the final repository.
8. No JavaScript files remain in the final docs/site surface.
9. GitHub language breakdown for the active source has no Go and no JavaScript.
10. Release artifacts are built by the new C++/C toolchain and verified through
    the same public commands users run.

Do not satisfy the language-breakdown goal with Linguist overrides that hide
active compiler source. Mark generated artifacts as generated only when they
are generated artifacts. The final absence of Go and JavaScript must be real.

## Current Baseline

Current project version at plan creation: `v5.14.0`.

Current implementation shape:

```text
.walk source -> Go reference compiler -> generated C -> native executable
```

Current proof surface to preserve:

```text
go test -count=1 ./...
go build -trimpath -ldflags "-X main.version=v5.14.0" -o build/walk ./cmd/walk
./build/walk version
./build/walk check --warnings=error tests/pass/walk_tests.walk
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.14.0 <out-dir>
scripts/install-local.sh v5.14.0
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
```

The port must replace these proofs with C++/C equivalents before deleting the
reference compiler.

## Architecture Decision

WalkLang will use a C++ compiler core, a C runtime, a C backend, and optional
assembly leaf routines.

### Why C++ For The Compiler Core

C++ is the permanent compiler implementation language because the compiler
needs:

- deterministic ownership and layout control
- high-performance lexer, parser, semantic analysis, and code generation
- strong internal data structures without a runtime VM
- RAII for compiler-owned resources
- direct control over allocation strategy
- straightforward native debugger behavior
- credibility with systems programmers evaluating the implementation

The compiler uses a restricted C++ style:

```text
C++20
no exceptions in compiler control flow
no RTTI requirement
explicit Result<T> / ErrorOr<T> returns for recoverable compiler failures
arena allocation for AST and IR nodes
value types for tokens, symbols, type IDs, and source ranges
stable diagnostic IDs and deterministic diagnostic ordering
no hidden global mutable compiler state
no compiler dependency on JavaScript, Python, Node, or Go
```

Allowed C++ standard library surfaces:

```text
string and string_view
vector
array
span or local equivalent
unordered_map only behind deterministic wrappers
filesystem only at CLI/project boundaries
optional only for local value absence
variant only when it replaces unsafe tagged unions cleanly
```

Disallowed compiler-core habits:

```text
exceptions as ordinary control flow
implicit process exits below main
raw owning pointers
undisciplined shared ownership
locale-sensitive parsing
non-deterministic map iteration in emitted output or diagnostics
test-only behavior branches in production compiler code
```

### Why C For The Runtime And ABI Layer

C is the permanent runtime layer because generated C must bind to simple,
stable, inspectable runtime functions. The runtime must not require C++ name
mangling, C++ exceptions, or C++ standard-library ABI details.

Runtime shape:

```text
runtime/walk_runtime.h
runtime/walk_runtime.c
runtime/platform/walk_platform_posix.c
runtime/platform/walk_platform_windows.c
runtime/asm/<target>/*.S
```

Runtime rules:

- Public WalkLang source still has no pointer syntax, no `malloc`, no `free`,
  no ownership annotations, and no public garbage collector promise.
- Runtime memory ownership stays internal to generated programs.
- Runtime failures are consistent native program failures with clear messages.
- Recoverable result data remains ordinary data, not exceptions.
- Every assembly routine has a C fallback and a targeted benchmark or
  correctness test.

### Why C Remains The Backend Target

C is the primary backend target because it is portable, inspectable, and already
matches the current native-output identity. The generated C is not a side
effect of convenience. It is the backend contract.

Generated C must remain:

```text
predictable
portable
debuggable
deterministic
readable enough to inspect
compatible with ordinary C compilers
```

Generated C must not become:

```text
a dump of unstable compiler internals
an opaque macro maze
a wrapper around a hidden interpreter
a dependency on the removed reference compiler
```

### Self-Hosting Boundary

Self-hosting is not part of this port. This port establishes the permanent
C++/C/assembly implementation. This plan does not replace the C++ compiler with
a WalkLang compiler.

## Target Repository Layout

The final repo layout is:

```text
compiler/
  main.cpp
  cli/
  source/
  lex/
  parse/
  ast/
  sema/
  ir/
  codegen/c/
  format/
  docs/
  lsp/
  repl/
  project/
  package/
  support/
runtime/
  walk_runtime.h
  walk_runtime.c
  platform/
  asm/
include/
  walk/
tests/
  pass/
  fail/
  snapshots/
  compat/
  conformance/
tools/
  walktop/
docs/
site/
  assets/site.css
public/
Makefile
scripts/
```

Final removed paths:

```text
cmd/
internal/
go.mod
go.sum
scripts/sitegen.go
site/assets/site.js
public/assets/site.js
```

Replacement paths:

```text
compiler/docs/sitegen.cpp
compiler/docs/api_docs.cpp
compiler/lsp/server.cpp
compiler/repl/repl.cpp
scripts/release.sh
scripts/install-local.sh
Makefile
```

## Build System Contract

Use a plain top-level `Makefile` plus small POSIX shell scripts.

Required targets:

```text
make walk
make test
make conformance
make docs
make check-docs
make release VERSION=vX.Y.Z OUT=dist
make install-local VERSION=vX.Y.Z
make clean
```

Required compiler variables:

```text
CXX
CC
AR
CXXFLAGS
CFLAGS
LDFLAGS
WALK_VERSION
WALK_RELEASE_MODE
```

Default local build:

```bash
make walk
./build/walk version
```

Release build:

```bash
make release VERSION=vX.Y.Z OUT=dist
```

Cross-platform release targets remain:

```text
darwin/arm64
darwin/amd64
linux/amd64
linux/arm64
windows/amd64
```

The final release gate must produce equivalent `walk` and `walktop` artifacts
or explicitly document a supported replacement artifact naming convention before
the first C++/C release.

## Feature Preservation Matrix

The port must preserve this feature set.

| Area | Current proof | Required C++/C proof |
| --- | --- | --- |
| CLI dispatch | `cmd/walk/main.go` tests | C++ CLI command tests |
| Direct file alias | `walk file.walk` | `walk file.walk` native execution |
| `walk run` | command tests | C++ command test plus native output |
| `walk build` | command and release tests | generated C plus native executable |
| `walk emit-c` | command tests and snapshots | deterministic emitted C snapshots |
| `walk check` | checker tests | diagnostics and warnings parity |
| `walk test` | WalkLang test fixtures | native test executable proof |
| `walk fmt` | formatter tests | source formatting parity |
| `walk clean` | project tests | build artifact cleanup test |
| `walk init` | project tests | project scaffold test |
| `walk package` | package tests | lockfile and package workflow tests |
| `walk docs` | generated API docs tests | Markdown and JSON docs output parity |
| `walk debug-map` | tooling tests | source-to-C mapping proof |
| `walk lsp` | LSP tests | initialize, diagnostics, and formatting proof |
| `walk repl` | REPL tests | expression compile/run parity |
| `walk version` | version test | C++ version embedding proof |
| indentation lexer | parser/fail fixtures | lex/parse tests and fail fixtures |
| single-quoted strings | pass/fail fixtures | parse and generated C proof |
| prefix math | pass fixtures | native output proof |
| variables/constants | checker tests | type and assignment diagnostics |
| static types/inference | checker tests | type table and diagnostic parity |
| `if`, `else`, loops | pass/fail fixtures | native control-flow proof |
| functions | pass/snapshots | semantic and C emission proof |
| function values | pass fixtures | type and call proof |
| modules/imports/exports | module tests | namespace and export diagnostics |
| arrays | pass/fail/runtime tests | runtime allocation and indexing proof |
| nullable string | pass/fail tests | type and runtime output proof |
| interpolation | pass/fail tests | formatting helper proof |
| tests/assertions | `walk test` | native test runner proof |
| warnings | checker tests | deterministic warning output |
| snippets/carets | diagnostic tests | exact formatting parity |
| `in:` | input tests | stdin native proof |
| draft `do:` | pass/fail tests | effect boundary proof |
| draft `defer:` | explicit systems tests | cleanup ordering proof |
| draft `io` | IO tests | stdout/stderr/stdin proof |
| draft `parse` | IO tests | result data proof |
| draft `process` | IO tests | command/env/cwd/exit proof |
| draft `file`, `dir`, `path` | IO tests | filesystem proof |
| draft `json` | IO tests | validation and compacting proof |
| draft `term` | walktop and IO tests | color/cursor/size proof |
| draft `http` | networking docs/tests | curl-backed result proof or C HTTP replacement proof |
| draft `html` | pass/fail tests | HTML text helper proof |
| experimental structs | pass/fail fixtures | field layout and diagnostics |
| experimental methods | pass/fail fixtures | receiver and call proof |
| experimental generics | pass/fail fixtures | instantiation proof |
| generated C snapshots | `tests/snapshots` | stable C snapshot parity |
| compatibility stress | `scripts/stress-compatibility.sh` | C++ compiler compatibility stress |
| docs site | Go site generator | C++ static site generator |
| `walktop` | WalkLang source and tests | built by C++ compiler and installed |
| release scripts | `scripts/release.sh` | C++/C release artifacts and checksums |

No phase can mark complete while it regresses a row already ported.

## Status Board

Update this table at the end of every porting phase.

| Phase | Status | Required exit proof |
| --- | --- | --- |
| 0. Port Contract | Complete | This document exists, is linked, and generated docs build |
| 1. Conformance Oracle | Not started | Dual-compiler harness records current behavior from the reference compiler |
| 2. Runtime Extraction | Not started | Generated C links against `runtime/walk_runtime.c` with current tests passing |
| 3. C++ Skeleton | Not started | `make walk` builds a C++ `walk` that supports version/help and test harness entrypoints |
| 4. Lexer, Parser, AST | Not started | C++ frontend parses current pass fixtures and rejects fail syntax fixtures |
| 5. Semantic Checker | Not started | C++ checker matches current type, name, module, and warning diagnostics |
| 6. C Backend | Not started | C++ compiler emits C that passes native behavior and snapshot checks |
| 7. CLI, Project, Package | Not started | C++ `walk` supports current command surface and project/package tests |
| 8. Runtime Module Parity | Not started | Draft runtime modules pass native tests through the C runtime |
| 9. Tooling Parity | Not started | Formatter, docs generator, debug map, LSP, and REPL pass parity checks |
| 10. Standard Platform Parity | Not started | `walktop` builds, tests, runs, installs, and releases through C++ `walk` |
| 11. Remove Go And JavaScript | Not started | No Go or JavaScript source remains; docs/site still build |
| 12. Final Release Gate | Not started | C++/C release artifacts, checksums, install, docs, and language breakdown are verified |

Status values:

```text
Not started
In progress
Blocked
Complete
```

## Global Execution Rules

Every phase must follow these rules:

1. Read this document and `docs/STATUS.md` first.
2. Work on the first phase whose status is not `Complete`.
3. Do not re-litigate the accepted architecture.
4. Preserve unrelated local edits. At plan creation, unrelated dirty playground
   files are known to exist.
5. Keep current features working until the replacement proof for that feature
   exists.
6. Write or update tests before claiming a ported feature works.
7. Compare C++ compiler behavior against the reference compiler until the final
   removal phase.
8. Update this status board, `docs/STATUS.md`, and release notes when a phase
   changes project state.
9. Regenerate public docs after docs changes.
10. Commit phase work in reviewable chunks.
11. Run release flow for completed executable phases.
12. Do not remove the reference compiler until the final removal phase gate is
    satisfied.

## Verification Ladder

Use the strongest applicable proof for the phase.

### Phase-Local Proof

```bash
make test
make conformance
```

During early phases, when the C++ build system does not exist yet, use the
current baseline:

```bash
go test -count=1 ./...
scripts/build-docs-site.sh
scripts/check-docs-site.sh
```

### Dual-Compiler Proof

Before the reference compiler is removed, any ported feature must pass:

```text
reference compiler accepts/rejects fixture
C++ compiler accepts/rejects same fixture
generated C behavior matches or intentionally documented difference exists
native executable output matches for pass fixtures
diagnostic text matches for fail fixtures where exact text is part of the contract
```

### Final No-Go/No-JS Proof

The final removal phase must run:

```bash
git ls-files '*.go' 'go.mod' 'go.sum'
git ls-files '*.js'
find . -path ./.git -prune -o \( -name '*.go' -o -name 'go.mod' -o -name 'go.sum' -o -name '*.js' \) -print
```

Expected output after Phase 11:

```text
no active Go files
no Go module metadata
no JavaScript files
```

After push, verify GitHub language accounting:

```bash
gh api repos/scwlkr/WalkLang/languages
```

Expected final language result:

```text
no Go key
no JavaScript key
C++ present
C present
Assembly present only if assembly files exist
HTML/CSS present for website/docs
```

## Phase 0: Port Contract

Status: Complete

Goal: establish the accepted architecture and the execution contract.

Files:

```text
docs/SYSTEMS_COMPILER_PORT_PLAN.md
docs/README.md
docs/STATUS.md
docs/RELEASE_NOTES.md
scripts/sitegen.go
public/
```

Required work:

1. Create this document.
2. Link it from the docs front door.
3. Add it to the generated docs site navigation.
4. Record the status update.
5. Regenerate the public docs site.
6. Verify the current baseline remains green.

Exit proof:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
go test -count=1 ./...
git diff --check -- . ':(exclude)playground/hangman.walk' ':(exclude)playground/hangman-v2.walk'
```

Commit message:

```text
docs: add systems compiler port plan
```

## Phase 1: Conformance Oracle

Status: Not started

Goal: turn the current reference compiler behavior into an executable oracle so
the C++ port is measured against real language behavior instead of memory.

Files to create:

```text
tests/conformance/README.md
tests/conformance/manifest.tsv
tests/conformance/run.sh
tests/conformance/expected/
tests/conformance/tmp/
```

Files to modify:

```text
scripts/stress-compatibility.sh
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Build a manifest covering every current pass fixture, fail fixture,
   compatibility fixture, snapshot fixture, and `walktop` fixture.
2. For each pass fixture, record:
   - source path
   - command mode: check, run, build, test, or emit-c
   - expected stdout
   - expected stderr when meaningful
   - expected generated C snapshot when meaningful
   - native execution requirement
3. For each fail fixture, record:
   - source path
   - command mode
   - expected diagnostic suffix or exact diagnostic
   - expected exit failure
4. Add a conformance runner that can invoke:
   - `WALK_REF`
   - `WALK_CANDIDATE`
5. Make the runner compare behavior when both compilers are available.
6. Make the runner produce a clear summary:

```text
conformance: N pass fixtures ok
conformance: N fail fixtures ok
conformance: N native executions ok
conformance: ok
```

7. Keep the runner shell-based or C++-based. Do not add Python, Node, or Go.
8. Add docs explaining how to refresh expected output from the reference
   compiler.

Exit proof:

```bash
go build -trimpath -ldflags "-X main.version=v5.14.0" -o build/walk-ref ./cmd/walk
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --record
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --verify
WALK_BIN=$PWD/build/walk-ref scripts/stress-compatibility.sh
scripts/build-docs-site.sh
scripts/check-docs-site.sh
go test -count=1 ./...
```

Phase completion prompt:

```text
/goal Complete Phase 1 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Treat the document as the execution contract. Build the conformance oracle around the current reference compiler, record expected behavior for all current pass/fail/compat/snapshot/walktop fixtures, update the phase status and docs/STATUS.md, run the required exit proof, commit the result, and preserve unrelated playground edits.
```

## Phase 2: Runtime Extraction

Status: Not started

Goal: move the generated helper layer out of ad hoc emitted C strings and into a
real C runtime library with a stable internal ABI.

Files to create:

```text
runtime/walk_runtime.h
runtime/walk_runtime.c
runtime/platform/walk_platform.h
runtime/platform/walk_platform_posix.c
runtime/platform/walk_platform_windows.c
tests/runtime/
```

Files to modify:

```text
internal/emitter/emitter.go
cmd/walk/main.go
tests/snapshots/
scripts/release.sh
docs/RUNTIME_BACKEND.md
docs/ARCHITECTURE.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Extract current generated helper functions into `runtime/walk_runtime.c`.
2. Define stable internal names:

```text
walk_rt_panic
walk_rt_alloc_array
walk_rt_string_len
walk_rt_string_at
walk_rt_string_concat
walk_rt_format_int
walk_rt_format_float
walk_rt_format_bool
walk_rt_format_string
walk_rt_input_line
walk_rt_array_push_*
walk_rt_array_contains_*
walk_rt_file_*
walk_rt_dir_*
walk_rt_path_*
walk_rt_process_*
walk_rt_json_*
walk_rt_term_*
walk_rt_http_*
walk_rt_html_*
```

3. Change generated C to include `walk_runtime.h` and call runtime functions.
4. Update build commands so emitted C links with `runtime/walk_runtime.c` and
   any platform file.
5. Keep `walk emit-c` useful by either:
   - emitting a clear include/link comment, or
   - supporting `--single-file` for standalone generated C with embedded runtime.
6. Add runtime C tests for allocation, strings, arrays, process, files,
   terminal helpers, and error messages.
7. Update snapshots to the new runtime call shape.

Exit proof:

```bash
go test -count=1 ./...
go build -trimpath -ldflags "-X main.version=v5.14.0" -o build/walk ./cmd/walk
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.14-runtime <temp>/release
```

Phase completion prompt:

```text
/goal Complete Phase 2 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Extract the generated runtime helpers into a real C runtime and platform layer, keep generated C deterministic, update snapshots/docs/status, prove all current tests and compatibility stress still pass, run release proof for the changed backend, commit the result, and preserve unrelated playground edits.
```

## Phase 3: C++ Skeleton

Status: Not started

Goal: introduce the permanent C++ compiler executable and build system without
claiming language parity yet.

Files to create:

```text
Makefile
compiler/main.cpp
compiler/cli/command.h
compiler/cli/command.cpp
compiler/support/result.h
compiler/support/source_file.h
compiler/support/source_file.cpp
compiler/support/diagnostic.h
compiler/support/diagnostic.cpp
compiler/support/version.h
tests/cpp/
```

Files to modify:

```text
scripts/install-local.sh
scripts/release.sh
docs/INSTALL.md
docs/TOOLING.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Add `make walk` that builds `build/walk-cpp`.
2. Add CLI command parsing for:

```text
version
help
check
emit-c
run
build
test
fmt
clean
init
package
docs
debug-map
lsp
repl
```

3. Commands other than `version` and `help` must return a clear
   `not ported in this phase` diagnostic until implemented.
4. Add version embedding through `WALK_VERSION`.
5. Add a C++ unit-test harness compiled by `make test`.
6. Add deterministic diagnostics plumbing and source file loading.
7. Do not route unimplemented C++ commands to the reference compiler.

Exit proof:

```bash
make clean
make walk WALK_VERSION=v5.14-cpp-skeleton
./build/walk-cpp version
./build/walk-cpp help
make test
go test -count=1 ./...
scripts/build-docs-site.sh
scripts/check-docs-site.sh
```

Phase completion prompt:

```text
/goal Complete Phase 3 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Add the C++ compiler skeleton, Makefile, command dispatch, version/help behavior, source/diagnostic support, and C++ test harness without delegating commands to the reference compiler. Update docs/status, run the required proof, commit the result, and preserve unrelated playground edits.
```

## Phase 4: Lexer, Parser, AST

Status: Not started

Goal: port source loading, lexing, indentation handling, parsing, and AST
construction to C++.

Files to create:

```text
compiler/lex/token.h
compiler/lex/lexer.h
compiler/lex/lexer.cpp
compiler/parse/parser.h
compiler/parse/parser.cpp
compiler/ast/ast.h
compiler/ast/ast.cpp
compiler/ast/arena.h
tests/cpp/lexer_tests.cpp
tests/cpp/parser_tests.cpp
```

Files to modify:

```text
compiler/main.cpp
compiler/cli/
tests/conformance/manifest.tsv
docs/SYSTEMS_COMPILER_PORT_PLAN.md
docs/STATUS.md
```

Required work:

1. Implement tokenization for names, numbers, strings, comments, symbols, and
   newlines.
2. Implement indentation block parsing with tabs rejected.
3. Implement AST nodes for all current statements and expressions:

```text
var
const
assignment
out
in:
func
return
if/else
while
for
break/continue
test
assert
imp
exp
struct
method-like function declarations
do
defer
array literal
index expression
field access
module call
function call
prefix operators
interpolation
null
```

4. Implement parser diagnostics with source ranges.
5. Wire `walk-cpp check --parse-only` for parser conformance.
6. Compare parser accept/reject behavior against the reference compiler for
   syntax fixtures.

Exit proof:

```bash
make test
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --parse
go test -count=1 ./...
```

Phase completion prompt:

```text
/goal Complete Phase 4 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port lexer, indentation handling, parser, and AST construction to C++. Prove parser parity against the conformance oracle for current pass/fail syntax fixtures, update docs/status and the phase board, commit the result, and preserve unrelated playground edits.
```

## Phase 5: Semantic Checker

Status: Not started

Goal: port type checking, name resolution, module resolution, warnings, and
semantic diagnostics.

Files to create:

```text
compiler/sema/checker.h
compiler/sema/checker.cpp
compiler/sema/scope.h
compiler/sema/scope.cpp
compiler/sema/types.h
compiler/sema/types.cpp
compiler/sema/modules.h
compiler/sema/modules.cpp
compiler/sema/builtins.h
compiler/sema/builtins.cpp
tests/cpp/checker_tests.cpp
tests/cpp/module_tests.cpp
```

Files to modify:

```text
compiler/cli/
tests/conformance/manifest.tsv
docs/SPEC.md
docs/STDLIB.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Implement symbol tables and deterministic scope lookup.
2. Implement stable types:

```text
int
float
bool
string
string?
array[T]
func(T...) R
null
void as internal marker only
```

3. Implement experimental type surfaces:

```text
structs
methods
simple generic functions
function values
```

4. Implement name, module, export, and package symbol rules.
5. Implement warnings:

```text
shadowing
unreachable statement
other current warning surfaces
```

6. Implement `--warnings=off|default|error`.
7. Match diagnostic formatting for fail fixtures.
8. Wire C++ `walk check`.

Exit proof:

```bash
make test
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --check
WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --fail-diagnostics
go test -count=1 ./...
```

Phase completion prompt:

```text
/goal Complete Phase 5 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port semantic checking, type rules, modules, warnings, and diagnostics to C++. Prove check/fail parity against the conformance oracle, update docs/status and the phase board, commit the result, and preserve unrelated playground edits.
```

## Phase 6: C Backend

Status: Not started

Goal: port typed IR lowering and C code generation to C++.

Files to create:

```text
compiler/ir/ir.h
compiler/ir/ir.cpp
compiler/ir/lower.h
compiler/ir/lower.cpp
compiler/codegen/c/c_emitter.h
compiler/codegen/c/c_emitter.cpp
compiler/codegen/c/name_mangle.h
compiler/codegen/c/name_mangle.cpp
tests/cpp/emitter_tests.cpp
```

Files to modify:

```text
tests/snapshots/
tests/conformance/manifest.tsv
runtime/
docs/RUNTIME_BACKEND.md
docs/ARCHITECTURE.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Define typed Walk IR that is independent from AST shape.
2. Lower checked AST to IR.
3. Emit deterministic C from IR.
4. Preserve module symbol names such as `module__symbol`.
5. Preserve source comments or replace them with a documented equivalent
   source-map comment format.
6. Emit runtime calls against `runtime/walk_runtime.h`.
7. Implement C build/link command in C++ CLI.
8. Wire:

```text
walk-cpp emit-c
walk-cpp build
walk-cpp run
walk-cpp test
```

9. Update snapshots only where the new runtime/IR architecture intentionally
   changes generated C shape.

Exit proof:

```bash
make test
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --emit-c
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --native
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh
go test -count=1 ./...
```

Phase completion prompt:

```text
/goal Complete Phase 6 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port typed IR lowering and C code generation to C++, wire emit-c/build/run/test, preserve deterministic generated C and native behavior, prove parity with the conformance oracle and compatibility stress, update docs/status and the phase board, commit the result, and preserve unrelated playground edits.
```

## Phase 7: CLI, Project, Package

Status: Not started

Goal: make the C++ compiler own the user-facing project and package workflows.

Files to create:

```text
compiler/project/project.h
compiler/project/project.cpp
compiler/package/package.h
compiler/package/package.cpp
compiler/support/toml_like.h
compiler/support/toml_like.cpp
tests/cpp/project_tests.cpp
tests/cpp/package_tests.cpp
```

Files to modify:

```text
compiler/cli/
scripts/install-local.sh
scripts/release.sh
docs/PROJECTS.md
docs/PACKAGES.md
docs/INSTALL.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Port `walk init`.
2. Port `walk clean`.
3. Port project `walk check`, `walk build`, `walk test`, and `walk fmt`.
4. Port `walk.toml` parsing and diagnostics.
5. Port package manifest, lockfile, install/publish/local cache behavior.
6. Preserve package checksum behavior.
7. Preserve command usage strings.
8. Make install and release scripts able to use `build/walk-cpp`.

Exit proof:

```bash
make test
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --project
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --package
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh
scripts/build-docs-site.sh
scripts/check-docs-site.sh
```

Phase completion prompt:

```text
/goal Complete Phase 7 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port CLI project mode and package workflows to C++, including walk.toml, lockfiles, package cache behavior, install/release script integration, docs/status updates, and conformance proof. Commit the result and preserve unrelated playground edits.
```

## Phase 8: Runtime Module Parity

Status: Not started

Goal: prove every current draft runtime module through the C runtime and C++
compiler.

Files to create or modify:

```text
runtime/
compiler/sema/builtins.cpp
compiler/codegen/c/
tests/runtime/
tests/conformance/manifest.tsv
docs/STDLIB.md
docs/RUNTIME_BACKEND.md
docs/NETWORKING.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Port all builtin module signatures to C++ semantic tables.
2. Port runtime-backed codegen for:

```text
io
parse
process
file
dir
path
json
term
http
html
```

3. Preserve recoverable result data shapes.
4. Preserve draft `do:` effect boundaries.
5. Preserve draft `defer:` cleanup ordering.
6. Keep HTTP behavior compatible with the documented backend. If replacing
   `curl` delegation with native C HTTP, update docs and tests in the same
   phase and preserve failure result behavior.
7. Add C runtime tests for every runtime family.

Exit proof:

```bash
make test
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --runtime-modules
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh
```

Phase completion prompt:

```text
/goal Complete Phase 8 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port every current draft runtime module through the C runtime and C++ compiler, preserve result data and effect boundaries, update docs/status, prove runtime-module conformance and compatibility stress, commit the result, and preserve unrelated playground edits.
```

## Phase 9: Tooling Parity

Status: Not started

Goal: port the non-build developer tools that make WalkLang feel complete.

Files to create:

```text
compiler/format/
compiler/docs/
compiler/debug_map/
compiler/lsp/
compiler/repl/
tests/cpp/format_tests.cpp
tests/cpp/docs_tests.cpp
tests/cpp/lsp_tests.cpp
tests/cpp/repl_tests.cpp
```

Files to remove by the end of this phase:

```text
scripts/sitegen.go
```

Files to modify:

```text
scripts/build-docs-site.sh
scripts/check-docs-site.sh
docs/DOCS_SITE.md
docs/TOOLING.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Port formatter.
2. Port docs generator for structured comments and API JSON/Markdown.
3. Port static docs site generator from Go to C++.
4. Port debug-map output.
5. Port LSP server enough to preserve current initialize, diagnostics, and
   formatting behavior.
6. Port REPL as a thin expression compiler over the C++ pipeline.
7. Update docs scripts to call the C++ tools.
8. Remove the Go site generator.

Exit proof:

```bash
make test
scripts/build-docs-site.sh
scripts/check-docs-site.sh
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --tooling
WALK_BIN=$PWD/build/walk-cpp scripts/stress-compatibility.sh
```

Phase completion prompt:

```text
/goal Complete Phase 9 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Port formatter, docs generation, static site generation, debug-map, LSP, and REPL to the C++ toolchain. Remove the Go site generator, update docs/status, prove tooling conformance plus docs-site checks, commit the result, and preserve unrelated playground edits.
```

## Phase 10: Standard Platform Parity

Status: Not started

Goal: prove the first standard-platform tool through the permanent compiler.

Files to modify:

```text
tools/walktop/
scripts/install-local.sh
scripts/release.sh
docs/STANDARD_PLATFORM.md
docs/TOOLING.md
docs/INSTALL.md
docs/STATUS.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
```

Required work:

1. Build `tools/walktop/src/main.walk` with the C++ compiler.
2. Run `tools/walktop` tests through C++ `walk test`.
3. Build a release-mode `walktop` native executable through C++ `walk build`.
4. Verify deterministic fixture mode.
5. Verify live local OS-command mode.
6. Install `walk` and `walktop` through the local install script.
7. Update release artifacts to use C++ `walk`.

Exit proof:

```bash
make walk WALK_VERSION=v5.14-cpp-platform
cd tools/walktop && ../../build/walk-cpp check --warnings=error
cd tools/walktop && ../../build/walk-cpp test --warnings=error
./build/walk-cpp build --mode release --warnings=error tools/walktop/src/main.walk -o build/walktop
NO_COLOR=1 ./build/walktop --once --fixture tools/walktop/testdata/basic
NO_COLOR=1 ./build/walktop --once
scripts/install-local.sh v5.14-cpp-platform
walk version
command -v walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
```

Phase completion prompt:

```text
/goal Complete Phase 10 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Prove walktop and the first standard-platform slice through the C++ compiler, including check/test/build/fixture/live/install/release-script paths. Update docs/status and the phase board, commit the result, and preserve unrelated playground edits.
```

## Phase 11: Remove Go And JavaScript

Status: Not started

Goal: remove the reference implementation and JavaScript site assets after the
C++/C implementation proves complete parity.

Files to remove:

```text
cmd/
internal/
go.mod
go.sum
scripts/sitegen.go
site/assets/site.js
public/assets/site.js
```

Files to modify:

```text
README.md
CONTRIBUTING.md
AGENTS.md
scripts/
site/
public/
docs/
docs/SYSTEMS_COMPILER_PORT_PLAN.md
docs/STATUS.md
docs/RELEASE_NOTES.md
```

Required work:

1. Remove all Go compiler and Go test source.
2. Remove Go module files.
3. Remove JavaScript from the docs/site surface.
4. Replace interactive docs-site behavior with static HTML/CSS behavior.
5. Remove references that describe the Go implementation as current.
6. Keep historical release notes intact when they describe historical releases.
7. Update installation and contribution docs for the C++/C toolchain.
8. Update CI to build and test with C++/C only.
9. Run final local source accounting:

```bash
git ls-files '*.go' 'go.mod' 'go.sum'
git ls-files '*.js'
```

10. Ensure output is empty.

Exit proof:

```bash
make clean
make walk WALK_VERSION=v6.0.0-port-candidate
make test
make conformance
scripts/build-docs-site.sh
scripts/check-docs-site.sh
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
git ls-files '*.go' 'go.mod' 'go.sum'
git ls-files '*.js'
find . -path ./.git -prune -o \( -name '*.go' -o -name 'go.mod' -o -name 'go.sum' -o -name '*.js' \) -print
```

Expected source-accounting output:

```text
empty
```

Phase completion prompt:

```text
/goal Complete Phase 11 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Remove the Go reference implementation, Go module metadata, Go docs tooling, and JavaScript site assets only after C++/C parity is proven. Update README, CONTRIBUTING, AGENTS, docs, CI, status, and release notes. Prove make/test/conformance/docs/stress and prove no Go or JavaScript files remain. Commit the result and preserve unrelated playground edits.
```

## Phase 12: Final Release Gate

Status: Not started

Goal: ship the first fully C++/C/assembly WalkLang release and prove public
language accounting.

Files to modify:

```text
README.md
docs/INSTALL.md
docs/STATUS.md
docs/RELEASE_NOTES.md
docs/ARCHITECTURE.md
docs/RUNTIME_BACKEND.md
docs/SYSTEMS_COMPILER_PORT_PLAN.md
scripts/release.sh
scripts/install-local.sh
```

Required work:

1. Set the release version for the completed port.
2. Build release artifacts through C++/C only.
3. Verify checksums.
4. Install locally.
5. Verify `walk` and `walktop`.
6. Push branch and tag.
7. Publish GitHub release.
8. Wait for GitHub language breakdown to refresh or query it through API.
9. Confirm no Go and no JavaScript in GitHub language results.
10. Mark this plan complete.

Exit proof:

```bash
make clean
make walk WALK_VERSION=v6.0.0
make test
make conformance
scripts/build-docs-site.sh
scripts/check-docs-site.sh
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
scripts/release.sh v6.0.0 <temp>/release
shasum -a 256 -c <temp>/release/SHA256SUMS
scripts/install-local.sh v6.0.0
walk version
command -v walktop
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
git diff --check
git status --short
gh release view v6.0.0
gh api repos/scwlkr/WalkLang/languages
```

Phase completion prompt:

```text
/goal Complete Phase 12 in docs/SYSTEMS_COMPILER_PORT_PLAN.md. Ship the first fully C++/C/assembly WalkLang release from the completed port. Run full tests, conformance, docs, stress, release artifacts, checksums, local install, git/tag/release verification, and GitHub language breakdown verification. Update status and mark the port complete.
```

## One-Shot Continuation Prompt

Use this prompt to resume the port in a fresh Codex session:

```text
/goal Continue the WalkLang systems compiler port in /Users/shanewalker/Desktop/dev/WalkLang. Read AGENTS.md, docs/STATUS.md, and docs/SYSTEMS_COMPILER_PORT_PLAN.md first. Treat docs/SYSTEMS_COMPILER_PORT_PLAN.md as the execution contract, not discussion material. Work on the first phase in its Status Board that is not Complete. Do not re-litigate the C++ compiler core, C runtime, C backend, and optional assembly architecture. Preserve unrelated local edits, especially playground files. Keep current WalkLang features working through tests and conformance. Update the phase status, docs/STATUS.md, release notes when appropriate, generated public docs, and verification evidence. Commit completed phase work and run the repo release flow when the phase changes executable behavior.
```

## Review Checklist Before Marking Any Phase Complete

A phase is not complete until every item is satisfied:

```text
phase status updated
docs/STATUS.md updated
release notes updated when user-visible behavior or release process changes
generated docs rebuilt when docs changed
targeted tests passed
full available regression tests passed
conformance proof passed for ported surfaces
language-accounting impact checked when source languages changed
git diff --check passed
unrelated dirty files preserved
commit created for phase work
release flow completed when executable behavior changed
```

## Explicit Anti-Shortcuts

The following do not count as port completion:

- a C++ wrapper that shells out to the Go compiler
- a C++ CLI that delegates unported commands to a hidden Go binary
- deleting tests to reduce parity requirements
- using Linguist overrides to hide active Go or JavaScript source
- keeping JavaScript because the docs site already had it
- changing generated C to an opaque interpreter loop
- shipping host-only release artifacts as the final release gate
- marking features removed without updating the feature preservation matrix,
  docs, migration notes, release notes, and conformance expectations

## Intended Result

At completion, WalkLang presents as a serious systems-language project:

```text
C++ compiler core
C runtime and platform layer
C backend
native executable output
optional assembly where it earns its place
static docs without JavaScript
no Go implementation footprint
no hidden prototype dependency
```

That is the architecture this port exists to deliver.
