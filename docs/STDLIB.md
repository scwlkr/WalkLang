# WalkLang v1.1 Standard Library

This document lists the stable built-in modules and functions in v1.1.

Import built-ins with `imp:` and call functions through their module namespace.

```walk
imp: math
out: math.sqrt(9)
```

---

## Stable Modules

```text
math
string
array
time
random
```

No other built-in module is stable in v1.1.

---

## math

### math.sqrt(number) -> float

Accepts `int` or `float`.

```walk
imp: math
out: math.sqrt(9)
```

### math.pow(number, number) -> float

Accepts `int` or `float` arguments.

```walk
imp: math
out: math.pow(2, 3)
```

---

## string

### string.len(string) -> int

Returns the byte length of a string as emitted through C `strlen`.

```walk
imp: string
out: string.len('walk')
```

---

## array

### array.len(array[T]) -> int

Returns the length stored with a stable v1.1 array.

```walk
imp: array
var: nums = [1, 2, 3]
out: array.len(nums)
```

---

## time

### time.now() -> int

Returns the current Unix timestamp in seconds from the native C runtime.

```walk
imp: time
out: > time.now() 0
```

---

## random

### random.int(int, int) -> int

Returns an integer in the inclusive range. If `max < min`, v1.1 returns `min`.

```walk
imp: random
out: random.int(1, 10)
```

---

## Testing Base

The stable testing surface is syntax, not an importable module:

```walk
test: 'works'
    assert: true
```

Run it with:

```bash
walk test tests.walk
```

---

## Not Stable In v1.1

These names are planned or draft ideas only:

```text
file.read
file.write
file.exists
json.parse
json.stringify
matrix.rows
matrix.cols
matrix.get
testing.assert as an imported function
```
