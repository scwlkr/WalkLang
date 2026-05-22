# WalkLang Deprecation Policy

Deprecation is how WalkLang warns users before a stable feature changes or leaves the language.

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

## Policy

Stable v1 features should not be removed during the v1.x line.

A stable feature can become deprecated only when release notes and docs name:

```text
the deprecated feature
the replacement
the first version where the deprecation applies
the earliest version where removal is allowed
the migration steps
```

Removal of stable behavior normally waits for `v2.0.0`. A v1.x release may break stable behavior only for a documented safety or correctness fix.

## Current v1.9 Deprecated Surface

None.

## Draft And Experimental Features

Draft or experimental features can change without a deprecation period. They should still be called out in release notes when the change may surprise users.

Current draft or experimental areas include:

```text
file/json/matrix APIs
structs
methods
generic functions
traits
interfaces
closures
package manager behavior
debugger and full LSP behavior
```
