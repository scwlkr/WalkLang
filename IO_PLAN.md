# WalkLang IO Plan

Status: design draft with the CLI text IO, local filesystem IO, and process/data
interop roadmap phases implemented as draft compiler APIs. This is not stable
language syntax yet.

Purpose: turn the current input/output brainstorm into a durable implementation
plan for WalkLang's console, file, process, terminal, data, network, and future
UI IO surface.

The goal is not to make every useful command work immediately. The goal is to
grow IO in layers that fit WalkLang's existing design:

```text
readable source
static types
small rules
explicit modules
generated C remains inspectable
native executables stay portable
```

## Current Baseline

Already stable:

```walk
out: 'hello'
out: + 1 2
out: true
var: name = in:
var: name = in: 'Name? '
```

`out:` prints one scalar value with a newline. Stable output types are `int`,
`float`, `bool`, `string`, and nullable string. Arrays, functions, structs, and
void values cannot be printed directly.

`in:` reads one required line from stdin and returns `string`. Its optional
prompt writes to stdout without a newline and flushes before reading. `in:`
strips the final line ending, preserves other whitespace, returns `''` for an
empty line, accepts final unterminated input, and runtime-stops on immediate EOF
stdin failure, or allocation failure.

Stable v1.7 built-in modules:

```text
math
string
array
time
random
testing
```

Draft implemented IO/process/file surface:

```text
do:
io.write
io.write_line
io.error_line
process.args
process.arg_count
process.env
process.cwd
process.exit
io.read_line
io.read_all
parse.int
parse.float
parse.bool
file.read
file.write
file.append
file.exists
file.try_read
file.try_write
file.try_append
dir.list
dir.make
dir.delete
path.join
path.base
path.ext
process.chdir
process.run
process.output
process.run_shell
json.parse
json.stringify
json.read
json.write
```

Planned draft-only stdlib names already called out in `docs/STDLIB.md`:

```text
matrix.rows
matrix.cols
matrix.get
```

Networking is explicitly later roadmap work. The roadmap says to add it only
after error handling is mature, stdlib design is stable, the project/package
model exists, and security rules are documented.

## Durable Direction

### Keep The Core Small

Do not turn every IO action into a core keyword.

Avoid this shape:

```walk
file-read: 'notes.txt'
json-write: 'data.json' data
draw-rect: 10 10 100 50
http-get: url
```

Prefer this shape:

```walk
imp: file
imp: json
imp: http

var: text = file.read('notes.txt')
var: data = json.parse(text)
var: body = http.get(url)
```

`out:` can stay special because it is already stable and fundamental. Most new
IO should be imported standard-library modules.

### Add One Effect Call Mechanism Before Side-Effect Modules

WalkLang currently has command statements such as `out:`, `var:`, `return:`,
and `assert:`. Function calls are expressions, not standalone statements.

Many IO APIs are side effects:

```text
write stderr
write a file
append a file
delete a file
set env var
change directory
move cursor
exit program
```

Before adding many side-effect functions, decide on exactly one effect-call
syntax. Recommended draft:

```walk
do: io.write('Loading...')
do: file.write('out.txt', text)
do: process.exit(1)
```

Why `do:`:

- it preserves the `keyword:` command style
- it avoids inventing many new command keywords
- it makes effects visible
- it lets stdlib APIs stay namespaced
- it gives the checker one place to reject non-effect expressions later if
  WalkLang grows purity or warning rules

Do not stabilize `file.write`, `io.write`, `process.exit`, terminal mutation, or
process spawning until this decision is settled.

### Use Module Families

Proposed module families:

