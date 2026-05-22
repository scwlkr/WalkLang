# SYNTAX.md

## WalkLang Syntax

Draft: v0

---

## 1. Files

WalkLang files use `.walk`.

```text
main.walk
math.walk
tests.walk
```

---

## 2. Comments

Use `#`.

```walk
# full-line comment
var: x = 5 # inline comment
```

---

## 3. Blocks

Blocks use indentation.

```walk
if: true
    out: 'inside'
out: 'outside'
```

Formatter emits 4 spaces.

Tabs are invalid in v0.

```walk
if: true
	out: 'bad' # error: tab indentation
```

---

## 4. Statements

One statement per line.

```walk
var: x = 1
out: x
```

No semicolons.

```walk
var: x = 1; out: x # error
```

---

## 5. Commands

Command statements use `keyword:`.

```walk
var: x = 1
const: pi = 3.14
out: x
func: add(a int, b int) int
    return: + a b
```

Command keywords are visually distinct.

```text
var:
func:
if:
while:
```

---

## 6. Names

Names use letters, digits, and `_`.

```walk
var: userName = 'scwlkr'
var: score2 = 100
var: user_name = 'scwlkr'
```

Invalid:

```walk
var: 2score = 100 # error
var: if = 1       # error: reserved word
```

---

## 7. Literals

```walk
var: i = 10
var: f = 3.14
var: yes = true
var: no = false
var: name = 'Walker'
var: none string? = null
```

Strings use single quotes.

```walk
var: msg = 'don\'t stop'
```

Double quotes are invalid in v0.

```walk
var: msg = "hello" # error
```

---

## 8. Variables

Create:

```walk
var: name = value
```

Example:

```walk
var: count = 0
count = + count 1
```

With explicit type:

```walk
var: count int = 0
var: price float = 9.99
```

Type cannot change:

```walk
var: count = 0
count = 'zero' # error
```

---

## 9. Constants

Constants use `const:`.

```walk
const: max = 10
const: app_name = 'WalkLang'
```

Cannot reassign:

```walk
const: max = 10
max = 20 # error
```

Const arrays are read-only through that name:

```walk
const: nums = [1, 2, 3]
nums[0] = 9 # error
```

---

## 10. Output

Use `out:`.

```walk
out: 'hello'
out: count
out: + a b
```

Block form supports long expressions.

```walk
out:
    +:
        subtotal
        tax
        fee
```

---

## 11. Prefix Math

Math is prefix only.

```walk
+ a b
- a b
* a b
/ a b
^ a b
```

Examples:

```walk
var: sum = + a b
var: diff = - a b
var: product = * a b
var: q = / 5 2
var: sq = ^ x 2
```

Invalid infix:

```walk
var: x = a + b # error
```

---

## 12. Operator Arity

```text
+   2+ args
*   2+ args
-   2 args
/   2 args
^   2 args
```

Valid:

```walk
var: total = + a b c d
var: product = * a b c
var: diff = - a b
```

Invalid:

```walk
var: x = - a b c # error
```

---

## 13. Grouping

Use `()`.

```walk
var: x = * (+ a b) (- c d)
```

Means:

```text
(a + b) * (c - d)
```

No precedence guessing.

---

## 14. Operator Blocks

Long prefix expressions may use `operator:` blocks.

```walk
var: result = /:
    * (+ a b) (^ (- c d) 2)
    - e a
```

Means:

```text
((a + b) * ((c - d) ^ 2)) / (e - a)
```

---

## 15. Negative Values

No space means negation.

```walk
var: a = -4
var: b = -a
var: c = -(+ a b)
```

Space means subtraction.

```walk
var: d = - a b
```

Invalid:

```walk
var: x = - (+ a b) # error
```

Use:

```walk
var: x = -(+ a b)
```

---

## 16. Comparisons

Comparisons are prefix.

```walk
> a b
< a b
>= a b
<= a b
== a b
!= a b
```

Example:

