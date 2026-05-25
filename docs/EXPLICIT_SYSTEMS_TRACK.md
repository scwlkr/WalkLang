# WalkLang Explicit Systems Track

Status: implemented in v5.13.0.

Date opened: 2026-05-25.

This document is the persistent tracker for one explicit-language implementation
slice. It covers four related improvements inspired by Odin-style language
discipline:

1. Scope cleanup with `defer:`.
2. Result values instead of exception-like control flow.
3. Package collections as explicit import roots.
4. First-class build modes.

This is one implementation track, not multiple phases. Implement all four items
in one release slice, or stop and record the blocker here before changing the
scope.

## Design Position

WalkLang stays explicit.

- Keep imports visible.
- Keep side effects visible through `do:` or another documented effect form.
- Keep calls namespaced.
- Do not add hidden preludes, implicit imports, operator overloading, exceptions,
  broad macros, or reflection-heavy runtime behavior in this track.
- Do not claim stable behavior until `docs/SPEC.md`, `docs/STDLIB.md`, tests,
  generated docs, native execution, status, release notes, and release artifacts
  prove it.

## Status

| Item | Status | Notes |
| --- | --- | --- |
| `defer:` scope cleanup | Implemented | New draft statement form. |
| Result-value policy | Implemented | Recoverable result behavior standardized in docs and conformance tests. |
| Package collections | Implemented | Explicit import roots documented; reserved roots enforced. |
| Build modes | Implemented | Debug/release mode selection is explicit and testable. |

## One-Pass Scope

### 1. `defer:` Scope Cleanup

Goal: add explicit cleanup that runs at scope exit without hidden destructors.

Required first-pass syntax:

```walk
imp: term
imp: io

do: term.color('red')
defer: do term.reset()
do: io.write_line('error')
```

Required behavior:

- `defer:` is draft until promoted by a later compatibility decision.
- A first-pass deferred statement may only be `do: module.effect(args)`.
- `defer:` may appear anywhere a statement is allowed inside `main`, `func:`,
  `if:`, `while:`, `for:`, and `test:` blocks.
- Deferred statements are owned by the lexical block that contains them.
- Deferred statements execute in last-in, first-out order when that block exits.
- Deferred statements execute on normal fallthrough and before a `return:` leaves
  the owning block or any parent block.
- A `defer:` inside a loop body is scoped to that iteration's loop-body block and
  runs before the next iteration starts or before the loop exits.
- Deferred effect-call arguments are evaluated when the `defer:` statement is
  encountered. The cleanup call uses those captured values at scope exit.
- If a deferred effect runtime-stops, native execution stops immediately; later
  deferred effects are not guaranteed to run.

Required diagnostics:

- Reject `defer:` at top level in imported module files unless the module file is
  being compiled as an entry or test source.
- Reject non-effect calls: `type error: defer requires do: effect call`.
- Reject non-call bodies such as `defer: out: 'x'`, `defer: return: 1`,
  `defer: var: x = 1`, and nested `defer: defer: ...`.
- Reject unknown modules, unimported modules, bad argument counts, and bad
  argument types through the existing module-call diagnostics.

Proof required:

- Parser/formatter test for `defer: do term.reset()`.
- Checker fail fixtures for each invalid form above.
- Native runtime test proving LIFO order.
- Native runtime test proving `defer:` runs before early `return:`.
- Native runtime test proving loop-body defers run once per iteration.
- Generated C snapshot update when emitter output changes.

### 2. Result Values Instead Of Exceptions

Goal: make recoverable failure explicit data, not hidden control flow.

Required first-pass scope:

- Do not add exceptions.
- Do not add a generic `Result[T]` type in this pass.
- Keep current concrete result structs, such as `IOReadResult`,
  `ParseIntResult`, `FileReadResult`, and `HttpResult`.
- Promote the recoverable-result shape to a documented language design rule:

```text
ok bool
value T
error string
```

Required behavior:

- `ok` is true only when `value` is meaningful.
- `error` is `''` on success.
- `error` is a short, stable, machine-testable code on ordinary recoverable
  failure.
- Allocation failure, invalid compiler state, and unrecoverable backend/runtime
  failures may still runtime-stop.
- APIs that are intended to be recoverable must not runtime-stop for ordinary
  documented failures.
- APIs that intentionally fail-stop must say so in `docs/STDLIB.md`.

Required diagnostics and docs:

- `docs/LANGUAGE_CONCEPTS.md` must define recoverable result data separately from
  runtime failure.
- `docs/DESIGN_RULES.md` must make result values the preferred recoverable-error
  policy.
- `docs/STDLIB.md` must label each draft IO/process/file/json/term/http API as
  recoverable or fail-stop.
- `docs/ERRORS.md` must keep compiler diagnostics, failed assertions,
  recoverable result data, and runtime failures distinct.

Proof required:

- Native tests proving representative recoverable APIs return `ok false` and a
  stable `error` string for ordinary failures.
- Native tests proving representative fail-stop APIs still produce the documented
  `walk runtime error: ...` message.
