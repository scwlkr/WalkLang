# WalkLang Standard Library

This document lists the stable built-in modules and functions for the current
project version. These APIs are compatibility-protected as stable features.

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

No other built-in module is stable.

The current compiler also includes draft `io`, `parse`, `process`, `file`,
`dir`, `path`, `json`, `map`, `term`, `http`, and `html` modules. They are
importable and tested, but they are not compatibility-protected as stable
features.

Draft APIs may expose draft result structs. They are documented here so current
compiler behavior is visible, but they are not part of the stable feature set.

Draft runtime-module proof surface: `tests/runtime_modules/` records native
fixtures for `io`, `parse`, `process`, `file`, `dir`, `path`, `json`, `map`,
`term`, `http`, and `html`. `tests/conformance/run.sh --runtime-modules` preserved
Go-reference parity before removal, and `make conformance` now verifies the
active C++/C `walk` compiler against the recorded oracle artifacts.

Stable APIs in this document are pure expression calls unless an entry says
otherwise. Invalid argument counts, argument types, or effect placement are
compiler diagnostics. Runtime failures named below are part of the documented
behavior for valid programs at runtime.

Proof surface: stable APIs are covered by `tests/pass/stdlib.walk`,
`tests/pass/walk_tests.walk`, compatibility fixtures, `tests/cpp/`, and
`tests/conformance/`.

---

## math

### math.sqrt(number) -> float

Stability: stable. Effect status: pure expression.

Accepts `int` or `float` and returns the native C `sqrt` result as `float`.
Valid numeric arguments have no WalkLang-defined runtime failure.

```walk
imp: math
out: math.sqrt(9)
```

### math.exp(number) -> float

Stability: stable. Effect status: pure expression.

Accepts `int` or `float` and returns the native C `exp` result as `float`.
Valid numeric arguments have no WalkLang-defined runtime failure.

```walk
imp: math
out: math.exp(1)
```

### math.log(number) -> float

Stability: stable. Effect status: pure expression.

Accepts `int` or `float` and returns the native C natural-log `log` result as
`float`. Valid numeric arguments have no WalkLang-defined runtime failure.

```walk
imp: math
out: math.log(1)
```

### math.pow(number, number) -> float

Stability: stable. Effect status: pure expression.

Accepts `int` or `float` arguments and returns the native C `pow` result as
`float`. Valid numeric arguments have no WalkLang-defined runtime failure.

```walk
imp: math
out: math.pow(2, 3)
```

### math.remainder(int, int) -> int

Stability: stable. Effect status: pure expression.

Returns the integer remainder of `value / divisor`. The divisor must not be
zero. If `divisor` is zero, the program runtime-stops with `walk runtime error:
math.remainder divisor is zero`.

`math.remainder` follows the native C integer remainder rule used by WalkLang's
C backend: division truncates toward zero, and the remainder has the same sign
as `value`.

```walk
imp: math
out: math.remainder(35, 11)
out: math.remainder(-35, 11)
```

---

## string

### string.len(string) -> int

Stability: stable. Effect status: pure expression.

Returns the byte length of a string as emitted through C `strlen`.

```walk
imp: string
out: string.len('walk')
```

### string.at(string, int) -> string

Stability: stable. Effect status: pure expression.

Returns the one-character string at a zero-based byte index. Out-of-range
indexes runtime-stop with `walk runtime error: string index out of range`.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

```walk
imp: string
out: string.at('walk', 1)
```

String indexing is equivalent:

```walk
out: 'walk'[1]
```

### string.contains(string, string) -> bool

Stability: stable. Effect status: pure expression.

Returns `true` when the second string appears inside the first. An empty search string returns `true`.

```walk
imp: string
out: string.contains('walk', 'al')
```

### string.slice(string, int, int) -> string

Stability: stable. Effect status: pure expression.

