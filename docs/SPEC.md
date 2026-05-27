# WalkLang Specification

This document is the stable WalkLang feature specification for the current
project version. It is normative and intentionally concise. `docs/SYNTAX.md`,
`docs/STDLIB.md`, `docs/ERRORS.md`, and `docs/COMPATIBILITY.md` provide
readable examples and longer explanations.

Core rule:

```text
If it is not in SPEC.md, it is not stable WalkLang.
If the compiler disagrees with SPEC.md, either the compiler or the spec must change.
```

Draft, experimental, planned, deprecated, and removed behavior must be labeled
with the stability words defined in `docs/LANGUAGE_CONCEPTS.md`.

---

## 1. Lexical Structure

A source file is UTF-8 text ending in `.walk`. A source file contains zero or
more statements. Blank lines are ignored. Comments start with `#` outside
strings and are not statements.

WalkLang statement structure is line-based. Semicolons are not statement
separators.

Stable token classes:

```text
name      letters, digits, and _
number    int and float literals
string    single-quoted string literals
symbol    ( ) [ ] , : = + - * / ^ > < ? .
```

Names cannot start with a digit and cannot be reserved keywords.

Reserved keywords:

```text
var const out if else while for repeat break continue
func return imp exp true false null and or not in test assert
```

Stable strings are single-quoted. Stable string escapes:

```text
\' single quote
\\ backslash
\n newline
\t tab
```

Stable string interpolation uses `{expression}` inside a single-quoted string.
Interpolation accepts display expressions of type `int`, `float`, `bool`,
`string`, and nullable string. Doubled braces emit literal braces.

```walk
out: 'score {score}'
out: '{{literal}}'
```

Double-quoted strings are not valid WalkLang.

Blocks are indentation-based. Spaces define indentation, tabs are invalid, a
greater indentation level starts a child block, and a lower indentation level
closes blocks. `walk fmt` emits 4 spaces per block level.

---

## 2. Types

Stable value types:

```text
int
float
bool
string
string?
array[T]
func(T...) R
```

`void` is an internal function return marker for functions with no return
value. It is not a value type.

Stable native array element types are:

```text
int
float
bool
string
```

Struct and generic types are implemented as experimental features. They are not
part of the stable feature set.

The current compiler also includes the draft collection type
`map[string]array[string]` for explicit string-keyed tables of string arrays.
It is not part of the stable feature set.

---

## 3. Values

Stable literals create values of these types:

```text
int
float
bool
string
null
```

`null` is stable for nullable strings:

```walk
var: name string? = null
```

Other nullable scalar forms are not part of the stable native feature set.

Array literals are homogeneous. Empty arrays need an explicit array annotation.
The draft `map[string]array[string]` type accepts an empty `[]` literal only
when an explicit map annotation is present.

```walk
var: nums = [1, 2, 3]
var: guessed array[string] = []
var: table map[string]array[string] = []
```

Runtime-created strings from stable `in:` and string interpolation are owned by
the native program.

---

## 4. Expressions

Stable expressions:

```text
literals
names
in:
prefix operators
grouped expressions
function calls
qualified module calls
array literals
index expressions
block expressions under commands
```

`in:` reads one required line from stdin and returns a `string`.

```walk
var: name = in:
var: name = in: 'Name? '
```

The optional prompt expression must be `string`. It is written to stdout without
a newline, and stdout is flushed before reading.

`in:` reads from stdin only, consumes one line, strips the final LF or CRLF line
ending, preserves all other whitespace, returns `''` for an empty line, accepts
final unterminated input as a line, has no language-level line length limit, and
runtime-stops on immediate EOF, stdin read failure, or allocation failure.

Index expressions work on arrays and strings.

```walk
var: nums = [1, 2, 3]
out: nums[0]
out: 'walk'[1]
```

String indexing returns a one-character string by zero-based byte index and
runtime-stops when the index is out of range.

Grouping uses parentheses:

```walk
var: x = * (+ 1 2) (- 9 4)
```

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

Negative numeric literals are stable. Unary negation of names is not stable
syntax; use subtraction from zero.

```walk
var: x = -4
var: y = - 0 x
```

Typed input is not part of `in:`. Read text first, then parse explicitly with
draft parse APIs when using the current compiler's draft surface.

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

`out:` writes one stable scalar value and a trailing newline. Stable output
types are `int`, `float`, `bool`, `string`, and nullable string. Arrays,
functions, and `void` values cannot be output directly.

`var:` creates a mutable binding. `const:` creates an immutable binding.
`var:` and `const:` infer a type from their initializer unless an explicit type
annotation is present. Once a name is declared, its type is locked.

```walk
var: x = 1
x = 2
const: limit = 10
```

`int` values may initialize or assign to `float` values. Other implicit
conversions are not stable.

Reassigning a `const:` name is a type error. Assigning through an indexed target
rooted at a `const:` array is also a type error.

`if:` and `while:` conditions must be `bool`. `repeat:` counts must be `int`.
`for:` iterates arrays. `break:` and `continue:` are valid only inside loops.

