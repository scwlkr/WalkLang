# WalkLang v1 Design Rules

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

## IO Effect Policy

Side-effect module calls use one draft statement form:

```walk
do: module.effect(args)
```

Do not add one-off command keywords for each IO action. `out:` and `in:` stay
as the small stable console primitives; broader IO belongs in explicit modules.

Draft IO APIs must say whether they are fail-stop or recoverable. Until a
recoverable result shape is stable, broad IO such as files, processes, JSON, and
networking must remain draft or gated.

Recoverable result values are the preferred policy for ordinary documented failures.
Use fail-stop runtime failures only when the program cannot reliably continue,
when the API is intentionally an effect helper with no recovery surface, or
when a separate recoverable helper already exists.

Draft recoverable IO uses concrete result structs instead of nullable-only
returns. The first accepted draft shape is:

```text
ok bool
value T
error string
```

`ok` is true only when `value` is meaningful. `error` is `''` on success and a
short machine-testable code on failure, such as `eof`, `invalid int`, or
`stdin read failed`. This shape is intentionally concrete until generic result
structs are stable.

Runtime-created strings live for the native process lifetime. This is acceptable
for CLI-oriented IO and not sufficient justification for long-running servers.

Early IO is UTF-8 text IO. Binary IO, streaming handles, and path normalization
need separate design before stabilization.

The current draft file slice uses native process paths without normalization,
`~` expansion, or binary mode. Relative paths resolve against the process
current working directory. Draft `file.read`, `file.write`, and `file.append`
are fail-stop helpers; `file.try_read`, `file.try_write`, and
`file.try_append` return draft recoverable result structs for ordinary
file/path/read/write failures; `file.exists` returns a boolean path-existence
check.

New built-in IO APIs must be registered in the built-in API registry with their
module, function name, parameter types, return type, effect flag, draft/stable
status, and C runtime helper.

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
