# WalkLang Stable Feature Notes

These notes summarize the stable WalkLang feature set that current docs,
fixtures, and release checks protect.

Contract docs:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
docs/DESIGN_RULES.md
docs/COMPATIBILITY.md
docs/DEPRECATION.md
docs/INSTALL.md
docs/MIGRATING.md
docs/PROJECTS.md
docs/RELEASE_NOTES.md
docs/STATUS.md
```

## Function Type Inference

Typed function signatures remain valid and are still best for public APIs that need explicit contracts.

```walk
func: add(a int, b int) int
    return: + a b
```

Local helpers may omit parameter and return types when the body proves them clearly.

```walk
func: power_four(n)
    return: ^ n 4
```

If the body is ambiguous, add the annotation:

```walk
func: identity(value int)
    return: value
```

Inference is local to the function body. WalkLang does not infer ordinary function signatures from later call sites.

## Input

`in:` reads one required line from stdin and returns `string`.

```walk
var: name = in:
var: answer = in: 'Name? '
```

The optional prompt is any `string` expression. It writes to stdout without a newline and flushes before reading. `in:` strips `\n` or `\r\n`, preserves all other whitespace, returns `''` for an empty line, and treats immediate EOF as a runtime error.

## String Interpolation

Put an expression inside `{}` when a string should include a display value.

```walk
imp: string

var: word = 'paddle'
var: wordLength = string.len(word)
out: 'the secret word is {wordLength} characters long'
```

Interpolation accepts `int`, `float`, `bool`, `string`, and nullable string
values. Use doubled braces for literal braces.

```walk
out: '{{word}}'
```

## Terminal Game Helpers

Strings can be indexed by zero-based byte position. The result is a one-character string.

```walk
out: 'walk'[1]
```

`array.push` returns a new array instead of mutating in place.

```walk
imp: array

var: guessed array[string] = []
guessed = array.push(guessed, 'w')
```

Use module helpers for contains checks, string building, and random choice:

```walk
imp: string
imp: random

var: words = ['dog', 'cat']
out: string.contains('dog', 'o')
out: string.concat('walk', 'lang')
out: random.choice(words)
```

## Module Rules

User modules are sibling `.walk` files imported by bare name.

```walk
# math_extra.walk
func: cube(x int) int
    return: * x x x

exp: cube
```

```walk
# main.walk
imp: math_extra

out: math_extra.cube(3)
```

Rules:

- `imp: name` loads `name.walk` from the importing file's directory unless `name` is a built-in module.
- Imported calls stay namespaced: `math_extra.cube(3)`.
- Only functions named by `exp:` are callable from another file.
- Private helper functions may be used inside the module but not through the imported namespace.
- Module files may contain only `imp:`, `func:`, and `exp:` at top level.

## Stable Built-In APIs

```text
math.sqrt(number) -> float
math.pow(number, number) -> float
string.len(string) -> int
string.at(string, int) -> string
string.contains(string, string) -> bool
string.concat(string, string) -> string
array.len(array[T]) -> int
array.contains(array[T], T) -> bool
array.push(array[T], T) -> array[T]
time.now() -> int
random.int(int, int) -> int
random.choice(array[T]) -> T
testing.assert(bool) -> bool
```

## Diagnostics And Warnings

The CLI prints the stable diagnostic first line, followed by source snippets, caret locations, and focused suggestions when obvious.

The checker emits warnings for shadowing outer names and unreachable statements.

```bash
go run ./cmd/walk check examples/stable.walk
go run ./cmd/walk check --warnings=off examples/stable.walk
go run ./cmd/walk check --warnings=error examples/stable.walk
```

## Build Flow

Native builds still go through generated C and `cc`.

Install the local CLI first:

```bash
scripts/install-local.sh local
walk version
```

```bash
walk build examples/stable.walk -o build/stable --release
./build/stable
```

Use a different C compiler or flags when needed:

```bash
walk build examples/stable.walk -o build/stable --cc clang --cflag -Wall
```

Run the compatibility stress path:

```bash
scripts/stress-compatibility.sh
```

Run the focused stable compatibility suite:

```bash
go test ./cmd/walk -run TestStableCompatibilitySuite
```

Build release CLI artifacts:

```bash
scripts/release.sh v5.13.1
```

## Project Mode

```bash
walk init hello
cd hello
walk check
walk build
walk test
walk fmt
walk clean
```

Project mode is defined in `docs/PROJECTS.md`.

Run conformance:

```bash
go test ./...
```
