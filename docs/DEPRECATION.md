# WalkLang Deprecation Policy

Deprecation is how WalkLang warns users before a stable feature changes or leaves the language.

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

## Policy

Stable features should not be removed without an explicit release-note and
migration entry.

A stable feature can become deprecated only when release notes and docs name:

```text
the deprecated feature
the replacement
the first version where the deprecation applies
the earliest version where removal is allowed
the migration steps
```

Removal of stable behavior normally waits for a clearly named future project
version. A release may break stable behavior only for a documented safety or
correctness fix.

## Current Deprecated Surface

None.

## Draft, Experimental, And Planned Features

Draft or experimental features can change without a deprecation period. They
should still be called out in release notes when the change may surprise users.
Planned features are named direction only; they are not importable or
compatibility-protected until implemented and documented elsewhere.

Current draft, experimental, or planned areas include:

```text
do: effect calls (draft)
defer: scope cleanup (draft)
io, parse, process, file, dir, path, json, term, http, and html APIs (draft)
matrix APIs (planned)
structs (experimental)
methods (experimental)
generic functions (experimental)
traits (planned)
interfaces (planned)
closures (planned)
anonymous functions (planned)
package manager behavior (draft)
project config behavior (draft)
LSP behavior (draft)
debugger behavior beyond debug-map (planned)
```
