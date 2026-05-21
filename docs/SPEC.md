# SPEC.md

## WalkLang v0 Spec

This document defines valid v0 behavior.

---

## 1. Source File

A source file is UTF-8 text ending in `.walk`.

```text
main.walk
```

A file contains zero or more statements.

```walk
var: x = 1
out: x
```

---

## 2. Compilation Contract

Compiler pipeline:

```text
source -> tokens -> AST -> typed IR -> C -> executable
```

Example:

```walk
out: + 1 2
```

Compiles through C into a native executable that prints:

```text
3
```

---

## 3. Grammar Sketch

```text
file        := stmt*
stmt        := command | assign | expr
command     := keyword ':' payload? block?
assign      := target '=' expr
block       := INDENT stmt+ DEDENT
expr        := literal | name | call | index | prefix | grouped | block_expr
call        := name '(' args? ')'
args        := expr (',' expr)*
prefix      := op expr+
grouped     := '(' expr ')'
array       := '[' args? ']'
index       := expr '[' expr ']'
```

Example command:

```walk
out: 'hello'
```

Example expression:

```walk
* (+ a b) (- c d)
```

---

## 4. Keywords

Keywords cannot be names.

```text
var const out if else while for repeat break continue
func return imp exp true false null and or not in
```

Invalid:

```walk
var: return = 1 # error
```

---

## 5. Indentation

Indentation defines block ownership.

```walk
if: true
    out: 'owned by if'
out: 'top-level'
```

A dedent closes the block.

Tabs are invalid in v0.

---

## 6. Type System

WalkLang is statically typed.

```text
Every expression has a type.
Every name has one type.
Assignments must match that type.
```

Example:

```walk
var: x = 1
x = 2      # ok
x = 'two'  # error
```

---

## 7. Type Inference

The compiler infers types from values.

```walk
var: x = 1      # int
var: y = 1.5    # float
var: s = 'hi'   # string
```

Explicit type annotations are allowed.

```walk
var: x int = 1
var: y float = 1
```

If no single type can be proven, compile error.

```walk
var: x = null # error
```

Use:

```walk
var: x string? = null
```

---

## 8. Type Lock

A variable cannot change type.

```walk
var: value = 10
value = 20      # ok
value = false   # error
```

A function return type is also locked.

```walk
func: bad(x bool) int
    if: x
        return: 1
    return: 'no' # error
```

---

## 9. Numeric Rules

Types:

```text
int
float
```

Rules:

```text
int + int -> int
int with float -> float
/ always -> float
```

Example:

```walk
var: a = + 1 2     # int
var: b = + 1 2.5   # float
var: c = / 5 2     # float, 2.5
```

Integer division must be explicit later; v0 `/` is not integer division.

---

## 10. Boolean Rules

Conditions must be bool.

```walk
if: > age 18
    out: 'adult'
```

Invalid:

```walk
if: age
    out: 'bad' # error
```

Boolean operators:

```text
and: 2+ bool args -> bool
or:  2+ bool args -> bool
not: 1 bool arg -> bool
```

Example:

```walk
if: and (> age 18) (< age 65)
    out: 'working age'
```

---

## 11. String Rules

Strings use single quotes.

```walk
var: s = 'hello'
```

Escapes:

```text
\' single quote
\\ backslash
\n newline
\t tab
```

Example:

```walk
var: s = 'don\'t\nstop'
```

No implicit string conversion.

```walk
var: s = + 'age: ' 5 # error
```

Use explicit conversion later:

```walk
var: s = string.from_int(5)
```

---

## 12. Null Rules

`null` is valid only for nullable types.

```walk
var: name string? = null
```

Nullable values must be checked before non-null use.

```walk
if: != name null
    out: name
```

Invalid:

```walk
var: name string = null # error
```

---

## 13. Array Rules

Array type is `array[T]`.

```walk
var: nums = [1, 2, 3] # array[int]
```

Arrays are homogeneous.

```walk
var: bad = [1, true] # error
```

Mutable arrays allow element assignment.

```walk
nums[0] = 9
```

Type remains locked.

```walk
nums[0] = 'x' # error
```

---

## 14. Matrix Rules

Matrix is `array[array[T]]`.

```walk
var: m = [
    [1, 2],
    [3, 4]
]
```

All rows must share element type.

