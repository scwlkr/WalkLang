# WalkLang Compatibility

WalkLang compatibility is tied to the current project version and the feature
statuses documented in `docs/SPEC.md`, `docs/STDLIB.md`, and
`docs/LANGUAGE_CONCEPTS.md`.

## Compatibility Promise

Stable features should continue to compile and behave as documented unless one
of these applies:

```text
the behavior was undocumented
the behavior failed conformance or compatibility tests
the behavior relied on internal generated C shape outside snapshots
a safety or correctness fix requires a documented break
```

When a safety or correctness fix breaks stable code, the release notes and migration guide must name the break.

## Stability Labels

```text
experimental
  may change anytime and is not compatibility-protected

draft
  intended shape, but still not compatibility-protected

stable
  part of the compatibility promise

deprecated
  still works, but has a documented replacement

removed
  no longer accepted
```

## Stable Surface

Stable behavior is defined by:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
docs/COMPATIBILITY.md
tests/pass/
tests/fail/
compatibility fixtures
tests/snapshots/
```

If a feature is accepted by the compiler but absent from the stable surface,
treat it as draft, experimental, or planned according to the labels in
`docs/LANGUAGE_CONCEPTS.md`.

## Compatibility Test Suite

Run the focused stable compatibility suite with:

```bash
go test ./cmd/walk -run TestStableCompatibilitySuite
```

The full repository test command also runs it:

```bash
go test ./...
```

The suite covers representative stable programs, test syntax, user-module exports, stable stdlib APIs, and selected stable diagnostic first lines.

## Draft, Experimental, Or Future Features

These are implemented or planned, but not compatibility-protected as stable
features:

```text
structs (experimental)
methods (experimental)
generic functions (experimental)
do: effect calls and defer: scope cleanup (draft)
io, parse, process, file, dir, path, json, term, http, and html APIs (draft)
matrix APIs (planned)
traits (planned)
interfaces (planned)
closures (planned)
anonymous functions (planned)
package manager behavior (draft)
project config behavior (draft)
LSP behavior (draft)
debugger behavior beyond debug-map (planned)
any generated C details outside snapshots
```

## Deprecation Policy

`docs/DEPRECATION.md` owns the deprecation lifecycle. Current deprecated
surface: none.

## Changing The Contract

When changing stable behavior:

1. Update `docs/SPEC.md`.
2. Update syntax, stdlib, diagnostics, compatibility, migration, or deprecation docs when relevant.
3. Add or update pass/fail and compatibility fixtures.
4. Update generated C snapshots when backend output intentionally changes.
5. Update release notes when the change affects users.
6. Update `docs/STATUS.md` with the new project state.

## Snapshot Compatibility

Generated C snapshots are regression tests for selected backend shapes. They do not freeze every byte of every generated program, but they do make covered output changes explicit.
