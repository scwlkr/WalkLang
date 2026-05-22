# WalkLang v1.5 Compatibility

WalkLang v1.5 defines the compatibility promise for the v1.x language line. v1.5 does not intentionally change stable v1 syntax or standard-library behavior; it makes the release boundary explicit.

## Compatibility Promise

Stable v1 code should continue to compile through the v1.x line unless one of these applies:

```text
the behavior was undocumented
the behavior failed conformance or compatibility tests
the behavior relied on internal generated C shape outside snapshots
a safety or correctness fix requires a documented break
```

When a safety or correctness fix breaks stable code, the release notes and migration guide must name the break.

## Version Meanings

```text
v1.0.x: bug fixes only
v1.x.0: compatible improvements to stable v1 behavior
v2.0.0: breaking language changes allowed
```

## Stability Labels

```text
experimental
  may change anytime and is not compatibility-protected

draft
  intended shape, but still not compatibility-protected

stable
  part of the v1 compatibility promise

deprecated
  still works, but has a documented replacement

removed
  no longer accepted
```

## Stable Surface

Stable v1 behavior is defined by:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
docs/COMPATIBILITY.md
tests/pass/
tests/fail/
tests/compat/v1/
tests/snapshots/
```

If a feature is accepted by the compiler but absent from those files, treat it as experimental.

## Compatibility Test Suite

Run the focused v1 compatibility suite with:

```bash
go test ./cmd/walk -run TestV15CompatibilitySuite
```

The full repository test command also runs it:

```bash
go test ./...
```

The suite covers representative stable programs, test syntax, user-module exports, stable stdlib APIs, and selected stable diagnostic first lines.

## Experimental Or Future Features

These are not compatibility-protected in v1:

```text
structs
methods
traits
interfaces
closures
anonymous functions
file/json/matrix APIs
package manager behavior
project config behavior
LSP/debugger behavior
any generated C details outside snapshots
```

## Deprecation Policy

`docs/DEPRECATION.md` owns the deprecation lifecycle. Current v1.5 deprecated surface: none.

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
