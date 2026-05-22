# WalkLang v1.1 Diagnostics

WalkLang diagnostics are intended to be deterministic enough for conformance tests.

## Shape

Compiler diagnostics use:

```text
file.walk:line:column: category: message
```

Example:

```text
main.walk:2:1: type error: x is int, got string
```

## Categories

Stable categories:

```text
syntax error
type error
name error
module error
warning
internal error
```

`internal error` means the compiler reached an unsupported path after parsing or checking. It is a compiler defect or unstable surface, not a user-facing language feature.

## Syntax Errors

Syntax errors come from invalid tokens, indentation, statements, expressions, or type syntax.

```walk
if: true
	out: 'tab' # tabs are invalid
```

## Type Errors

Type errors come from incompatible assignments, bad conditions, bad operators, bad calls, invalid output types, or unsupported array use.

```walk
var: x = 1
x = 'one'
```

## Name Errors

Name errors come from missing names, missing imports, non-callable names, or unknown library functions.

```walk
out: missing_name
```

## Module Errors

Module errors come from unavailable modules, invalid module top-level statements, import cycles, or invalid exports.

```walk
imp: calc
out: calc.hidden(1)
```

## Warnings

Warnings are non-fatal unless promoted.

```walk
var: x = 1
if: true
    var: x = 2
```

The current stable warning is shadowing an outer name.

```bash
walk check --warnings=off main.walk
walk check --warnings=default main.walk
walk check --warnings=error main.walk
```

## Runtime Failures

`walk test` builds and runs a native executable. Failed assertions print failure lines and make the executable exit non-zero.

Native C runtime failures are outside the stable diagnostic contract unless a WalkLang conformance test explicitly covers them.
