# WalkLang v1.5 Standard Library

This document lists the stable built-in modules and functions for the v1 line. These APIs became stable in v1.3 and remain compatibility-protected in v1.5.

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
testing
```

No other built-in module is stable in v1.5.

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

Returns the length stored with a stable v1 array.

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

Returns an integer in the inclusive range. If `max < min`, v1 returns `min`.

`random.int` uses the native C runtime's default `rand()` state. v1 does not expose seeding.

```walk
imp: random
out: random.int(1, 10)
```

---

## testing

### testing.assert(bool) -> bool

Returns the bool argument unchanged. Use it with `assert:` when a test wants the assertion helper to be visibly namespaced.

```walk
imp: testing

test: 'works'
    assert: testing.assert(true)
```

`testing.assert` itself does not print or stop a program. The `assert:` statement owns test failure reporting.

---

## Test Syntax

The stable testing surface also includes syntax:

```walk
test: 'works'
    assert: true
```

Run it with:

```bash
walk test tests.walk
```

---

## Draft APIs

These names are planned draft APIs only. They are documented here so naming can stay consistent, but they are not stable, not importable, and not compatibility-protected in v1.5.

```text
file.read
file.write
file.exists
json.parse
json.stringify
matrix.rows
matrix.cols
matrix.get
```