| Module | Scope | Example |
| --- | --- | --- |
| `io` | stdin, stdout, stderr stream helpers | `io.read_line()` |
| `file` | text file operations | `file.read('notes.txt')` |
| `dir` | directory operations | `dir.list('.')` |
| `path` | path joining and path facts | `path.join('data', 'in.txt')` |
| `process` | args, env, cwd, exit, commands | `process.args()` |
| `term` | terminal styling, cursor, TTY state | `term.clear()` |
| `json` | JSON parse/stringify | `json.parse(text)` |
| `http` | HTTP client | `http.get(url)` |
| `log` | structured console logging | `log.warn('low memory')` |
| `html` | HTML text generation, not browser runtime | `html.h1('Hello')` |

Keep graphics, windows, mouse input, web servers, and browser targets outside
the early standard library. Those belong in later packages or alternate runtime
targets once the core IO model is proven.

### Prefer Text First, Binary Later

Early IO should be UTF-8 text IO:

```text
read text
write text
append text
read line
read all stdin
parse text
stringify text
```

Binary IO needs byte arrays, buffer ownership, and stronger memory rules. Do not
smuggle binary IO through `string`.

### Make Errors Predictable Before Broad IO

IO fails often:

```text
missing file
permission denied
invalid UTF-8
EOF
bad integer input
network timeout
TLS failure
command not found
non-zero exit status
terminal not interactive
```

Do not add broad IO while the only story is "something prints to stderr".

Recommended error progression:

1. Keep the first draft small and fail-stop where recovery is not yet possible.
   A failed `file.read` can stop the program with a clear runtime error while
   the API is draft-only.
2. Add a recoverable result shape before stabilizing broad IO. The accepted
   draft shape is concrete module-specific structs with `ok bool`, `value T`,
   and `error string`. A future generic `Result[T]` can replace this once
   generic structs exist. Explicit `try_` variants remain possible if a future
   core API stays fail-stop.
3. Stable IO docs must state exactly which failures stop the program and which
   failures are returned as values.

Do not use nullable-only returns for real IO errors. `null` can mean "missing"
for `process.env(name)`, but it should not hide why a file or network operation
failed.

### Treat Runtime Ownership As Part Of IO

Reading from stdin, files, processes, and network sockets creates new strings.
The current C backend mostly prints string literals or values already present in
the program. Before dynamic text IO is stable, document and implement runtime
string ownership.

Recommended first ownership rule:

```text
runtime-created strings live for the process lifetime
```

That matches the current v5 array storage direction and keeps source-level
memory management out of WalkLang. It is acceptable for short CLI programs. It
is not enough for long-running servers, so server/network work should stay later
until ownership is revisited.

### Refactor Built-In Function Registration Before The List Grows

Today, built-in module behavior is spread across checker, inference, and
emitter switch statements. That is manageable for six modules. It will become
fragile for IO.

Before adding more than one or two IO functions, introduce a small built-in API
registry that records:

```text
module name
function name
parameter types
return type
effect or pure
runtime helper name or emitter hook
stable or draft status
doc/example metadata if useful
```

This avoids each IO function being hand-wired in multiple unrelated places.

## Capability Triage

### 1. Basic Console Output

Original ideas:

```text
print text
print number
print decimal
print bool
print variable
print expression
print multiple values
print formatted string
print without newline
print newline
blank line
print error text
debug value
inspect type
```

Recommended direction:

```walk
out: 'Hello'
out: + 2 3

imp: io
do: io.write('Loading...')
do: io.write_line('done')
do: io.error_line('Something failed')
```

Keep:

- `out:` for one scalar line
- `out: ''` for a blank line
- `io.write` for no newline
- `io.write_line` for explicit stdout newline behavior
- `io.error_line` for stderr

Defer:

- multiple-value `out:` until there is vararg or formatting design
- formatted strings until interpolation or `string.format` is designed
- `debug:` and `debug-type:` as language syntax

`debug` belongs first in tooling:

```text
walk check diagnostics
walk debug-map
future walk inspect
future debugger
```

What it takes:

- `do:` effect statement, or another approved side-effect statement form
- `io` built-in module signatures
- stdout/stderr runtime helpers in generated C
- checker rules for effect calls
- formatter support for `do:`
- pass/fail tests for scalar output and side effects
- docs in `SPEC.md`, `SYNTAX.md`, and `STDLIB.md`