Returns a new string copied from a zero-based byte `start`, containing at most
`count` bytes. If `start` is at or past the end of the string, the result is
`''`. If `count` extends past the end, the result stops at the end. Negative
`start` runtime-stops with `walk runtime error: string slice start out of
range`; negative `count` runtime-stops with `walk runtime error: string slice
count out of range`. Allocation failure runtime-stops with `walk runtime error:
out of memory`.

`string.slice` is byte-indexed to match `string.len`, `string.at`, and string
indexing. It does not support negative indexes.

```walk
imp: string
out: string.slice('walklang', 4, 99)
```

### string.prefix(string, int) -> string

Stability: stable. Effect status: pure expression.

Returns a new string containing at most the first `count` bytes. If `count`
exceeds the string length, the whole string is copied. Negative `count`
runtime-stops with `walk runtime error: string prefix count out of range`.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

Use `string.prefix` when a program needs a bounded text sample without building
it one character at a time.

```walk
imp: string
out: string.prefix('walklang', 4)
```

### string.concat(string, string) -> string

Stability: stable. Effect status: pure expression.

Returns a new string made from the left string followed by the right string.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

```walk
imp: string
out: string.concat('walk', 'lang')
```

### string.lower(string) -> string

Stability: stable. Effect status: pure expression.

Returns a new string with ASCII `A` through `Z` converted to `a` through `z`.
Non-ASCII bytes and all other characters are left unchanged. Allocation failure
runtime-stops with `walk runtime error: out of memory`.

```walk
imp: string
out: string.lower('Hi WALK')
```

### string.split(string, string) -> array[string]

Stability: stable. Effect status: pure expression.

Returns a new array of string pieces split on the exact separator string.
Splitting on an empty separator returns an array containing the original text.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

```walk
imp: string
var: parts = string.split('a b', ' ')
out: parts[0]
```

### string.join(array[string], string) -> string

Stability: stable. Effect status: pure expression.

Returns a new string made by joining each part with the separator between
items. Joining an empty array returns `''`. Allocation failure runtime-stops
with `walk runtime error: out of memory`.

Use `string.join` when a program has text fragments already separated and wants
one final allocation instead of repeated `string.concat` copies.

```walk
imp: string
var: parts = string.split('walk lang', ' ')
out: string.join(parts, '::')
```

### string.replace(string, string, string) -> string

Stability: stable. Effect status: pure expression.

Returns a new string with every non-overlapping match of `from` replaced by
`to`. Replacing an empty `from` string returns a copy of the original text.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

```walk
imp: string
out: string.replace('banana', 'na', 'NA')
```

---

## array

### array.len(array[T]) -> int

Stability: stable. Effect status: pure expression.

Returns the length stored with a stable array.

```walk
imp: array
var: nums = [1, 2, 3]
out: array.len(nums)
```

### array.contains(array[T], T) -> bool

Stability: stable. Effect status: pure expression.

Returns `true` when a stable native array contains an equal item. Supported stable element types are `int`, `float`, `bool`, and `string`.

```walk
imp: array
var: letters = ['w', 'a']
out: array.contains(letters, 'w')
```

### array.push(array[T], T) -> array[T]

Stability: stable for `int`, `float`, `bool`, and `string` arrays;
experimental for arrays of structs. Effect status: pure expression.

Returns a new array with the item appended. `array.push` does not mutate the input array in place.
Allocation failure runtime-stops with `walk runtime error: out of memory`.

```walk
imp: array
var: guessed array[string] = []
guessed = array.push(guessed, 'w')
```

---

## time

### time.now() -> int

Stability: stable. Effect status: pure expression.

Returns the current Unix timestamp in seconds from the native C runtime.

```walk
imp: time
out: > time.now() 0
```

---

## random

### random.int(int, int) -> int

Stability: stable. Effect status: pure expression.

Returns an integer in the inclusive range. If `max < min`, WalkLang returns
`min`.

`random.int`, `random.float`, and `random.choice` use a runtime-owned PRNG
seeded once per native process. WalkLang does not expose manual seeding.