```walk
if: > score 90
    out: 'passed'
```

---

## 17. Boolean Logic

```walk
and a b
or a b
not a
```

Example:

```walk
if: and (> age 18) (< age 65)
    out: 'valid'
```

---

## 18. If / Else

```walk
if: condition
    statement
else:
    statement
```

Example:

```walk
if: == name 'Walker'
    out: 'hello'
else:
    out: 'unknown'
```

Condition must be bool.

```walk
if: 1
    out: 'bad' # error
```

---

## 19. While

```walk
var: count = 0

while: < count 3
    out: count
    count = + count 1
```

---

## 20. Repeat

```walk
repeat: 3
    out: 'again'
```

Count must be `int >= 0`.

```walk
repeat: -1
    out: 'bad' # error
```

---

## 21. For

For iterates arrays.

```walk
var: nums = [1, 2, 3]

for: n in nums
    out: n
```

---

## 22. Break / Continue

```walk
while: true
    break:
```

```walk
for: n in nums
    if: == n 0
        continue:
    out: n
```

Only valid inside loops.

```walk
break: # error at top level
```

---

## 23. Functions

Define:

```walk
func: name(param type, param type) return_type
    return: value
```

Example:

```walk
func: add(a int, b int) int
    return: + a b
```

Call:

```walk
var: result = add(5, 10)
```

Types may be omitted only when inference is unambiguous.

```walk
func: double(x)
    return: * x 2

out: double(4)
```

Ambiguous inference is an error.

---

## 24. Return

```walk
return: value
```

Example:

```walk
func: square(x int) int
    return: * x x
```

All paths must return for non-null return types.

```walk
func: bad(x int) int
    if: > x 0
        return: x
    # error: missing return
```

---

## 25. Recursion

Allowed.

```walk
func: fact(n int) int
    if: <= n 1
        return: 1
    return: * n fact(- n 1)
```

---

## 26. Function Values

Named functions can be passed.

```walk
func: inc(x int) int
    return: + x 1

func: apply(f func(int) int, x int) int
    return: f(x)

out: apply(inc, 4)
```

No anonymous functions in v0.

```walk
apply(func(x) return + x 1, 4) # error
```

No closures in v0.

---

## 27. Arrays

Arrays use `[]` and commas.

```walk
var: nums = [1, 2, 3]
var: names = ['a', 'b', 'c']
```

Arrays are homogeneous.

```walk
var: bad = [1, 'a'] # error
```

---

## 28. Indexing

Indexes are zero-based.

```walk
var: nums = [10, 20, 30]

out: nums[0]
nums[1] = 99
```

Element type is locked.

```walk
nums[0] = 'x' # error
```

---

## 29. Matrices

A matrix is an array of arrays.

```walk
var: grid = [
    [1, 3],
    [2, 4]
]
```

Index twice:

```walk
out: grid[0][1] # 3
```

---

## 30. Null

`null` requires a nullable type.

```walk
var: email string? = null
email = 'a@b.com'
```

Non-null types reject null.

```walk
var: name string = null # error
```

---

## 31. Imports

Use `imp:`.

```walk
imp: math

out: math.sqrt(9)
```

Imports are namespaced.

```walk
sqrt(9)      # error unless locally defined
math.sqrt(9) # ok
```

---

## 32. Exports

Use `exp:`.

```walk
func: square(x int) int
    return: * x x

exp: square
```

Then:

```walk
imp: calc

out: calc.square(5)
```

In v1, `calc` resolves to a sibling `calc.walk` file. Only names listed with `exp:` are public through the namespace.

---

## 33. Tests

Testing syntax is v0.1 target.

```walk
test: 'add works'
    assert: == add(2, 3) 5
```

---

## 34. Reserved Words

```text
var
const
out
if
else
while
for
repeat
break
continue
func
return
imp
exp
true
false
null
and
or
not
in
test
assert
```

---

## 35. Complete Example

```walk
# main.walk

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