Order: first IO implementation slice after design prework.

### 2. Text Input

Original ideas:

```text
input text
input int
input float
input bool
read line
read all stdin
read char after enter
read instant key
wait for enter
```

Recommended direction:

```walk
var: name = in: 'Name? '
out: name
```

`in:` is now the stable core required-line input expression. Do not add
prompt-specific typed commands like `in-int:`. Keep reading and
parsing separate:

```walk
imp: parse

var: text = in: 'Age? '
var: age = parse.int(text)
```

Lower-level stream APIs such as `io.read_line()` remain future work for code
that needs EOF/error as data. The exact `parse` API is a separate design
decision. It depends on the error model because invalid input is normal, not
exceptional.

Defer:

- `read_key` until terminal raw mode exists
- typed input helpers until parse/error handling exists
- password input until terminal mode and security rules exist

What it takes:

- stable `in:` parser, checker, formatter, emitter, docs, and tests
- future runtime-owned strings for lower-level stream APIs
- future EOF/result behavior for `io.read_line()`
- future line length and allocation behavior for long-running stream APIs
- clear invalid-conversion behavior for parse helpers
- tests that feed stdin to native executables
- cross-platform newline handling

Order: stable `in:` is the first input slice. Lower-level stream APIs come
after recoverable errors.

### 3. Files

Original ideas:

```text
read file
write file
append file
delete file
check file exists
copy file
move file
```

Recommended direction:

```walk
imp: file

var: text = file.read('notes.txt')
do: file.write('out.txt', text)
do: file.append('log.txt', 'started\n')
var: ok = file.exists('notes.txt')
```

Start with text files only:

```text
file.read(path) -> string
file.write(path, text) -> effect
file.append(path, text) -> effect
file.exists(path) -> bool
```

Then add:

```text
file.delete(path)
file.copy(from, to)
file.move(from, to)
```

Defer:

- binary files
- file permissions
- file metadata
- streaming file handles
- watching files

What it takes:

- approved effect-call syntax
- runtime-owned strings
- path policy
- file error policy
- generated C helpers using portable C where possible
- platform-specific handling where C is not enough
- tests with temp directories
- negative tests for invalid paths and missing files
- docs that state overwrite behavior

Order: after input/string ownership and the initial error policy.

### 4. Directories And Paths

Original ideas:

```text
list folder
create folder
delete folder
get current folder
change folder
```

Recommended direction:

```walk
imp: dir
imp: path
imp: process

var: here = process.cwd()
var: files = dir.list('.')
do: dir.make('data')
do: process.chdir('data')
```

Keep directory operations separate from file operations. Files and directories
fail differently and need different docs.

What it takes:

- arrays of strings returned from runtime helpers
- runtime string and array ownership
- cross-platform directory iteration
- path normalization rules
- cwd mutation policy
- tests that isolate cwd changes

Order: after file basics. `process.cwd()` can come earlier with process basics.

### 5. Process, Args, Env, Exit, Commands

Original ideas:

```text
get command args
get arg count
get env variable
set env variable
run shell command
run command with args
capture command output
get exit code
exit program
exit with error
open URL
```

Recommended direction:

```walk
imp: process

var: args = process.args()
var: mode = process.env('MODE')
var: here = process.cwd()
do: process.exit(0)
```

For commands, prefer argv-style APIs. Do not stabilize shell-string execution as
the default.

Prefer:

```walk
var: result = process.run('git', ['status'])
```

Avoid:

```walk
var: result = process.run_shell('git status')
```

If `run_shell` ever exists, it should be explicitly named and documented as
shell-dependent.

Defer:

- command spawning until arrays, structs/results, and error policy are ready
- `open_url` until OS integration rules are clear
- background processes and pipes

What it takes:

