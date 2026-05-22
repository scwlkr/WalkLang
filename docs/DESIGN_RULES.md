# WalkLang v1.1 Design Rules

These rules keep the stable language small and predictable.

## Contract First

`docs/SPEC.md` owns the stable language surface.

```text
If it is not in SPEC.md, it is not stable WalkLang.
If the compiler disagrees with SPEC.md, either the compiler or the spec must change.
```

## Readability Over Cleverness

WalkLang favors visible structure.

```walk
var: result = * (+ a b) (- c d)
```

This is preferred over hidden precedence or dense punctuation.

## Explicit Boundaries

Stable features need all of these:

```text
spec text
syntax docs when user-facing
stdlib docs when imported
positive tests
negative tests when invalid forms matter
generated C snapshot coverage when backend output matters
```

## Small Standard Library

The standard library grows only when behavior is clear, testable, and documented. Draft APIs must stay out of `STDLIB.md` stable sections until they compile and pass tests.

## No Surprise Magic

Do not add implicit conversions, hidden imports, global state, or syntax aliases unless the spec is updated first and conformance tests prove the behavior.

## Native Backend Discipline

The stable surface is what can pass the full pipeline:

```text
.walk -> C -> native executable
```

Checker-only behavior is not stable if generated C cannot build and run.

## Diagnostics Are Product Surface

Common invalid programs need predictable diagnostics. If a diagnostic is asserted by a fail fixture, changing it is a compatibility decision.

## Formatter Rules Are Language Rules

The formatter defines canonical spacing and indentation for supported syntax. New syntax is not complete until `walk fmt` handles it.