```walk
for: n in nums
    out: n
```

`test:` defines a test block for `walk test`. `assert:` requires a bool
expression. Failed assertions make the generated test executable exit non-zero.
Normal `walk build` ignores `test:` blocks.

The current compiler also implements draft `do:` effect calls and draft
`defer:` scope cleanup. They are not part of the stable feature set.

---

## 6. Declarations

Stable declarations introduce or expose named surface:

```text
imp:
exp:
var:
const:
func:
test:
```

`imp:` imports a built-in module, sibling user module, or package module
namespace. `exp:` exposes an existing module symbol.

User modules are `.walk` files imported by module name. A bare module import
such as `imp: calc` resolves to `calc.walk` in the importing file's directory.
Package module imports use dotted names resolved through project/package rules.

User module rules:

```text
imported calls stay namespaced
only functions listed with exp: are callable from another file
module files may contain only imp:, struct:, func:, and exp: at top level
import cycles are errors
```

`struct:` declarations are implemented as an experimental feature and are
allowed in module files by the current compiler, but structs are not part of
the stable feature set.

---

## 7. Functions

Stable functions are named reusable blocks declared with `func:`.

```walk
func: add(a int, b int) int
    return: + a b
```

Stable functions may use typed parameters and a typed return. Obvious local
functions may omit parameter and return types when the body proves the types
clearly. Whole-number arithmetic infers `int`; float contexts infer `float`;
boolean contexts infer `bool`.

```walk
func: power_four(n)
    return: ^ n 4
```

If a parameter type cannot be inferred from the function body, the parameter
needs an explicit annotation. Function types are not inferred from later call
sites.

If a return type is omitted and the function has no value return, the function
returns `void` and should end normally.

`return:` requires a value and is valid only inside a function with a compatible
return type. Non-void functions must return on all paths.

Named functions may be passed as values to typed function parameters.

```walk
func: inc(x int) int
    return: + x 1

func: apply(f func(int) int, x int) int
    return: f(x)
```

Anonymous functions and closures are not stable syntax.

---

## 8. Modules

Stable built-in modules:

```text
math
string
array
time
random
testing
```

Stable built-in functions:

```text
math.sqrt(number) -> float
math.pow(number, number) -> float
string.len(string) -> int
string.at(string, int) -> string
string.contains(string, string) -> bool
string.concat(string, string) -> string
string.lower(string) -> string
string.split(string, string) -> array[string]
string.replace(string, string, string) -> string
array.len(array[T]) -> int
array.contains(array[T], T) -> bool
array.push(array[T], T) -> array[T]
time.now() -> int
random.int(int, int) -> int
random.choice(array[T]) -> T
testing.assert(bool) -> bool
```

`array.contains`, `array.push`, and `random.choice` are stable for arrays whose
elements are `int`, `float`, `bool`, or `string`.

The current compiler also includes draft built-in modules:

```text
io
parse
process
file
dir
path
json
map
term
http
html
```

Draft modules are importable for experimentation, but they are not
compatibility-protected by the stable feature set. See `docs/STDLIB.md` for
their signatures, effect status, result structs, runtime behavior, and failure
behavior.

---

## 9. Errors

Use `diagnostic` for compiler-reported errors and warnings. Compiler
diagnostics use this stable first-line shape:

```text
file.walk:line:column: category: message
```

The command-line display may add a source snippet, caret, and focused
suggestion under that stable first line.

Stable diagnostic categories:

```text
syntax error
type error
name error
module error
warning
internal error
```

Warnings do not fail by default. `--warnings=error` promotes warnings to
errors. Stable warnings cover shadowing an outer name and unreachable statements
after `return:`, `break:`, or `continue`.

Stable runtime failures include:

```text
walk runtime error: input reached EOF
walk runtime error: stdin read failed
walk runtime error: out of memory
walk runtime error: format failed
walk runtime error: string index out of range
walk runtime error: random.choice on empty array
```

Failed assertions and draft recoverable result data are not compiler
diagnostics. Draft runtime failures are documented with their draft APIs in
`docs/STDLIB.md`.

---

## 10. Standard Library

The stable standard library is the set of stable built-in modules and functions
listed in this specification and documented in `docs/STDLIB.md`.

A stable standard library API must name:

```text
module
function
parameter types
return type
effect status
runtime behavior
failure behavior
```

No other built-in module is stable.

---

## Non-Stable Current Compiler Surface

The current compiler accepts experimental data-modeling features and draft
IO/runtime APIs that are not stable behavior.

Experimental language surface:

```text
structs
methods
generic functions
```

Draft statement/API surface:

```text
do:
defer:
io
parse
process
file
dir
path
json
map
term
http
html
```

Planned or not-current stable features include:

```text
classes
inheritance
interfaces
traits
anonymous functions
closures
try/catch
networking server/runtime APIs
debugger
full LSP
matrix stdlib APIs
nested arrays in native output
```

---

## Minimal Valid Program

```walk
out: 'hello'
```