- generated `main(int argc, char **argv)` instead of `main(void)`
- runtime conversion from argv to WalkLang `array[string]`
- nullable env return for missing variables
- platform-specific env set/chdir/spawn code
- process result type with stdout, stderr, status, and error
- security docs warning against shell interpolation
- tests that do not depend on local machine-specific commands

Order: args/env/cwd/exit can come early; command spawning comes much later.

### 6. Terminal UI

Original ideas:

```text
set text color
reset text color
print colored text
set background color
bold text
italic text
underline text
reset style
clear terminal
move cursor
hide cursor
show cursor
get terminal width
get terminal height
draw terminal text
draw horizontal line
draw vertical line
draw box
read instant key
```

Recommended direction:

```walk
imp: term
imp: io

if: term.is_tty()
    do: term.color('red')
    do: io.write_line('Error')
    do: term.reset()
```

Start with ANSI-style helpers:

```text
term.is_tty()
term.color(name)
term.background(name)
term.style(name)
term.reset()
term.clear()
```

Then cursor and size:

```text
term.move(x, y)
term.cursor('hide')
term.cursor('show')
term.width()
term.height()
```

Defer raw keyboard input and drawing helpers until terminal capability detection
exists.

What it takes:

- TTY detection
- ANSI support policy
- Windows terminal behavior
- no-color behavior for redirected output
- terminal dimensions fallback
- cleanup strategy when hiding cursor
- integration tests that can run without a real interactive TTY

Order: after console/process basics. This is not required for file/network IO.

### 7. JSON And Structured Data IO

Original ideas:

```text
read JSON
write JSON
parse JSON
stringify JSON
```

Recommended direction:

```walk
imp: file
imp: json

var: text = file.read('data.json')
var: data = json.parse(text)
```

Do not stabilize JSON until the data model can represent JSON honestly.

Open design questions:

- Does WalkLang need maps/dictionaries?
- Are JSON objects decoded into structs?
- How are missing fields represented?
- How are parse errors returned?
- How does JSON handle arrays of mixed types?
- Is `json.stringify` for typed structs only, or for dynamic JSON values?

What it takes:

- data model decision for objects and dynamic values
- recoverable parse errors
- string escaping rules
- UTF-8 behavior
- generated C JSON runtime or linked C library decision
- round-trip tests

Order: after data modeling and error handling. File IO can ship before JSON.

### 8. Time, Sleep, Timers, Random

Original ideas:

```text
wait seconds
start timer
stop timer
get current time
get date
random int
random float
seed random
```

Already stable:

```walk
imp: time
imp: random

var: now = time.now()
var: n = random.int(1, 10)
```

Recommended next additions:

```text
time.sleep(seconds)
time.millis()
time.monotonic()
random.float()
random.seed(seed)
```

Be careful with global state. `random.seed` changes process-level randomness.
That is acceptable only if documented clearly.

What it takes:

- monotonic vs wall-clock decision
- cross-platform sleep helper
- deterministic random tests
- docs that explain default seeding
- stable behavior when ranges are invalid

Order: `time.sleep` can come near console/input work. Better timer APIs can wait.

### 9. Logging, Testing, Assertions

Original ideas:

```text
log info
log warning
log error
assert output
test output
```

Already stable:

```walk
test: 'works'
    assert: true
```

`testing.assert(bool)` is also stable.

Recommended direction:

```walk
imp: log

do: log.info('Started')
do: log.warn('Low memory')
do: log.error('Failed')
```

Start logging as formatted stderr/stdout helpers only. Do not add log files,
timestamps, levels, configuration, or structured sinks until basic IO is stable.

What it takes:

- stderr output
- effect-call syntax
- stable level names
- predictable output format
- tests that assert exact output when feasible

Order: after `io.error_line`.

### 10. HTTP And Networking

Original ideas:

```text
HTTP GET
HTTP POST
open URL
```

Roadmap gate:

```text
Only add networking when:
error handling is mature
stdlib design is stable
package/project model exists
security rules are documented
```

