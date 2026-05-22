# WalkLang v1 Specification

This document is the stable WalkLang language contract for the v1 line. v1.3 adds compatible standard library APIs without changing the core syntax.

Core rule:

```text
If it is not in SPEC.md, it is not stable WalkLang.
If the compiler disagrees with SPEC.md, either the compiler or the spec must change.
```

---

## 1. Source Files

WalkLang source files are UTF-8 text files ending in `.walk`.

```text
main.walk
```

A source file contains zero or more statements. Blank lines are ignored. `#` starts a comment outside strings.

```walk
# comment
var: x = 1 # inline comment
out: x
```

---

## 2. Compilation Contract

The stable compiler pipeline is:

```text
.walk source -> lexer -> parser -> AST -> type checker -> C emitter -> native executable
```

The `walk build` command writes generated C next to the executable path unless `--emit-c` chooses another path. Native builds use `cc` by default and link with `-lm`.

---

## 3. Blocks And Indentation

Indentation owns blocks.

```walk
if: true
    out: 'inside'
out: 'outside'
```

Rules:

- spaces define indentation
- tabs are invalid
- a greater indentation level starts a child block
- a lower indentation level closes blocks
- formatter output uses 4 spaces per block level

---

## 4. Lexical Rules

Stable tokens:

```text
names       letters, digits, and _
numbers     int and float literals
strings     single-quoted strings
symbols     ( ) [ ] , : = + - * / ^ > < ? .
comments    # outside strings
```

Strings support these escapes:

```text
\' single quote
\\ backslash
\n newline
\t tab
```

Double-quoted strings are not valid WalkLang.

---

## 5. Statements

Stable statements:

```text
imp:
exp:
var:
const:
assignment
out:
test:
assert:
func:
return:
if:
else:
while:
repeat:
for:
break:
continue:
```

One statement appears on each physical line. Semicolons are not part of v1.1 syntax.

---

## 6. Reserved Words

These words cannot be user-defined names:

```text
var const out if else while for repeat break continue
func return imp exp true false null and or not in test assert
```

---

## 7. Types

Stable value types:

```text
int
float
bool
string
array[T]
func(T...) R
```

`void` is used internally for functions with no return type. It is not a value type.

`null` is stable for nullable strings in native v1.1 programs:

```walk
var: name string? = null
```

Other nullable scalar forms are not part of the v1.1 stable native contract.

---

## 8. Type Inference And Type Lock

`var:` and `const:` infer a type from their initializer unless an explicit type annotation is present.

```walk
var: x = 1
var: y float = 1
const: name = 'Walker'
```

Once a name is declared, its type is locked.

```walk
var: x = 1
x = 2      # ok
x = 'two'  # type error
```

`int` values may initialize or assign to `float` values. Other implicit conversions are not stable.

---

## 9. Variables And Constants

`var:` creates a mutable binding.

```walk
var: count = 0
count = + count 1
```

`const:` creates an immutable binding.

```walk
const: limit = 10
```

Reassigning a `const:` name is a type error. Assigning through an indexed target rooted at a `const:` array is also a type error.

---

## 10. Output

`out:` writes one value and a trailing newline.

```walk
out: 'hello'
out: + 1 2
```

Stable output types:

```text
int
float
bool
string
nullable string
```

Arrays, functions, and void values cannot be output.

---

## 11. Expressions

Stable expressions:

```text
literals
names
prefix operators
grouped expressions
function calls
qualified module calls
array literals
index expressions
block expressions under commands
```

Grouping uses parentheses:

```walk
var: x = * (+ 1 2) (- 9 4)
```

---

## 12. Operators

Numeric operators:

```text
+  2 or more args
*  2 or more args
-  exactly 2 args
/  exactly 2 args, returns float
^  exactly 2 args
```

Comparison operators:

```text
> < >= <= == !=
```

Boolean operators:

```text
and  2 or more bool args
or   2 or more bool args
not  exactly 1 bool arg
```

Negative numeric literals are supported:

```walk
var: x = -4
```

Unary negation of variables is not v1.1 syntax. Use subtraction from zero:

```walk
var: y = - 0 x
```

---

## 13. Arrays

Array literals are homogeneous and non-empty.

```walk
var: nums = [1, 2, 3]
var: words = ['a', 'b']
```

Stable native array element types:

```text
int
float
bool
string
```

Indexing is zero-based.

```walk
out: nums[0]
nums[1] = 9
```

Empty arrays and nested array emission are not stable v1.1 native features.

---

## 14. Functions

Function declarations use typed parameters.

```walk
func: add(a int, b int) int
    return: + a b
```

If a return type is omitted, the function returns `void` and should end normally.

```walk
func: say(message string)
    out: message
```

`return:` requires a value and is valid only inside a function with a compatible return type.

Non-void functions must return on all paths.

---

## 15. Function Values

Named functions may be passed as values.

```walk
func: inc(x int) int
    return: + x 1

func: apply(f func(int) int, x int) int
    return: f(x)

out: apply(inc, 4)
```

Anonymous functions and closures are not v1.1 syntax.

---

## 16. Control Flow

`if:` conditions must be `bool`.

```walk
if: > age 18
    out: 'adult'
else:
    out: 'minor'
```

`while:` conditions must be `bool`.

```walk
while: < count 3
    count = + count 1
```

`repeat:` counts must be `int`.

```walk
repeat: 3
    out: 'again'
```

`for:` iterates arrays.

```walk
for: n in nums
    out: n
```

`break:` and `continue:` are valid only inside loops.

---

## 17. Modules

Built-in modules:

```text
math
string
array
time
random
testing
```

User modules are sibling `.walk` files imported by bare module name.

```walk
imp: calc
out: calc.square(5)
```

`calc` resolves to `calc.walk` in the importing file's directory.

User module rules:

- imported calls stay namespaced
- only functions listed with `exp:` are callable from another file
- module files may contain only `imp:`, `func:`, and `exp:` at top level
- import cycles are errors

---

## 18. Tests

`test:` defines a test block for `walk test`.

```walk
test: 'add works'
    assert: == add(2, 3) 5
```

`assert:` requires a bool expression. Failed assertions make the generated test executable exit non-zero.

`testing.assert(bool)` is a stable v1.3 stdlib helper that returns its bool argument unchanged. It is intended to be paired with `assert:`:

```walk
imp: testing

test: 'wrapped assertion works'
    assert: testing.assert(true)
```

Normal `walk build` ignores `test:` blocks.

---

## 19. Formatter

`walk fmt` emits stable spacing and indentation:

```walk
var:x=+ 1 2
```

becomes:

```walk
var: x = + 1 2
```

Formatter output uses 4 spaces for indentation.

---

## 20. Diagnostics

Compiler diagnostics use this shape:

```text
file.walk:line:column: category: message
```

Stable categories:

```text
syntax error
type error
name error
module error
warning
internal error
```

Warnings do not fail by default. `--warnings=error` promotes warnings to errors.

---

## 21. Non-Goals

These are not stable v1 features:

```text
classes
structs
methods
inheritance
interfaces
traits
anonymous functions
closures
try/catch
networking
package manager
debugger
full LSP
file/json/matrix stdlib APIs
empty arrays
nested arrays in native output
```

---

## 22. Minimal Valid Program

```walk
out: 'hello'
```

---

## 23. Representative v1.1 Program

```walk
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