```walk
imp: random
out: random.int(1, 10)
```

### random.float(number, number) -> float

Stability: stable. Effect status: pure expression.

Returns a uniform float in `[min, max)`. Arguments may be `int` or `float`. If
`max < min`, WalkLang returns `min`.

```walk
imp: random
out: random.float(0, 1)
```

### random.choice(array[T]) -> T

Stability: stable. Effect status: pure expression.

Returns one item from a non-empty stable native array. Calling
`random.choice` on an empty array runtime-stops with
`walk runtime error: random.choice on empty array`.

```walk
imp: random
var: words = ['dog', 'cat']
out: random.choice(words)
```

### Normal Sampling Recipe

Stability: stable recipe. Effect status: pure expression calls.

WalkLang does not expose a first-party `random.normal` helper yet. Use a
source-level Marsaglia polar helper when a program needs normally distributed
samples:

```walk
imp: math
imp: random

func: normal_sample() float
    var: u = 0.0
    var: v = 0.0
    var: s = 2.0
    while: or >= s 1.0 == s 0.0
        u = - (* 2 random.float(0, 1)) 1
        v = - (* 2 random.float(0, 1)) 1
        var: uu = * u u
        var: vv = * v v
        s = + uu vv
    var: log_s = math.log(s)
    var: numerator = * -2 log_s
    var: ratio = / numerator s
    var: scale = math.sqrt(ratio)
    return: * u scale
```

---

## testing

### testing.assert(bool) -> bool

Stability: stable. Effect status: pure expression.

Returns the bool argument unchanged. Use it with `assert:` when a test wants the assertion helper to be visibly namespaced.

```walk
imp: testing

test: 'works'
    assert: testing.assert(true)
```

`testing.assert` itself does not print or stop a program. The `assert:` statement owns test failure reporting.

---

## Draft map

Draft `map` supports the PicoNet-oriented `map[string]array[string]` type. It is
explicit, insertion-ordered, and value-returning: `map.set` and `map.push`
return a new map value instead of mutating in place.

Current draft limits:

```text
key type: string
value type: array[string]
empty literal: [] only with an explicit map[string]array[string] annotation
missing map.get key: empty array[string]
```

```walk
imp: map
imp: array

var: table map[string]array[string] = []
table = map.push(table, 'of the', 'people')
table = map.push(table, 'of the', 'walk')

out: map.has(table, 'of the')
out: table['of the'][0]
out: array.len(map.keys(table))
```

### map.empty() -> map[string]array[string]

Stability: draft. Effect status: pure expression.

Returns an empty draft string-array map. Use an explicit annotation when the
type needs to be visible at the binding site.

### map.has(map[string]array[string], string) -> bool

Stability: draft. Effect status: pure expression.

Returns `true` when the map contains the key.

### map.get(map[string]array[string], string) -> array[string]

Stability: draft. Effect status: pure expression.

Returns the array stored for the key. Missing keys return an empty
`array[string]`.

Map index lookup is equivalent:

```walk
out: table['of the'][0]
```

### map.set(map[string]array[string], string, array[string]) -> map[string]array[string]

Stability: draft. Effect status: pure expression.

Returns a map with the key set to the supplied array. Existing keys keep their
position in `map.keys`; new keys append to the insertion order.

### map.keys(map[string]array[string]) -> array[string]

Stability: draft. Effect status: pure expression.

Returns keys in insertion order.

### map.push(map[string]array[string], string, string) -> map[string]array[string]

Stability: draft. Effect status: pure expression.

Returns a map with the string appended to the key's array. Missing keys are
created with a one-item array.

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

Failure policy:

- `io.write`, `io.write_line`, and `io.error_line`: fail-stop runtime failure
  only for unrecoverable runtime/backend failure.
- `io.read_line` and `io.read_all`: recoverable result data for ordinary EOF or
  stdin read failure; allocation failure remains fail-stop.