Recommended future direction:

```walk
imp: http

var: response = http.get('https://example.com')
out: response.body
```

Open design questions:

- What is the response type?
- How are headers represented?
- How are timeouts configured?
- What TLS implementation is used with the C backend?
- Is HTTP in the core stdlib or a first-party package?
- Are redirects followed by default?
- How are request bodies represented?

What it takes:

- mature error/result shape
- networking security docs
- timeout and size-limit policy
- TLS/backend dependency decision
- process lifetime and allocation rules for response bodies
- tests that do not depend on flaky public network calls

Order: much later. Do not let HTTP block console, file, or process IO.

### 11. Graphics, Windows, Mouse, Screen

Original ideas:

```text
make window
draw pixel
draw line
draw rectangle
draw circle
draw image
get mouse position
check mouse click
check key down
update screen
close window
```

This should not be core language IO.

Recommended direction:

```text
first-party package or alternate backend
not v1/v2 core stdlib
not required for CLI or file IO
```

Why:

- it needs an event loop
- it needs assets
- it needs platform libraries
- it needs a windowing backend
- it changes the runtime model
- it probably wants a package boundary

Order: after package ecosystem and backend/runtime maturity.

### 12. HTML, CSS, Web Server, Browser Target

Original ideas:

```text
create HTML file
write HTML tag
write paragraph
write button
write CSS file
start web server
route web page
send web response
browser target
compile --target web main.walk
```

Split this into three separate tracks:

1. HTML text generation:

   ```walk
   imp: html
   var: page = html.h1('Hello')
   ```

2. HTTP server runtime:

   ```walk
   imp: web
   # future only
   ```

3. Browser/WASM target:

   ```text
   alternate backend
   future only
   ```

Do not mix these. Generating an HTML string is small. Running a server is a
runtime architecture decision. Browser compilation is a backend decision.

Order: HTML helpers can come after strings are stronger. Web servers and browser
targets are much later.

## Implementation Roadmap

Use this five-phase roadmap for planning, status, and future handoffs. The
older fine-grained sections below remain as dependency notes, not the primary
roadmap.

### Roadmap Phase 1: CLI Text IO

Status: done as draft compiler APIs in `v5.7.0`.

Scope:

```text
do:
io.write
io.write_line
io.error_line
process.args
process.arg_count
process.env
process.cwd
process.exit
io.read_line
io.read_all
parse.int
parse.float
parse.bool
```

This phase proves explicit effect calls, process basics, runtime-owned text,
stdin tests, EOF-as-data, and the first concrete recoverable result structs.
The stable language contract remains v1.9.

### Roadmap Phase 2: Local Filesystem IO

Status: done as draft compiler APIs in `v5.9.0`.

Scope:

```text
file.read
file.write
file.append
file.exists
dir.list
dir.make
dir.delete
path.join
path.base
path.ext
process.chdir
recoverable file results
```

This phase combines the old text-file, directory/path, and file-error phases
into one local-filesystem milestone. It should make file-backed CLI programs
possible without starting process spawning, JSON, HTTP, terminal raw mode, or
browser/runtime work.

Phase decisions:

- Path policy: pass native relative and absolute paths to the host OS without
  normalization or `~` expansion. Relative paths resolve against the native
  process current working directory.
- UTF-8 file policy: `file.read` and `file.write` are text-only. Reads reject
  invalid UTF-8 and embedded null bytes. Writes accept WalkLang strings and do
  not expose binary output. `file.append` follows the same text policy.
- Error policy: fail-stop `file.read`, `file.write`, `file.append`, directory
  helpers, and `process.chdir` stop the native program with a clear
  `walk runtime error`. Recoverable `file.try_read`, `file.try_write`, and
  `file.try_append` return draft result structs for ordinary file/path/read and
  write failures; allocation failure remains fail-stop.
