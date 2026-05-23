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

The current compiler also includes draft `io`, `parse`, `process`, and `file`
modules. They are importable and tested, but they are not
compatibility-protected in v1.9.

Draft APIs may expose draft result structs. They are documented here so current
compiler behavior is visible, but they are not part of the stable v1.9
compatibility contract.

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

## Draft io

Use draft `io` output functions with `do:` because they are effects. Draft
`io` read functions are expressions because they return result values.

```walk
imp: io

do: io.write('Loading')
do: io.write_line('done')
do: io.error_line('warning')
```

Recoverable draft reads return:

```walk
struct: IOReadResult
    ok bool
    value string
    error string
```

`ok` is true when `value` contains text. `error` is `''` on success. Immediate
EOF from `io.read_line()` returns `ok false`, `value ''`, and `error 'eof'`.
Stdin read failure returns `error 'stdin read failed'`. Allocation failure still
runtime-stops because the program cannot reliably recover without memory.

### io.write(string) -> effect

Writes the string to stdout without adding a newline and flushes stdout.

### io.write_line(string) -> effect

Writes the string and a trailing newline to stdout and flushes stdout.

### io.error_line(string) -> effect

Writes the string and a trailing newline to stderr and flushes stderr.

### io.read_line() -> IOReadResult

Reads one line from stdin. A trailing LF or CRLF line ending is stripped. A final
unterminated line succeeds. Immediate EOF is returned as data with
`error 'eof'`.

### io.read_all() -> IOReadResult

Reads the remaining stdin as one runtime-owned string. EOF is a successful empty
or completed read. Stdin read failure is returned as data.

---

## Draft parse

```walk
imp: parse

var: age = parse.int('41')
if: age.ok
    out: age.value
```

Parse helpers use draft result structs:

```walk
struct: ParseIntResult
    ok bool
    value int
    error string

struct: ParseFloatResult
    ok bool
    value float
    error string

struct: ParseBoolResult
    ok bool
    value bool
    error string
```

The helpers parse the whole input string. Extra characters and leading or
trailing whitespace make the result fail instead of being ignored.

### parse.int(string) -> ParseIntResult

Parses a base-10 integer with an optional leading sign. Invalid input returns
`error 'invalid int'`; overflow returns `error 'int out of range'`.

### parse.float(string) -> ParseFloatResult

Parses a finite floating-point number. Invalid input returns
`error 'invalid float'`; range errors return `error 'float out of range'`.

### parse.bool(string) -> ParseBoolResult

Parses exactly `true` or `false`. Other text returns `error 'invalid bool'`.

---

## Draft file

Draft `file` APIs are UTF-8 text helpers. They use native process paths:
relative paths resolve against the current working directory, absolute paths
are passed to the host OS, and this draft slice does not normalize paths or
expand `~`. Empty paths runtime-stop for reads and writes.

`file.read` and `file.write` are fail-stop in this draft slice. Missing files,
permission errors, invalid UTF-8, embedded null bytes on read, and write
failures stop the native program with a `walk runtime error`.

### file.read(string) -> string

Reads the whole UTF-8 text file into a runtime-owned string.

### file.write(string, string) -> effect

Overwrites the target path with UTF-8 text. Create parent directories first;
this helper does not create them.

```walk
imp: file

do: file.write('note.txt', 'hello')
out: file.read('note.txt')
```

### file.exists(string) -> bool

Returns `true` when the path currently exists according to the host OS and
`false` when it does not. Empty paths return `false`.

---

## Draft process

```walk
imp: process

out: process.arg_count()
out: process.cwd()
do: process.exit(0)
```

### process.args() -> array[string]

Returns command-line arguments passed after the executable path.

### process.arg_count() -> int

Returns the number of command-line arguments passed after the executable path.

### process.env(string) -> string?

Returns an environment variable value, or `null` when the variable is not set.

### process.cwd() -> string

Returns the current working directory as a runtime-owned string.

### process.exit(int) -> effect

Exits the native process with the given status code.

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

These names are planned draft APIs only. They are documented here so naming can
stay consistent, but they are not stable, not importable, and not
compatibility-protected in v1.9.

```text
file.append
json.parse
json.stringify
matrix.rows
matrix.cols
matrix.get
```