### io.write(string) -> effect

Failure policy: fail-stop runtime failure.

Writes the string to stdout without adding a newline and flushes stdout.

### io.write_line(string) -> effect

Failure policy: fail-stop runtime failure.

Writes the string and a trailing newline to stdout and flushes stdout.

### io.error_line(string) -> effect

Failure policy: fail-stop runtime failure.

Writes the string and a trailing newline to stderr and flushes stderr.

### io.read_line() -> IOReadResult

Failure policy: recoverable result data.

Reads one line from stdin. A trailing LF or CRLF line ending is stripped. A final
unterminated line succeeds. Immediate EOF is returned as data with
`error 'eof'`.

### io.read_all() -> IOReadResult

Failure policy: recoverable result data.

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

Failure policy: recoverable result data.

Parses a base-10 integer with an optional leading sign. Invalid input returns
`error 'invalid int'`; overflow returns `error 'int out of range'`.

### parse.float(string) -> ParseFloatResult

Failure policy: recoverable result data.

Parses a finite floating-point number. Invalid input returns
`error 'invalid float'`; range errors return `error 'float out of range'`.

### parse.bool(string) -> ParseBoolResult

Failure policy: recoverable result data.

Parses exactly `true` or `false`. Other text returns `error 'invalid bool'`.

---

## Draft file

Draft `file` APIs are UTF-8 text helpers. They use native process paths:
relative paths resolve against the current working directory, absolute paths
are passed to the host OS, and this draft slice does not normalize paths or
expand `~`. Empty paths runtime-stop for fail-stop file effects and reads.

`file.read`, `file.write`, `file.append`, `file.write_chunks`, and
`file.append_chunks` are fail-stop helpers. Missing files, permission errors,
invalid UTF-8, embedded null bytes on read, and write failures stop the native
program with a `walk runtime error`.

Recoverable file helpers return draft result structs:

```walk
struct: FileReadResult
    ok bool
    value string
    error string

struct: FileActionResult
    ok bool
    value bool
    error string
```

`error` is `''` on success. `FileActionResult.value` is `true` on success and
`false` on failure.

Failure policy:

- `file.read`, `file.write`, `file.append`, `file.write_chunks`, and `file.append_chunks`: fail-stop runtime failure for ordinary file/path/read/write failures.
- `file.try_read`, `file.try_write`, `file.try_append`, `file.try_write_chunks`, and `file.try_append_chunks`: recoverable result data for ordinary file/path/read/write failures.
- `file.exists`: boolean data for path existence and no runtime failure for an empty path.

### file.read(string) -> string

Failure policy: fail-stop runtime failure.

Reads the whole UTF-8 text file into a runtime-owned string.

### file.try_read(string) -> FileReadResult

Failure policy: recoverable result data.

Reads a UTF-8 text file without runtime-stopping for ordinary file failures.
Missing files return `ok false`, `value ''`, and `error 'file read failed'`.
Invalid UTF-8 and embedded null bytes are reported as file errors. Allocation
failure still runtime-stops.

### file.write(string, string) -> effect

Failure policy: fail-stop runtime failure.

Overwrites the target path with UTF-8 text. Create parent directories first;
this helper does not create them.

### file.try_write(string, string) -> FileActionResult

Failure policy: recoverable result data.

Attempts the same overwrite behavior as `file.write` and returns failures as
data instead of runtime-stopping for ordinary file/path/write errors.

### file.append(string, string) -> effect

Failure policy: fail-stop runtime failure.

Appends UTF-8 text to the target file, creating the file if needed. Create
parent directories first; this helper does not create them.

### file.try_append(string, string) -> FileActionResult

Failure policy: recoverable result data.

Attempts the same append behavior as `file.append` and returns failures as data
instead of runtime-stopping for ordinary file/path/write errors.

### file.write_chunks(string, array[string]) -> effect