- Tests: file APIs are proven with native executable tests that run inside Go
  `t.TempDir()` directories.
- Cwd mutation policy: `process.chdir` is draft-only, explicit through `do:`,
  and covered by isolated cwd tests that restore the original directory.

Done when:

- programs can read, write, append, and check UTF-8 text files
- programs can create, list, and delete directories
- programs can build paths without hardcoding separators
- file and directory failures are either recoverable values or clearly
  documented fail-stop behavior
- docs state overwrite, encoding, path, cwd, and failure behavior

### Roadmap Phase 3: Process And Data Interop

Status: done as draft compiler APIs in `v5.10.0`.

Scope:

```text
process.run
process.output
process.run_shell
json.parse
json.stringify
json.read
json.write
process result structs
JSON result structs
```

This phase combines command execution and structured text data. `run_shell`
must stay explicit and later than argv-style command execution.

Entry gates:

- accepted result struct pattern from filesystem work
- command result type with status, stdout, stderr, and error
- deterministic local helper command tests
- JSON data model decision for objects, maps, dynamic values, or typed structs
- JSON string escaping and invalid JSON error policy

Phase decisions:

- Process result policy: `process.run(command, args)` uses argv-style execution
  without a shell and returns `ProcessResult` with `ok`, `status`, `stdout`,
  `stderr`, and `error`. Non-zero status is data, not a runtime stop.
- Output convenience policy: `process.output(command, args)` returns
  `ProcessOutputResult` with stdout in `value` while preserving `status` and
  error data for failures.
- Shell policy: `process.run_shell(command)` stays explicit and shell-dependent;
  it exists after argv-style process execution and is documented as unsafe for
  interpolated untrusted input.
- Process text policy: captured stdout and stderr are UTF-8 text. Output read
  failures are returned as process errors; allocation failure remains fail-stop.
- JSON data model policy: until WalkLang has maps, dynamic values, or generic
  recursive JSON structs, JSON APIs use validated compact JSON text as the
  interchange value. `JsonResult.value` is canonical JSON text, not a native map
  or object.
- JSON error policy: invalid JSON returns `ok false`, `value ''`, and
  `error 'invalid json'` for `json.parse` and `json.read`. `json.write` is an
  effect and fail-stops on invalid JSON or write failure.

Done when:

- argv-style process execution works
- command output and non-zero status are visible as data
- JSON can round-trip through the documented supported types
- invalid JSON errors are recoverable and testable

### Roadmap Phase 4: Terminal UX

Status: future.

Scope:

```text
term.is_tty
term.color
term.background
term.style
term.reset
term.clear
term.move
term.width
term.height
term.read_key
```

Terminal work stays separate because TTY detection, ANSI behavior, cleanup, and
Windows support are a distinct portability problem from file/process/data IO.

Entry gates:

- TTY detection
- ANSI/no-color policy
- terminal cleanup behavior
- Windows behavior
- non-interactive test strategy

Done when:

- redirected output remains clean
- styling is opt-in and resettable
- non-interactive tests can still run

### Roadmap Phase 5: Network And Rich Runtimes

Status: future.

Scope:

```text
http.get
http.post
http.request
html helpers
web server package
native graphics package
WASM/browser backend
compiler explorer/playground integration
```

This phase collects everything that changes security posture, backend/runtime
architecture, event loops, or public network behavior. It should not block the
CLI, filesystem, process, JSON, or terminal milestones.

Entry gates:

- mature error handling
- package/project model
- networking security docs
- timeout and response-size policy
- TLS/backend dependency decision
- runtime/backend architecture decision for web, graphics, and browser targets
- event loop and asset policies

Done when:

- network tests avoid public-network flakiness
- failures are recoverable
- security behavior is documented
- rich runtime tracks have their own design docs

### Small Helpers

`time.sleep` is not a roadmap phase. Treat it as a small helper that can attach
to whichever release first needs it, with cross-platform sleep behavior and
deterministic tests.

