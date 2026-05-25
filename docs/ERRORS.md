# WalkLang v1 Diagnostics

WalkLang diagnostics are compiler-reported errors and warnings. They are
intended to be deterministic enough for conformance tests.

Use `runtime failure` for native program failures after compilation. Use
`failed assertion` for `walk test` assertion failures. Use `recoverable result
data` for draft APIs that return `ok`, `value`, and `error` fields instead of
stopping the native program.

Recoverable result data is runtime data, not a diagnostic, failed assertion, or runtime failure.
It is the preferred shape for ordinary documented failures in draft APIs that
are intended to let the program continue.

## Shape

Compiler diagnostics keep this stable first line:

```text
file.walk:line:column: category: message
```

Example:

```text
main.walk:1:16: type error: age is int, got string
```

The command-line display also includes the source line, a caret, and a focused suggestion when one is obvious:

```text
main.walk:1:16: type error: age is int, got string

var: age int = 'old'
               ^ string cannot initialize int
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

Type errors come from incompatible assignments, bad conditions, bad operators, bad calls, invalid output types, unsupported array use, or invalid struct field use.

```walk
var: x = 1
x = 'one'
```

v1.6 function inference diagnostics also use `type error`:

```walk
func: identity(value)
    return: value # type error: cannot infer type for parameter value
```

v2 struct diagnostics also use `type error`:

```walk
struct: User
    name string
    age int

var: user = User('Walker', 25)
out: user.height # type error: User has no field height
```

v2.2 generic diagnostics also use `type error`:

```walk
func: choose[T](left T, right T) T
    return: left

out: choose(1, 'one') # type error: arg 2 to choose needs T as int, got string
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

Stable warnings cover shadowing an outer name and unreachable statements after a block-terminating statement such as `return:`, `break`, or `continue`.

```bash
walk check --warnings=off main.walk
walk check --warnings=default main.walk
walk check --warnings=error main.walk
```

## Runtime Failures

`walk test` builds and runs a native executable. Failed assertions print failure
lines and make the executable exit non-zero. A failed assertion is not a
compiler diagnostic.

Stable v1.9 runtime failure messages include:

```text
walk runtime error: input reached EOF
walk runtime error: stdin read failed
walk runtime error: out of memory
walk runtime error: format failed
walk runtime error: string index out of range
walk runtime error: random.choice on empty array
```

Draft APIs may either runtime-stop or return recoverable result data. Their
current failure behavior is documented with each draft API in `docs/STDLIB.md`.

Other native C runtime failures are outside the stable diagnostic contract
unless a WalkLang conformance test explicitly covers them.