Failure policy: fail-stop runtime failure.

Overwrites the target path by writing each UTF-8 chunk in order through one
opened file. Create parent directories first; this helper does not create them.
Use this for large generated text when the program can produce bounded chunks
and should not build one large aggregate string first.

### file.try_write_chunks(string, array[string]) -> FileActionResult

Failure policy: recoverable result data.

Attempts the same chunked overwrite behavior as `file.write_chunks` and returns
ordinary file/path/write failures as data.

### file.append_chunks(string, array[string]) -> effect

Failure policy: fail-stop runtime failure.

Appends each UTF-8 chunk in order through one opened file, creating the file if
needed. Create parent directories first; this helper does not create them.

### file.try_append_chunks(string, array[string]) -> FileActionResult

Failure policy: recoverable result data.

Attempts the same chunked append behavior as `file.append_chunks` and returns
ordinary file/path/write failures as data.

```walk
imp: array
imp: file

var: chunks array[string] = []
chunks = array.push(chunks, 'hello')
chunks = array.push(chunks, ' world')
do: file.write_chunks('note.txt', chunks)
out: file.read('note.txt')
```

### file.exists(string) -> bool

Failure policy: returns `false` for ordinary missing or empty paths.

Returns `true` when the path currently exists according to the host OS and
`false` when it does not. Empty paths return `false`.

---

## Draft dir

Draft `dir` APIs operate on native process paths. They do not normalize paths,
expand `~`, or create missing parents. Directory failures are fail-stop in this
draft slice.

### dir.list(string) -> array[string]

Failure policy: fail-stop runtime failure.

Lists names directly inside a directory, excluding `.` and `..`. Results are
sorted by bytewise string order for deterministic output. Returned names are
not joined to the input path.

### dir.make(string) -> effect

Failure policy: fail-stop runtime failure.

Creates one directory. It fails if the parent directory is missing or the path
already exists.

### dir.delete(string) -> effect

Failure policy: fail-stop runtime failure.

Deletes one empty directory. It fails for non-empty directories.

```walk
imp: dir
imp: path

do: dir.make('data')
do: dir.make(path.join('data', 'empty'))
var: files = dir.list('data')
do: dir.delete(path.join('data', 'empty'))
```

---

## Draft path

Draft `path` APIs are small host-path string helpers. They do not normalize
paths, resolve `..`, check whether a path exists, or expand `~`.

### path.join(string, string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Joins two path segments with the host separator when neither side already has a
separator at the join point.

### path.base(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns the substring after the final `/` or `\` path separator. A path ending
with a separator returns `''`.

### path.ext(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns the final extension in the last path segment, including the dot. Paths
with no dot in the final segment return `''`.

---

## Draft process

```walk
imp: process

out: process.arg_count()
out: process.cwd()
do: process.chdir('data')
do: process.exit(0)
```

Process execution helpers return draft result structs:

```walk
struct: ProcessResult
    ok bool
    status int
    stdout string
    stderr string
    error string

struct: ProcessOutputResult
    ok bool
    value string
    status int
    error string
```

`process.run` and `process.output` use argv-style execution. `process.run_shell`
is explicit shell execution and should not be used with interpolated untrusted
input.

### process.args() -> array[string]

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns command-line arguments passed after the executable path. `walk build`
preserves arguments passed to the built executable, and `walk run <source.walk>
-- <program args>` passes arguments through to the temporary executable.

### process.arg_count() -> int

Failure policy: no ordinary runtime failure.

Returns the number of command-line arguments passed after the executable path.

### process.env(string) -> string?

Failure policy: returns `null` when the variable is not set.

Returns an environment variable value, or `null` when the variable is not set.

### process.cwd() -> string

Failure policy: fail-stop runtime failure.

Returns the current working directory as a runtime-owned string.

### process.chdir(string) -> effect

Failure policy: fail-stop runtime failure.