## Detailed Dependency Notes

### Phase 0: IO Design Groundwork

Goal: make the next code slice coherent before adding features.

Decisions:

- accept, revise, or reject `do:` for effect calls
- define draft vs stable IO API policy
- define runtime-created string ownership
- define initial fail-stop vs recoverable error policy
- decide path and UTF-8 text policy
- decide whether built-in APIs need a central registry now

Files likely touched:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ARCHITECTURE.md
docs/DESIGN_RULES.md
internal/checker/checker.go
internal/checker/infer.go
internal/emitter/emitter.go
```

Done when:

- IO naming and effect syntax are written down
- unstable APIs are clearly marked draft
- broad IO work has an agreed error story
- tests can be planned without guessing parser/checker behavior

### Phase 1: Console And Process Foundation

Goal: make simple CLI programs more useful without dynamic input yet.

Candidate APIs:

```text
io.write
io.write_line
io.error_line
process.args
process.arg_count
process.env
process.cwd
process.exit
```

Prerequisites:

- effect statement
- `main(argc, argv)` support
- argv to `array[string]`
- nullable string return for missing env vars
- generated C runtime helpers

Done when:

- a WalkLang program can inspect args
- write to stdout without newline
- write to stderr
- exit with a chosen code
- pass/fail/compat tests cover the behavior

### Phase 2: Runtime-Owned Text Input

Goal: read text safely enough for CLI programs.

Status: draft text input and parse helpers implemented. `time.sleep` remains a
separate candidate time helper, not part of this text-input completion slice.

Accepted draft recoverable result policy:

```text
IOReadResult(ok bool, value string, error string)
ParseIntResult(ok bool, value int, error string)
ParseFloatResult(ok bool, value float, error string)
ParseBoolResult(ok bool, value bool, error string)
```

`error` is `''` on success. `io.read_line()` returns immediate EOF as
`error 'eof'`. Parse helpers return invalid input as data, not as nullable-only
missing values and not as runtime stops.

Candidate APIs:

```text
io.read_line
io.read_all
time.sleep
parse.int
parse.float
parse.bool
```

Prerequisites:

- runtime-owned dynamic strings
- stdin tests
- EOF behavior
- conversion error policy

Done when:

- a program can prompt manually with `io.write`
- read a line
- parse simple typed values
- handle invalid input according to the documented policy

### Phase 3: Text File IO

Goal: make file-backed CLI programs possible.

Candidate APIs:

```text
file.read
file.write
file.append
file.exists
```

Prerequisites:

- runtime-owned dynamic strings
- path policy
- file error policy
- temp-directory tests

Done when:

- a program can read a UTF-8 text file
- write and append text
- check existence
- docs state overwrite, encoding, and failure behavior

### Phase 4: Directory And Path Utilities

Goal: support project and data-folder workflows.

Candidate APIs:

```text
dir.list
dir.make
dir.delete
path.join
path.base
path.ext
process.chdir
```

Prerequisites:

- arrays of runtime-owned strings
- cross-platform directory implementation
- cwd mutation policy

Done when:

- a program can list a folder
- create and delete a folder
- build paths without hardcoding separators
- tests isolate cwd changes

### Phase 5: Recoverable IO Errors

Goal: stop treating all IO failure as fatal.

Candidate work:

```text
result structs
try_ file APIs
process result type
json parse result
```

Prerequisites:

- stable or accepted struct/result shape
- docs for fail-stop vs recoverable variants
- compatibility rules for error messages that are asserted in tests

Done when:

- file and process failures can be handled in WalkLang source
- the docs do not hide common IO failure paths

### Phase 6: Process Spawning

Goal: let WalkLang run other commands without making shell injection the easy
path.

Candidate APIs:

```text
process.run(command, args)
process.output(command, args)
process.run_shell(command)
```

Rules:

- `run_shell` is explicit and later, not the default
- command results include status, stdout, stderr, and error
- tests use deterministic local helper commands

Done when:

- argv-style command execution works
- output capture works
- non-zero status is visible without being confused with runtime failure

### Phase 7: Terminal Module

Goal: support nicer CLI output without pretending to be a graphics library.

Candidate APIs:

```text
term.is_tty
term.color
term.background
term.style
term.reset
term.clear
term.move
term.width
term.height
term.read_key
```

Prerequisites:

- TTY detection
- ANSI/no-color policy
- terminal cleanup behavior
- Windows behavior

Done when:

- redirected output remains clean
- terminal styling is opt-in and resettable
- non-interactive tests can still run

### Phase 8: JSON

Goal: support structured text data once WalkLang can represent it honestly.

Candidate APIs:

```text
json.parse
json.stringify
json.read
json.write
```

Prerequisites:

- data model for objects/maps/dynamic JSON or typed struct decoding
- recoverable parse errors
- string escaping behavior

Done when:

- JSON round trips through documented supported types
- invalid JSON errors are recoverable and testable

### Phase 9: HTTP

Goal: add network client IO after the language has the safety rails to own it.

Candidate APIs:

```text
http.get
http.post
http.request
```

Prerequisites:

- mature error handling
- package/project model
- security docs
- timeout and response-size policy
- TLS/backend dependency decision

Done when:

- tests avoid public-network flakiness
- failures are recoverable
- security behavior is documented

### Phase 10: Web, Graphics, Browser Targets

Goal: explore richer runtimes without bloating core IO.

Tracks:

```text
html helpers
web server package
native graphics package
WASM/browser backend
compiler explorer/playground integration
```

Prerequisites:

- package ecosystem
- runtime/backend architecture decision
- event loop design
- asset policy

Done when:

- each track has its own design doc
- no track forces core CLI/file IO to carry graphics or server complexity

## First Recommended Implementation Slice

Do not start with files, HTTP, terminal raw mode, or JSON.

The first stable slice is now `in:`:

```text
1. Add core `in:` expression.
2. Support optional string prompts.
3. Read stdin without a fixed language-level line limit.
4. Runtime-stop on immediate EOF, stdin failure, or allocation failure.
5. Add compatibility tests, formatter support, docs, and generated C coverage.
```

The next side-effect slice should prove the `do:` architecture:

```text
1. Add the `do:` effect statement.
2. Add a tiny `io` module:
   - io.write(string)
   - io.write_line(string)
   - io.error_line(string)
