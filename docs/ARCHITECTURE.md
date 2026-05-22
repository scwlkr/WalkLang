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
main.walk -> lexer -> parser -> typed IR -> C -> native executable
```

Example:

```walk
# main.walk
out: 'hello'
```

Build shape:

```text
main.walk -> main.c -> main / main.exe
```

---

## 1. Identity

```text
Name: WalkLang
Ext:  .walk
Kind: compiled
Target: C
Output: native executable
VM: no
GC: no in v0
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

WalkLang v0 is:

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

Not v0:

```walk
class: Person       # error in v0
```

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

WalkLang v0 is statically typed.

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

v0 has no public memory syntax.

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

v1 module loading is file-local:

```text
main.walk imp: calc -> calc.walk
calc.square -> generated C symbol calc__square
```

---

## 9. Standard Library Targets

v0 target libraries:

```text
math
string
array
matrix
file
json
testing
time
random
```

Example:

```walk
imp: math
imp: random

var: x = random.int(1, 10)
out: math.sqrt(x)
```

Matrix is library-supported, not a separate core type.

```walk
var: grid = [
    [1, 3],
    [2, 4]
]
```

Type:

```text
array[array[int]]
```

---

## 10. Error Model

v0 has compiler errors, warnings, and runtime stops.

Compile error:

```walk
var: age = 25
age = 'old'
```

Diagnostic:

```text
main.walk:2
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

No v0 recovery:

```walk
try:          # error in v0
catch:
```

---

## 11. Tooling Architecture

v0:

```text
compiler
formatter
clear errors
```

v0.1:

```text
test runner
basic REPL
```

v1:

```text
module loader
warning levels
release build flags
cross-platform CLI release script
```

Later:

```text
language server
package manager
debugger
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