Changes the native process current working directory. This is process-global
state; tests and programs that use it should change back when needed. Empty
paths and failed changes runtime-stop.

### process.run(string, array[string]) -> ProcessResult

Failure policy: recoverable result data.

Runs a command without invoking a shell. The first argument is the executable
path or command name and the second argument is the argv array passed after the
command. Captured stdout and stderr are UTF-8 text. A zero status returns
`ok true`; a non-zero status returns `ok false`, keeps `stdout`, `stderr`, and
`status` as data, and sets `error 'process exited non-zero'`.

Spawn or wait failures return `ok false`, `status -1`, and a process error.
Allocation failure still runtime-stops.

### process.output(string, array[string]) -> ProcessOutputResult

Failure policy: recoverable result data.

Runs a command without invoking a shell and returns stdout in `value`. It is a
convenience wrapper around `process.run`; non-zero status is still returned as
data.

### process.run_shell(string) -> ProcessResult

Failure policy: recoverable result data.

Runs a command through the host shell (`/bin/sh -c` on POSIX hosts and
`cmd.exe /C` on Windows hosts). This helper exists for explicit shell-dependent
programs only; prefer `process.run` when arguments are known.

### process.exit(int) -> effect

Failure policy: intentional fail-stop process termination.

Exits the native process with the given status code.

---

## Draft json

The draft `json` module is a conservative text boundary. WalkLang does not yet
have maps, dynamic values, or generic JSON value structs, so `json.parse` returns
canonical JSON text inside a result struct instead of pretending object fields
are native WalkLang data.

```walk
imp: json

var: parsed = json.parse('{{"name":"walk"}}')
if: parsed.ok
    do: json.write('data.json', parsed.value)
```

Recoverable JSON helpers return:

```walk
struct: JsonResult
    ok bool
    value string
    error string
```

`value` contains compact validated JSON text on success. Invalid JSON returns
`ok false`, `value ''`, and `error 'invalid json'`.

### json.parse(string) -> JsonResult

Failure policy: recoverable result data.

Validates JSON text and returns compact JSON text with insignificant whitespace
removed. Supported JSON text includes objects, arrays, strings, finite JSON
number syntax, `true`, `false`, and `null`.

### json.stringify(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Escapes one WalkLang string as a JSON string literal.

### json.read(string) -> JsonResult

Failure policy: recoverable result data.

Reads a UTF-8 text file with the same file path policy as `file.read`, then
parses it as JSON. File read failures and invalid JSON are returned as data.

### json.write(string, string) -> effect

Failure policy: fail-stop runtime failure.

Validates and compacts JSON text, then overwrites the target file. Invalid JSON
or file write failures runtime-stop with `walk runtime error`.

---

## Draft term

Draft `term` APIs support terminal-oriented CLI output without changing the
stable feature set. Terminal mutation helpers are effects and must
use `do:`.

```walk
imp: io
imp: term

if: term.is_tty()
    do: term.color('red')
    do: io.write_line('error')
    do: term.reset()
```

ANSI output policy:

- `term.color`, `term.background`, `term.style`, `term.reset`, `term.clear`,
  and `term.move` emit ANSI only when stdout is a TTY.
- `NO_COLOR` disables ANSI output.
- `CLICOLOR_FORCE=1` forces ANSI output for deterministic tests and explicit
  scripted use.
- Redirected stdout stays clean by default because mutation helpers no-op when
  stdout is not a TTY.

Supported colors are `default`, `black`, `red`, `green`, `yellow`, `blue`,
`magenta`, `cyan`, and `white`. Supported styles are `bold`, `dim`, `italic`,
`underline`, `reverse`, `normal`, and `reset`. Unknown color, background, or
style names runtime-stop with a `walk runtime error`.

`term.read_key()` returns `IOReadResult`. Non-interactive stdin returns
`ok false`, `value ''`, and `error 'terminal not interactive'` instead of
blocking. Allocation failure still runtime-stops.

