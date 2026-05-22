# WalkLang v1.9 Standard Library

This document lists the stable built-in modules and functions for the v1 line. These APIs are compatibility-protected in v1.9.

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

No other built-in module is stable in v1.9.

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

### string.at(string, int) -> string

Returns the one-character string at a zero-based byte index. Out-of-range indexes runtime-stop.

```walk
imp: string
out: string.at('walk', 1)
```

String indexing is equivalent:

```walk
out: 'walk'[1]
```

### string.contains(string, string) -> bool

Returns `true` when the second string appears inside the first. An empty search string returns `true`.

```walk
imp: string
out: string.contains('walk', 'al')
```

### string.concat(string, string) -> string

Returns a new string made from the left string followed by the right string.

```walk
imp: string
out: string.concat('walk', 'lang')
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

### array.contains(array[T], T) -> bool

Returns `true` when a stable native array contains an equal item. Supported stable element types are `int`, `float`, `bool`, and `string`.

```walk
imp: array
var: letters = ['w', 'a']
out: array.contains(letters, 'w')
```

### array.push(array[T], T) -> array[T]

Returns a new array with the item appended. `array.push` does not mutate the input array in place.

```walk
imp: array
var: guessed array[string] = []
guessed = array.push(guessed, 'w')
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

`random.int` and `random.choice` use a runtime-owned PRNG seeded once per native process. v1 does not expose manual seeding.

```walk
imp: random
out: random.int(1, 10)
```

### random.choice(array[T]) -> T

Returns one item from a non-empty stable native array. Calling `random.choice` on an empty array runtime-stops.

```walk
imp: random
var: words = ['dog', 'cat']
out: random.choice(words)
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

These names are planned draft APIs only. They are documented here so naming can stay consistent, but they are not stable, not importable, and not compatibility-protected in v1.9.

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
