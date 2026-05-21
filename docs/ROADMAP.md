# ROADMAP.md

## WalkLang Roadmap

Build the smallest useful language first.

```text
v0: compile real programs
v0.1: test and explore
v1: harden and grow
later: ecosystem
```

---

## v0 Goal

A `.walk` file compiles to a native executable through C.

Example target:

```walk
func: add(a int, b int) int
    return: + a b

var: nums = [1, 2, 3]

for: n in nums
    out: add(n, 10)
```

Expected output:

```text
11
12
13
```

---

## v0 Includes

Language:

```text
.walk files
indentation blocks
single quotes
prefix math
static types
type inference
optional annotations
var / const
if / else
while / for / repeat
break / continue
func / return
recursion
arrays
null
function values
imp / exp
```

Example:

```walk
var: age = 25
var: name string? = null

if: > age 18
    name = 'adult'
```

---

## v0 Compiler

Pipeline:

```text
lexer
parser
AST
type checker
C emitter
native build
```

Example compile flow:

```text
main.walk
  -> tokens
  -> AST
  -> typed IR
  -> main.c
  -> executable
```

---

## v0 Tooling

Required:

```text
compiler
formatter
clear errors
```

Formatter example:

```walk
var:x=+ 1 2
```

Becomes:

```walk
var: x = + 1 2
```

Error example:

```walk
var: x = 1
x = 'one'
```

Output:

```text
main.walk:2
type error: x is int, got string
```

---

## v0 Standard Library Target

Core libraries:

```text
math
string
array
matrix
file
json
time
random
testing base
```

Example:

```walk
imp: math
imp: random

var: n = random.int(1, 10)
out: math.sqrt(n)
```

---

## v0 Non-Goals

Do not build these yet:

```text
classes
structs
methods
inheritance
interfaces
closures
anonymous functions
try/catch
networking
package manager
debugger
full LSP
```

Example:

```walk
class: User # rejected until later
```

---

## v0 Done When

v0 is done when this works:

```walk
# main.walk

imp: math

func: add(a int, b int) int
    return: + a b

func: distance(x1 float, y1 float, x2 float, y2 float) float
    return:
        math.sqrt(
            +:
                ^ (- x2 x1) 2
                ^ (- y2 y1) 2
        )

var: nums = [1, 2, 3]

for: n in nums
    out: add(n, 10)

var: d = distance(0, 0, 3, 4)

if: == d 5
    out: 'distance is 5'
else:
    out: 'distance is not 5'
```

Expected:

```text
11
12
13
distance is 5
```

---

## v0.1 Goal

Make the language easier to test and explore.

Add:

```text
test runner
basic REPL
better diagnostics
standard library polish
```

Test example:

```walk
test: 'add works'
    assert: == add(2, 3) 5
```

REPL example:

```text
walk> + 1 2
3
```

---

## v1 Goal

Make WalkLang feel stable.

Add or stabilize:

```text
module rules
library APIs
formatter rules
warning levels
docs
release builds
cross-platform build flow
```

Example module target:

```walk
# math_extra.walk
func: cube(x int) int
    return: * x x x

exp: cube
```

Use:

```walk
imp: math_extra

out: math_extra.cube(3)
```

---

## v1 Data Modeling

Add structs after v0 is solid.

Target syntax:

```walk
struct: User
    name string
    age int

var: user = User('Walker', 25)
out: user.name
```

No inheritance.

Preferred future model:

```text
structs + methods later
traits maybe later
inheritance no
```

---

## Later Ecosystem

Later tools:

```text
language server
package manager
debugger
networking
project templates
docs generator
```

Package example target:

```walk
imp: http # later only
```

---

## Design Rule For Every Future Feature

Every feature must pass this test:

```text
Can it be explained with one small example?
Does it keep code readable at a glance?
Does it avoid magic?
Does it compile predictably to C/native output?
```

Example accepted:

```walk
const: max_users = 100
```

Example rejected:

```text
a clever shortcut that hides behavior
```

---

## Version Summary

```text
v0
  compile .walk to native exe
  core syntax + types + functions + loops

v0.1
  tests + REPL + diagnostics

v1
  stable modules + stdlib + docs + build flow

v2
  structs + methods

later
  package manager + LSP + debugger
```
