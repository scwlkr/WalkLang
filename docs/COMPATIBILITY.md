# WalkLang v1 Compatibility

WalkLang v1.1 is the first contract-focused checkpoint for the v1 language line. v1.4 adds compatible diagnostics and developer-experience improvements on top of the v1.3 standard library foundation.

## Stable Surface

Stable v1 behavior is defined by:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
tests/pass/
tests/fail/
tests/snapshots/
```

If a feature is accepted by the compiler but absent from those files, treat it as experimental.

## Compatibility Promise

Stable v1 programs should continue to compile through the v1.x line unless one of these applies:

```text
the behavior was undocumented
the behavior failed conformance tests
the behavior relied on internal generated C shape
a safety or correctness fix requires a documented break
```

## Version Meanings

```text
v1.1.x: bug fixes and diagnostic clarifications
v1.x.0: compatible additions to stable v1 behavior
v2.0.0: breaking language changes allowed
```

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

## Changing The Contract

When changing stable behavior:

1. Update `docs/SPEC.md`.
2. Update syntax, stdlib, diagnostics, or compatibility docs when relevant.
3. Add or update pass/fail fixtures.
4. Update generated C snapshots when backend output intentionally changes.
5. Update `docs/STATUS.md` with the new project state.

## Snapshot Compatibility

Generated C snapshots are regression tests for selected backend shapes. They do not freeze every byte of every generated program, but they do make covered output changes explicit.