```walk
var: bad = [
    [1, 2],
    ['x', 'y']
] # error
```

Rectangular shape is recommended; matrix library functions may require it.

```walk
matrix.rows(m)
```

---

## 15. Constants

`const:` creates an immutable binding.

```walk
const: pi = 3.14
pi = 3 # error
```

Const array access is read-only through that name.

```walk
const: xs = [1, 2]
xs[0] = 9 # error
```

---

## 16. Scope

Scopes:

```text
global
function
block
```

Example:

```walk
var: x = 1

if: true
    var: x = 2
    out: x # 2

out: x # 1
```

Inner shadowing is allowed but may warn.

```walk
var: x = 1
if: true
    var: x = 2 # warning: shadows outer x
```

---

## 17. Assignment

Assignment updates existing names or indexed targets.

```walk
x = 2
items[0] = 9
```

Invalid assignment target:

```walk
+ a b = 3 # error
```

---

## 18. Prefix Operators

Math:

```text
+ - * / ^
```

Comparison:

```text
> < >= <= == !=
```

Boolean:

```text
and or not
```

Example:

```walk
if: and (> x 0) (< x 10)
    out: x
```

---

## 19. Function Declarations

Function form:

```walk
func: name(params) return_type
    body
```

Example:

```walk
func: add(a int, b int) int
    return: + a b
```

Parameter and return types may be omitted only if inferred.

```walk
func: add(a, b)
    return: + a b
```

If inference is ambiguous, compile error.

---

## 20. Function Calls

Call form:

```walk
name(arg1, arg2)
```

Example:

```walk
var: x = add(1, 2)
```

Wrong arity is compile error.

```walk
add(1) # error
```

---

## 21. Return Rules

`return:` exits the current function.

```walk
func: id(x int) int
    return: x
```

A function with return type must return on all paths.

```walk
func: sign(x int) int
    if: > x 0
        return: 1
    else:
        return: -1
```

---

## 22. Recursion

Functions may call themselves.

```walk
func: countdown(n int) int
    if: <= n 0
        return: 0
    return: countdown(- n 1)
```

---

## 23. Function Values

Named functions may be values.

```walk
func: inc(x int) int
    return: + x 1

func: run(f func(int) int, x int) int
    return: f(x)
```

No anonymous functions:

```walk
run((x) => + x 1, 3) # error
```

No closures:

```walk
func: outer()
    var: x = 1
    func: inner() int
        return: x # error in v0
```

---

## 24. Control Flow

`if`:

```walk
if: condition
    block
else:
    block
```

`while`:

```walk
while: condition
    block
```

`repeat`:

```walk
repeat: count
    block
```

`for`:

```walk
for: item in array
    block
```

Example:

```walk
for: n in [1, 2, 3]
    out: n
```

---

## 25. Loop Control

`break:` exits nearest loop.

```walk
while: true
    break:
```

`continue:` skips to next iteration.

```walk
for: n in nums
    if: == n 0
        continue:
    out: n
```

Outside loops, both are errors.

---

## 26. Modules

`imp:` imports a module namespace.

```walk
imp: math
out: math.sqrt(4)
```

`exp:` exports a name.

```walk
func: add(a int, b int) int
    return: + a b

exp: add
```

Unexported names remain private to the file.

---

## 27. Errors

Compiler errors stop compilation.

```walk
var: x = 1
x = 'one'
```

Error:

```text
type error
```

Runtime errors stop the program.

```walk
out: / 1 0
```

Error:

```text
divide by zero
```

Warnings do not stop by default.

```walk
var: unused = 1 # warning
```

---

## 28. v0 Exclusions

These are not valid v0:

```text
classes
structs
methods
interfaces
traits
inheritance
anonymous functions
closures
try/catch
networking
package manager
manual memory syntax
```

Example invalid OOP:

```walk
class: User # error
```

Example invalid exception syntax:

```walk
try:
    out: 'x'
catch:
    out: 'bad'
```

---

## 29. Minimal Valid Program

```walk
out: 'hello'
```

---

## 30. Representative Program

```walk
imp: math

func: hyp(a float, b float) float
    return: math.sqrt(+ (^ a 2) (^ b 2))

var: h = hyp(3, 4)

if: == h 5
    out: 'ok'
else:
    out: 'bad'
```

Expected output:

```text
ok
```