### term.is_tty() -> bool

Failure policy: no ordinary runtime failure.

Returns `true` when stdout is an interactive terminal.

### term.color(string) -> effect

Failure policy: fail-stop runtime failure.

Sets the foreground color by supported color name when ANSI output is enabled.

### term.background(string) -> effect

Failure policy: fail-stop runtime failure.

Sets the background color by supported color name when ANSI output is enabled.

### term.style(string) -> effect

Failure policy: fail-stop runtime failure.

Sets a supported terminal style when ANSI output is enabled.

### term.reset() -> effect

Failure policy: no ordinary runtime failure.

Resets terminal styling when ANSI output is enabled.

### term.clear() -> effect

Failure policy: no ordinary runtime failure.

Clears the terminal screen and moves the cursor home when ANSI output is
enabled.

### term.move(int, int) -> effect

Failure policy: fail-stop runtime failure.

Moves the cursor to one-based column and row coordinates when ANSI output is
enabled. Columns and rows less than `1` runtime-stop.

### term.width() -> int

Failure policy: no ordinary runtime failure.

Returns the current terminal width when available, then `COLUMNS` when it is a
positive integer, then `80`.

### term.height() -> int

Failure policy: no ordinary runtime failure.

Returns the current terminal height when available, then `LINES` when it is a
positive integer, then `24`.

### term.read_key() -> IOReadResult

Failure policy: recoverable result data.

Reads one key from an interactive stdin terminal. On POSIX hosts this enters raw
mode for a single key read and restores the terminal before returning.

---

## Draft http

Draft `http` APIs are recoverable value-returning helpers. They do not
runtime-stop for ordinary network, DNS, TLS, timeout, missing-backend, or HTTP
status failures.

```walk
imp: http

var: response = http.get('http://127.0.0.1:8080/health')
out: response.ok
out: response.status
out: response.body
out: response.error
```

Recoverable HTTP helpers return:

```walk
struct: HttpResult
    ok bool
    status int
    body string
    error string
```

`ok` is true for status codes `200` through `399`. Other status codes return
`ok false`, preserve `body`, and set `error 'http status'`. Runtime backend
failures such as missing `curl`, DNS failure, TLS failure, connection failure,
timeout, response-size failure, or invalid output return `ok false` with
`status` from the backend when available, otherwise `-1`.

Draft HTTP uses the system `curl` executable through argv-style process
execution. It does not invoke a shell. Redirects are followed, the timeout is
10 seconds, response bodies are capped at 1 MiB, and response bodies are UTF-8
text only. See [Draft networking](NETWORKING.md) for the security and backend
policy.

### http.get(string) -> HttpResult

Failure policy: recoverable result data.

Sends a `GET` request to the URL.

### http.post(string, string) -> HttpResult

Failure policy: recoverable result data.

Sends a `POST` request with the string body.

### http.request(string, string, string) -> HttpResult

Failure policy: recoverable result data.

Sends a request with `method`, `url`, and `body`. Empty method or URL returns a
recoverable error before any runtime backend is started.

---

## Draft html

Draft `html` helpers generate escaped HTML strings only. They do not write
files, start a web server, run a browser, attach assets, or create a DOM.

```walk
imp: html

out: html.h1('Walk <Lang>')
out: html.p('copy & text')
```

### html.escape(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Escapes `&`, `<`, `>`, `"`, and `'` for HTML text contexts.

### html.h1(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns an escaped `<h1>...</h1>` string.

### html.p(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns an escaped `<p>...</p>` string.

### html.button(string) -> string

Failure policy: fail-stop runtime failure only for unrecoverable allocation failure.

Returns an escaped `<button>...</button>` string.

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

## Planned API Names

These names are planned only. They are documented here so naming can stay
consistent, but they are not stable, not draft-implemented, not importable, and
not compatibility-protected as stable features.

```text
matrix.rows
matrix.cols
matrix.get
```
