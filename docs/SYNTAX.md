# WalkLang v1 Syntax Guide

This guide is the readable syntax companion to `docs/SPEC.md`. `SPEC.md` is the contract when the two disagree.

---

## Files

WalkLang files use `.walk`.

```text
main.walk
calc.walk
tests.walk
```

Comments use `#` outside strings.

```walk
# full-line comment
var: x = 5 # inline comment
```

---

## Blocks

Blocks are indentation-based.

```walk
if: true
    out: 'inside'
out: 'outside'
```

Tabs are invalid. `walk fmt` emits 4 spaces for each block level.

---

## Commands

Command statements use `keyword:`.

```walk
var: x = 1
const: limit = 10
out: x
```

Long expressions can use a block after a command.

```walk
out:
    +:
        1
        2
        3
```

---

## Names

Names use letters, digits, and `_`, and cannot be reserved words.

```walk
var: user_name = 'Walker'
var: score2 = 100
```

Invalid:

```walk
var: if = 1
var: 2score = 100
```

---

## Literals

```walk
var: i = 10
var: f = 3.14
var: yes = true
var: no = false
var: name = 'Walker'
var: optional_name string? = null
```

Strings are single-quoted.

```walk
var: msg = 'don\'t stop'
```

---

## Variables And Constants

`var:` creates a mutable binding.

```walk
var: count = 0
count = + count 1
```

`const:` creates an immutable binding.

```walk
const: max = 10
```

Type annotations go between the name and `=`.

```walk
var: count int = 0
var: price float = 9
var: name string? = null
```

---

## Output

Use `out:` to print a scalar value.

```walk
out: 'hello'
out: + 1 2
out: true
```

Arrays and function values cannot be printed directly.

---

## Prefix Math

Math is prefix only.

```walk
+ a b
- a b
* a b
/ a b
^ a b
```

`+` and `*` accept 2 or more operands.

```walk
var: total = + a b c
var: product = * a b c
```

`-`, `/`, and `^` accept exactly 2 operands.

```walk
var: diff = - a b
var: ratio = / 5 2
var: square = ^ x 2
```

Negative numeric literals are valid.

```walk
var: x = -4
```

Use subtraction from zero to negate a name.

```walk
var: y = - 0 x
```

---

## Grouping

Use parentheses when an expression must be one operand.

```walk
var: x = * (+ a b) (- c d)
```

There is no infix precedence.

---

## Operator Blocks

Long prefix expressions may use `operator:` blocks.

```walk
var: total =
    +:
        subtotal
        tax
        fee
```

---

## Comparisons And Boolean Logic

```walk
> a b
< a b
>= a b
<= a b
== a b
!= a b
and a b
or a b
not a
```

Example:

```walk
if: and (> age 18) (< age 65)
    out: 'working age'
```

---

## If / Else

```walk
if: condition
    statement
else:
    statement
```

The condition must be bool.

---

## Loops

`while:` loops while a bool condition is true.

```walk
var: count = 0

while: < count 3
    out: count
    count = + count 1
```

`repeat:` loops an int count.

```walk
repeat: 3
    out: 'again'
```

`for:` iterates arrays.

```walk
var: nums = [1, 2, 3]

for: n in nums
    out: n
```

`break:` and `continue:` are valid only inside loops.

---

## Functions

Parameters must have types.

```walk
func: add(a int, b int) int
    return: + a b
```

Omitting the return type makes the function `void`.

```walk
func: say(message string)
    out: message
```

Non-void functions must return on all paths.

---

## Function Values

Named functions can be passed to typed function parameters.

```walk
func: inc(x int) int
    return: + x 1

func: apply(f func(int) int, x int) int
    return: f(x)

out: apply(inc, 4)
```

Anonymous functions and closures are not v1.1 syntax.

---

## Arrays

Arrays use brackets and commas.

```walk
var: nums = [1, 2, 3]
var: names = ['a', 'b', 'c']
```

Arrays must be homogeneous and non-empty.

```walk
nums[1] = 99
out: nums[0]
```

Stable native element types are `int`, `float`, `bool`, and `string`.

---

## Null

Use nullable string annotations when assigning `null`.

```walk
var: email string? = null
email = 'a@b.com'

if: != email null
    out: email
```

---

## Imports And Exports

Use `imp:` to import built-in modules or sibling user modules.

```walk
imp: math
out: math.sqrt(9)
```

User module example:

```walk
# calc.walk
func: square(x int) int
    return: * x x

exp: square
```

```walk
# main.walk
imp: calc
out: calc.square(5)
```

Only names listed with `exp:` are public through the namespace.

---

## Tests

Use `test:` and `assert:` with `walk test`.

```walk
func: add(a int, b int) int
    return: + a b

test: 'add works'
    assert: == add(2, 3) 5
```

`assert:` requires a bool expression.

`testing.assert(bool)` can wrap that bool expression when you want a namespaced stdlib assertion helper.

```walk
imp: testing

test: 'wrapped assertion works'
    assert: testing.assert(true)
```

---

## Reserved Words

```text
var const out if else while for repeat break continue
func return imp exp true false null and or not in test assert
```

---

## Complete Example

```walk
imp: math

func: distance(x1 float, y1 float, x2 float, y2 float) float
    return:
        math.sqrt(
            +:
                ^ (- x2 x1) 2
                ^ (- y2 y1) 2
        )

var: d = distance(0, 0, 3, 4)

if: == d 5
    out: 'distance is 5'
else:
    out: 'distance is not 5'
```