- Conformance test proving docs and current runtime behavior match for at least
  one `io`, one `file`, one `parse`, and one `http` result shape.

### 3. Package Collections

Goal: formalize explicit import roots without adding hidden package behavior.

Required first-pass scope:

- Do not change import syntax.
- Keep user, package, and built-in imports explicit through `imp:`.
- Treat the first segment of a dotted package import as the collection root.
- Keep existing package imports such as `imp: geometry.core` valid.
- Reserve `std` as a future first-party collection root.
- Reject publishing a user package named `std`.
- Do not move current built-in modules from `math`, `string`, `array`, `time`,
  `random`, `testing`, or draft module names into `std.*` in this pass.

Required package model:

```text
imp: geometry.core

collection root: geometry
module path: geometry/core.walk
generated C prefix: geometry__core__
```

Required diagnostics:

- `walk package init std` must fail with a clear reserved-name diagnostic.
- `walk package publish` must reject package names reserved for current or future
  built-in roots.
- Import-cycle and module-not-found diagnostics must keep naming dotted package
  modules precisely.

Proof required:

- Package lifecycle test showing a collection-root package imports and runs.
- Package publish/init tests rejecting `std`.
- Existing v3 package tests must keep passing.
- Docs must explain collection roots in `docs/PROJECTS.md` and `docs/V3.md`.

### 4. First-Class Build Modes

Goal: make debug and release behavior explicit in CLI, project config, docs, and
generated native builds.

Required first-pass CLI:

```bash
walk build --mode debug
walk build --mode release
walk build --debug
walk build --release
walk run --mode debug
walk run --mode release
walk run --debug
walk run --release
```

Required compatibility:

- Keep `--release` as a supported alias for `--mode release`.
- Add `--debug` as a supported alias for `--mode debug`.
- Default mode is `debug`.
- Project config may use `mode = "debug"` or `mode = "release"` under `[build]`.
- Existing `release = true|false` remains supported for compatibility.
- If both `mode` and `release` appear in `walk.toml`, `mode` wins and a warning
  is emitted.
- CLI mode wins over project config.
- Reject conflicting CLI flags such as `--debug --release` or
  `--mode debug --release`.

Required native behavior:

- Debug mode emits native compiler flags suitable for inspection:
  `-g -O0`.
- Release mode keeps current behavior:
  `-O3 -DNDEBUG`.
- User-supplied `--cflag` values append after mode flags.
- The generated C remains inspectable in both modes.

Required diagnostics:

- Unknown mode error: `unknown build mode "<value>"`.
- Conflict error: `build mode conflict: choose one of debug or release`.
- Invalid project config error must point at `[build].mode`.

Proof required:

- CLI parser tests for `--mode`, aliases, defaults, and conflicts.
- Project config tests for `mode`, compatibility with `release`, warning when
  both are present, and CLI override.
- Native build test proving debug flags are present.
- Existing release test proving `-O3 -DNDEBUG` remains present.
- Docs updates in `docs/PROJECTS.md`, `docs/V5.md`, `docs/ARCHITECTURE.md`, and
  release notes.

## Required One-Pass Execution Order

Follow this order in one branch and one release slice:

1. Read this file, `docs/LANGUAGE_CONCEPTS.md`, `docs/DESIGN_RULES.md`,
   `docs/SPEC.md`, `docs/STDLIB.md`, `docs/PROJECTS.md`, `docs/V3.md`,
   `docs/V5.md`, `IO_PLAN.md`, and the current checker/parser/emitter/package
   code.
2. Add failing tests for all four items before implementation.
3. Implement parser, AST, formatter, checker, emitter/runtime, CLI, project
   config, package, and docs changes needed by those tests.
4. Update generated snapshots and generated docs only after source docs and code
   behavior agree.
5. Run the full verification gate below.
6. Update `docs/STATUS.md` and `docs/RELEASE_NOTES.md` with exact commands and
   outcomes.
7. Commit, tag, publish, and refresh the local installed `walk` binary following
   the repo release flow.

Do not split this into separate defer/result/package/build-mode phases. If the
slice becomes too large to finish safely, stop before partial implementation,
record the blocker in this file, and leave tests/docs showing the exact boundary.

## Verification Gate

Minimum commands before this track can be marked complete:

```bash
go test -count=1 ./...
go build -trimpath -ldflags "-X main.version=<version>" -o build/walk ./cmd/walk
./build/walk version
./build/walk check --warnings=error tests/pass/walk_tests.walk
WALK_BIN=$PWD/build/walk scripts/stress-v1.sh
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh <version> <temp>/release
wc -l <temp>/release/SHA256SUMS
git diff --check
```

Also run focused tests added for this track and record their exact names in
`docs/STATUS.md`.

## Status Log

- 2026-05-25: Track opened from design discussion. No implementation has started.
  Current decision: keep WalkLang explicit and implement these four items as one
  future release slice.
- 2026-05-25: Track implemented for v5.13.0. Draft `defer:` cleanup,
  recoverable-result policy docs, package collection-root enforcement, and
  first-class build modes are covered by focused tests and the release
  verification gate recorded in `docs/STATUS.md`.