3. Add a tiny `process` module:
   - process.exit(int)
4. Document that these are draft until the broader IO policy is accepted.
5. Add pass/fail tests, formatter support, and generated C coverage.
```

Why this first:

- it tests the side-effect model
- it avoids dynamic string allocation
- it avoids file-system portability
- it avoids recoverable error design
- it gives immediate CLI value
- it keeps the implementation small enough to review

Second slice:

```text
process.args
process.env
process.cwd
```

Third slice:

```text
recoverable stream result shape
io.read_line()
io.read_all()
parse helpers
```

That sequence is complete as draft compiler APIs. The local filesystem slice is
also complete as draft compiler APIs with UTF-8 text file helpers, recoverable
`file.try_*` results, directory/path helpers, and `process.chdir`.

## Stable API Gate

No IO API is stable until it has:

```text
spec text if it affects syntax or stable language behavior
syntax docs when user-facing
stdlib docs when imported
positive tests
negative tests for invalid use
native C build-and-run proof
formatter support for any new syntax
compatibility fixture if part of the stable contract
clear runtime error or result behavior
generated C remains inspectable
```

## Summary Recommendation

Keep `out:` as the simple line-output command.

Add one general side-effect command before growing IO.

Grow IO through modules in this order:

```text
CLI text IO
local filesystem IO
process and data interop
terminal UX
network and rich runtimes
```

This keeps WalkLang useful for real CLI programs first, while leaving room for
larger IO systems without gluing them onto the language as one-off keywords.
