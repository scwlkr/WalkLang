Fixed gaps embedded:

```text
Typing: static with inference
Variables can change type: no
Optional explicit types: yes
```

# ARCHITECTURE.md

## WalkLang Architecture

WalkLang is a compiled, general-purpose language for readable native programs on any OS.

```text
main.walk -> lexer -> parser -> typed IR -> generated C + Walk C runtime -> native executable
```

The active compiler is the C++/C systems port. It checks source files, lowers
checked programs to typed IR, emits deterministic C, links that C with the Walk
runtime, and runs source-file, project, package, docs, LSP, REPL, and `walktop`
workflows without a Go reference implementation.

Example:

```walk
# main.walk
out: 'hello'
```

Build shape:

```text
main.walk -> main.c -> main / main.exe
main.c links with runtime/walk_runtime.c and the host runtime/platform file
```

Build mode is explicit at the tool boundary. Debug mode is the default and uses
inspectable native flags (`-g -O0`); release mode uses optimized native flags
(`-O3 -DNDEBUG`). `--release` remains a compatibility alias for
`--mode release`.

---

## 1. Identity

```text
Name: WalkLang
Ext:  .walk
Kind: compiled
Target: C
Output: native executable
VM: no
GC: no public promise
```

Purpose:

```text
Readable source.
Native output.
Small rules.
No syntax ideology maze.
```

Example:

```walk
var: a = 5
var: b = 10
out: + a b
```

Means:

```text
create a
create b
print a + b
```

Core text IO stays small:

```walk
var: name = in: 'Name? '
out: name
```

`in:` reads one required stdin line. Larger IO surfaces belong in explicit modules.
Draft side-effect module calls go through `do:` so effects remain visible:

```walk
imp: io

do: io.write('Loading')
do: io.write_line('done')
```

Draft scope cleanup uses `defer:` with the same visible effect boundary:

```walk
imp: term

do: term.color('red')
defer: do term.reset()
```

---

## 2. Design Priorities

Order matters:

```text
1. readability
2. speed
3. simplicity
4. safety
5. flexibility
```

Example:

```walk
var: result = * (+ a b) (- c d)
```

Preferred over:

```text
dense precedence tricks
```

WalkLang chooses visible structure over clever syntax.

---

## 3. Language Shape

WalkLang is:

```text
procedural first
limited functional
not OOP
not class-based
not inheritance-based
```

Example:

```walk
func: add(a int, b int) int
    return: + a b

out: add(2, 3)
```

Not stable syntax:

```walk
class: Person       # error
```

Structs, methods, and simple generic functions are experimental composition
features without classes or inheritance.

---

## 4. Syntax Model

Core visual rules:

```text
blocks: indentation
commands: keyword:
math: prefix
calls: name(a, b)
strings: 'single quotes'
lines: newline-terminated
grouping: (), []
```

Example:

```walk
if: > score 90
    out: 'A'
else:
    out: 'not A'
```

No braces, no semicolons:

```walk
if: > score 90 { out: 'A'; }   # error
```

---

## 5. Type Model

WalkLang is statically typed.

Types are inferred unless written.

```walk
var: age = 25        # inferred int
var: score float = 9 # explicit float

age = 26             # ok
age = 'old'          # error: int name cannot hold string
```

Rule:

```text
A name gets one type at declaration.
That type never changes.
```

---

## 6. Core Types

```text
int
float
bool
string
array
null
function
struct (experimental)
```

Example:

```walk
var: count = 10
var: price = 3.14
var: ok = true
var: name = 'Walker'
var: nums = [1, 2, 3]
```

Null needs a nullable type:

```walk
var: nickname string? = null
nickname = 'scwlkr'
```

Invalid:

```walk
var: nickname = null    # error: unknown nullable type
```

---

## 7. Memory Model

WalkLang has no public memory syntax.

```text
No malloc.
No free.
No pointers in source.
No GC promise.
```

Memory is internal to compiler/runtime output.

Example:

```walk
var: names = ['a', 'b', 'c']
out: names[0]
```

The user writes array logic, not allocation logic.

The current runtime model makes array storage explicit in the C runtime:

```text
array literals allocate item storage through the walk runtime helper
array item storage is owned for the process lifetime
arrays can be returned from functions without pointing at expired stack storage
```

This is still not source-level memory management. WalkLang programs do not call `malloc`, `free`, or pointer APIs.

---

## 8. Modules

Imports use namespaces.

```walk
imp: math

out: math.sqrt(9)
```

Exports name public values.

```walk
# calc.walk
func: square(x int) int
    return: * x x

exp: square
```

Use:

```walk
imp: calc

out: calc.square(4)
```

User module loading is file-local:

```text
main.walk imp: calc -> calc.walk
calc.square -> generated C symbol calc__square
```

Package imports are still module imports, but the module name is dotted and
resolved through the package cache:

```walk
imp: geometry.core

out: geometry.core.double(3)
```

Resolution shape:

```text
walk.toml [dependencies] geometry = "0.1.0"
walk package resolve <registry>
walk.lock verifies geometry@0.1.0
.walk/packages/geometry/0.1.0/src/geometry/core.walk
geometry.core.double -> generated C symbol geometry__core__double
```

---

## 9. Standard Library

Stable libraries:

```text
math
string
array
time
random
testing
```

Example:

```walk
imp: math
imp: random

var: x = random.int(1, 10)
out: math.sqrt(x)
```

`testing.assert(bool)` is stable as a namespaced assertion helper for `walk test`.

Draft library areas are not stable until they appear in `docs/STDLIB.md` and pass conformance tests.

```text
file
json
matrix
```

---

## 10. Error Model

WalkLang has compiler errors, warnings, and native runtime stops.

Compile error:

```walk
var: age = 25
age = 'old'
```

Diagnostic:

```text
main.walk:2:1
type error: age is int, got string
```

Runtime stop:

```walk
var: x = / 5 0
```

Diagnostic:

```text
runtime error: divide by zero
```

No exception-style recovery:

```walk
try:          # error
catch:
```

---

## 11. Tooling Architecture

Initial compiler:

```text
compiler
formatter
clear errors
```

Exploration tooling:

```text
test runner
basic REPL
```

Project tooling:

```text
module loader
warning levels
release build flags
cross-platform CLI release script
```

Specification and conformance:

```text
stable contract docs
pass/fail conformance fixtures
generated C snapshots
current status doc
```

Professional tooling:

```text
language server
editor integration
docs generator
debug map
```

Runtime and backend maturity:

```text
external Walk C runtime and host platform layer
process-lifetime array storage
source comments in generated C
optimized native release builds
runtime/backend contract docs
```

Current IO groundwork:

```text
do: effect statements
draft io/process modules
built-in API registry for new IO functions
process-lifetime runtime-created strings
fail-stop policy until recoverable IO results exist
UTF-8 text IO before binary IO
```

Later:

```text
debugger
reference site
playground
advanced backends
```

Formatter example:

```walk
var:nums=[1,2,3]
```

Becomes:

```walk
var: nums = [1, 2, 3]
```

---

## 12. Non-Negotiables

WalkLang must feel:

```text
readable
consistent
explicit
professional
command-oriented
common-sense natural
```

WalkLang must not become:

```text
symbol soup
magic-heavy
overly implicit
visually noisy
ideology-heavy
hard to explain
```

Example of accepted style:

```walk
func: total(a int, b int, fee int) int
    return: + a b fee
```

Rejected style:

```text
clever symbols that save 3 chars but cost understanding
```
