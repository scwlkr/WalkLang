# ROADMAP.md

## WalkLang Roadmap

Build the smallest useful language first.

```text
v0: compile real programs
v0.1: test and explore
v1: harden and grow
v1.1: stabilize the language contract
v1.2: make projects and releases real
v1.3: grow the standard library carefully
v1.6: local function type inference
v1.7: required-line stdin input
v1.8: terminal game helpers
v1.9: string interpolation
v2: data modeling
v2.1: methods and stronger composition
v2.2: simple generic functions
v3: ecosystem
v4: professional tooling
v5: runtime/backend maturity
v5.12: draft network and rich-runtime boundary
v5.12.1: docs search
v5.12.2: docs search result previews
v5.12.3: docs sidebar navigation cleanup
v5.13.0: explicit systems track
later: package/runtime tracks and advanced language work
```

WalkLang should become real in layers:

```text
first: programs compile
second: behavior is specified
third: tools are reliable
fourth: users can build projects
fifth: ecosystem grows around stable rules
```

---

## v0 Goal

A `.walk` file compiles to a native executable through C.

Example target:

```walk
func: add(a int, b int) int
    return: + a b

var: nums = [1, 2, 3]

for: n in nums
    out: add(n, 10)
```

Expected output:

```text
11
12
13
```

---

## v0 Includes

Language:

```text
.walk files
indentation blocks
single quotes
prefix math
static types
type inference
optional annotations
var / const
if / else
while / for / repeat
break / continue
func / return
recursion
arrays
null
function values
imp / exp
```

Example:

```walk
var: age = 25
var: name string? = null

if: > age 18
    name = 'adult'
```

---

## v0 Compiler

Pipeline:

```text
lexer
parser
AST
type checker
C emitter
native build
```

Example compile flow:

```text
main.walk
  -> tokens
  -> AST
  -> typed IR
  -> main.c
  -> executable
```

---

## v0 Tooling

Required:

```text
compiler
formatter
clear errors
```

Formatter example:

```walk
var:x=+ 1 2
```

Becomes:

```walk
var: x = + 1 2
```

Error example:

```walk
var: x = 1
x = 'one'
```

Output:

```text
main.walk:2
type error: x is int, got string
```

---

## v0 Standard Library Target

Core libraries:

```text
math
string
array
matrix
file
json
time
random
testing base
```

Example:

```walk
imp: math
imp: random

var: n = random.int(1, 10)
out: math.sqrt(n)
```

---

## v0 Non-Goals

Do not build these yet:

```text
classes
structs
methods
inheritance
interfaces
closures
anonymous functions
try/catch
networking
package manager
debugger
full LSP
```

Example:

```walk
class: User # rejected until later
```

---

## v0 Done When

v0 is done when this works:

```walk
# main.walk

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

Expected:

```text
11
12
13
distance is 5
```

---

## v0.1 Goal

Make the language easier to test and explore.

Add:

```text
test runner
basic REPL
better diagnostics
standard library polish
```

Test example:

```walk
test: 'add works'
    assert: == add(2, 3) 5
```

REPL example:

```text
walk> + 1 2
3
```

v0.1 done when:

```text
walk test runs .walk tests
walk repl evaluates basic expressions
diagnostics include file, line, and column
basic stdlib functions are documented
example programs can be tested automatically
```

---

## v1 Goal

Make WalkLang feel stable.

Add or stabilize:

```text
module rules
library APIs
formatter rules
warning levels
docs
release builds
cross-platform build flow
```

Example module target:

```walk
# math_extra.walk
func: cube(x int) int
    return: * x x x

exp: cube
```

Use:

```walk
imp: math_extra

out: math_extra.cube(3)
```

v1 done in this repo means:

```text
examples/v1.walk builds and runs through a sibling module
walk check supports warning levels
formatter rules normalize spacing and indentation
release builds accept native compiler flags
scripts/release.sh creates cross-platform CLI artifacts
docs describe the stable v1 surface
```

---

# Next Phases

## v1.1 Language Contract

Make the language less like “whatever the compiler accepts” and more like a real contract.

Add:

```text
complete v1 spec
syntax guide
stdlib reference
diagnostic guide
compatibility rules
conformance tests
negative test fixtures
generated C snapshot tests
```

Required docs:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
docs/DESIGN_RULES.md
docs/COMPATIBILITY.md
```

Core rule:

```text
If it is not in SPEC.md, it is not stable WalkLang.
If the compiler disagrees with SPEC.md, either the compiler or the spec must change.
```

Test layout target:

```text
tests/pass/
  hello.walk
  functions.walk
  arrays.walk
  modules.walk

tests/fail/
  type_mismatch.walk
  unknown_name.walk
  bad_indent.walk
  private_module_func.walk

tests/snapshots/
  hello.c
  functions.c
```

v1.1 done when:

```text
every stable feature has spec text
every stable feature has tests
known invalid programs fail with expected diagnostics
generated C is predictable enough for snapshot testing
README clearly says what version is implemented
```

---

## v1.2 Project Mode

Make WalkLang useful for more than single-file experiments.

Add:

```text
walk init
walk build
walk check
walk test
walk fmt
walk clean
project config file
multi-file project builds
examples as testable fixtures
official release binaries
GitHub Actions CI
```

Possible project file:

```toml
name = "hello"
version = "0.1.0"
entry = "src/main.walk"

[build]
output = "build/hello"
release = false
```

Project layout target:

```text
hello/
  walk.toml
  src/
    main.walk
    math_extra.walk
  tests/
    main_test.walk
  build/
```

Command target:

```bash
walk init hello
cd hello
walk build
walk test
walk fmt
```

v1.2 done when:

```text
a stranger can install walk
create a project
write source
build it
test it
format it
and understand failures
```

---

## v1.3 Standard Library Foundation

Grow the standard library carefully without making it huge.

Stable first:

```text
math.sqrt
math.pow

string.len

array.len

time.now

random.int

testing.assert
```

Draft next:

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

Rules for stdlib APIs:

```text
small names
clear behavior
no hidden global state unless documented
no magic conversions
errors must be predictable
APIs must have examples and tests
```

Example:

```walk
imp: string
imp: array

var: name = 'Walker'
var: nums = [1, 2, 3]

out: string.len(name)
out: array.len(nums)
```

v1.3 done when:

```text
STDLIB.md documents every stable function
stdlib tests run in CI
draft APIs are clearly marked
stdlib naming is consistent across modules
```

---

## v1.4 Diagnostics and Developer Experience

Make errors feel professional.

Improve diagnostics:

```text
file name
line number
column number
error category
clear message
source snippet
caret location
suggested fix when obvious
```

Target diagnostic:

```text
main.walk:3:14: type error: expected int, got string

var: age int = 'old'
             ^ string cannot initialize int
```

Warning examples:

```text
shadowed name
unused variable
unused import
unreachable code
missing return
deprecated API
```

Warning modes:

```text
--warnings=off
--warnings=default
--warnings=error
```

v1.4 done when:

```text
common mistakes produce clear messages
tests verify important diagnostics exactly
warnings help without feeling noisy
walk check is useful before build
```

---

## v1.5 Compatibility Release

Prepare for a real stable release.

Add:

```text
versioned language surface
release notes
migration guide
deprecation policy
compatibility test suite
official install instructions
```

Compatibility rule:

```text
Stable v1 code should keep compiling through the v1.x line unless a documented safety fix requires a breaking change.
```

Version meanings:

```text
v1.0.x: bug fixes only
v1.x.0: compatible improvements
v2.0.0: breaking language changes allowed
```

v1.5 done when:

```text
WalkLang can promise basic compatibility
releases are repeatable
users can tell stable features from experimental features
breaking changes require a version boundary
```

---

## v1.6 Local Function Type Inference

Make obvious helper functions short without hiding type decisions at call sites.

Add:

```text
omitted parameter types inferred from local body evidence
omitted return types inferred from value returns
clear errors for ambiguous omitted parameter types
no ordinary function inference from later call sites
```

Example:

```walk
func: power_four(n)
    return: ^ n 4
```

v1.6 done when:

```text
inferred helpers build through generated C
ambiguous helpers ask for annotations
SPEC, SYNTAX, migration, release notes, and status docs describe the rule
compatibility tests continue to pass
```

---

## v1.7 Required-Line Input

Make basic stdin input a small, stable language primitive that pairs cleanly with `out:`.

Add:

```text
core in: expression
optional string prompt expression
stdin-only required-line behavior
stdout prompt flush before reading
empty line returns ''
CRLF and LF line ending stripping
final unterminated input returns as a line
immediate EOF and stdin errors runtime-stop clearly
```

Example:

```walk
var: name = in: 'Name? '
out: name
```

v1.7 done when:

```text
in: parses anywhere expressions are valid
prompt expressions type-check as string
generated C reads stdin without a fixed line limit
compatibility tests cover prompt, pipes, empty lines, CRLF, EOF, and final unterminated input
SPEC, SYNTAX, migration, release notes, and status docs describe the rule
```

---

## v1.8 Terminal Game Helpers

Add small helpers needed for simple terminal games without changing WalkLang's statement model or adding broad runtime systems.

Stable additions:

```text
string.at
string.contains
string.concat
string indexing with word[0]
array.contains
array.push returning a new array
empty arrays with explicit array annotations
random.choice
```

Example:

```walk
imp: string
imp: array
imp: random

var: words = ['dog', 'cat']
var: word = random.choice(words)
var: guessed array[string] = []

guessed = array.push(guessed, 'd')
out: string.contains(word, guessed[0])
```

v1.8 done when:

```text
SPEC and STDLIB document the helpers
pass/fail/compatibility fixtures cover the helpers
a small Hangman playground program compiles
the v1 stress script proves the helpers through native output
```

---

## v1.9 String Interpolation

Add display-friendly string interpolation without changing numeric `+` or making
`string.concat` accept mixed scalar types.

Stable addition:

```text
'text {expression}'
```

Example:

```walk
imp: string

var: word = 'paddle'
var: wordLength = string.len(word)
out: 'the secret word is {wordLength} characters long'
```

v1.9 done when:

```text
SPEC and SYNTAX document interpolation
pass/fail/compatibility fixtures cover interpolation
the v1 stress script proves interpolation through native output
```

---

## v2 Data Modeling

Add structs after the v1 language contract is solid.

Target syntax:

```walk
struct: User
    name string
    age int

var: user = User('Walker', 25)
out: user.name
```

Struct rules:

```text
fields have fixed types
field access uses dot syntax
struct values have named types
constructors are predictable
missing fields are compile errors
unknown fields are compile errors
```

Possible named-field constructor:

```walk
var: user = User(
    name: 'Walker',
    age: 25
)
```

No inheritance.

Preferred model:

```text
structs first
methods later
traits maybe later
inheritance no
```

v2 done when:

```text
struct declarations compile
struct values can be created
fields can be read
fields can be assigned when mutable
structs work in arrays
structs work across modules
diagnostics explain field errors
```

---

## v2.1 Methods

Add methods without turning WalkLang into class-based OOP.

Target syntax:

```walk
struct: User
    name string
    age int

func: User.is_adult(self User) bool
    return: >= self.age 18

var: user = User('Walker', 25)

if: user.is_adult()
    out: 'adult'
```

Method rules:

```text
methods are functions with receiver syntax
methods do not imply inheritance
methods do not hide state
method calls compile predictably to functions
```

Possible emitted shape:

```text
user.is_adult() -> User__is_adult(user)
```

v2.1 done when:

```text
methods work on structs
method names are namespaced by type
method calls have clear type checking
methods remain explainable as functions
```

---

## v2.2 Stronger Composition

Add composition features only if structs and methods prove useful.

Possible features:

```text
traits
interfaces
protocol-like behavior
generic functions
generic arrays/functions
```

Preferred order:

```text
1. structs
2. methods
3. simple generics
4. traits or interfaces
```

Non-goal:

```text
inheritance
deep class hierarchies
magic dispatch
complex metaprogramming
```

Trait target, if accepted later:

```walk
trait: Printable
    func: print(self) string
```

v2.2 done when:

```text
composition solves real examples
syntax remains readable
type errors remain understandable
generated C remains predictable
```

---

## v3 Package Ecosystem

Add packages only after local modules are stable.

Add:

```text
package manifest
package versioning
dependency resolver
package cache
package publish flow
package docs
package tests
```

Possible package file:

```toml
name = "geometry"
version = "0.1.0"

[dependencies]
```

Import target:

```walk
imp: geometry.distance
```

Package rules:

```text
packages use semantic versions
package names are globally unique or namespaced
dependencies are locked
builds are reproducible
package code is tested before publish
```

v3 done when:

```text
a user can create a package
import a package
pin dependency versions
build reproducibly
publish documented library code
```

---

## v4 Professional Tooling

Make WalkLang comfortable in editors and larger projects.

Add:

```text
language server
syntax highlighting
formatter integration
diagnostics in editor
go-to-definition
hover docs
rename symbol
project-wide check
debugger foundation
MVP docs generator
```

Editor targets:

```text
VS Code extension
Neovim support
JetBrains syntax support later
```

LSP features:

```text
document diagnostics
format document
hover type info
go to definition
find references
completion
```

v4 done when:

```text
WalkLang feels usable in a normal editor
errors appear while editing
formatter works from editor
users can navigate larger projects
```

---

## v4.1 Documentation Generator Hardening

Make the v4 docs generator useful enough to support generated reference docs without doing a broad docs overhaul before v5 runtime truth is stable.

Add:

```text
structured /// doc comments above public symbols
docs.json output for tools and future sites
Markdown rendered from the same docs index
opt-in strict checks for missing public symbol docs
2-3 existing symbols documented end-to-end
```

Keep this slice narrow. Do not rewrite the whole docs set until v5 runtime and backend behavior have been clarified.

v4.1 done when:

```text
walk docs still generates Markdown by default
walk docs --format json writes the machine-readable docs index
walk docs --strict fails on undocumented public symbols
the v1 example module proves structured comments end-to-end
```

---

## v5 Runtime and Backend Maturity

Improve the generated output and runtime model.

Possible work:

```text
runtime library cleanup
better memory ownership rules
faster generated C
debuggable generated C
optimized release builds
portable runtime layer
alternative backend research
```

Keep C backend as the primary target unless there is a strong reason to change.

Backend rule:

```text
Generated output must remain predictable, portable, and debuggable.
```

Possible future backend targets:

```text
C primary
LLVM later maybe
WASM later maybe
```

v5 done when:

```text
release builds are meaningfully optimized
runtime behavior is documented
memory behavior is predictable
generated C remains understandable enough to inspect
```

---

## v5.1 Public Docs and Reference Site

Publish the documentation and generated reference surface as a static site.

Add:

```text
repo-owned static site build
public docs root
generated API Markdown and JSON artifacts
generated reference page
static-site link check
GitHub Pages deployment workflow
custom-domain CNAME
```

Keep this slice focused on publishing docs and reference material. Do not add
networking, a playground, a package index, or a compiler explorer here.

v5.1 done when:

```text
scripts/build-docs-site.sh regenerates docs/reference and public site files
scripts/check-docs-site.sh verifies generated docs and static links
generated reference docs use publishable repo-relative source paths
public/docs/index.html is the hosted docs front door
public/docs/reference/api.html renders the generated API reference
CI runs the docs-site check before release artifacts
Pages workflow can deploy public/ from main
```

---

## Later Ecosystem

Later tools and package/runtime tracks:

```text
debugger
server networking
project templates
playground
online package index
formatter service
compiler explorer
```

Draft HTTP client example:

```walk
imp: http # draft only
```

Networking target:

```walk
imp: http

var: response = http.get('https://example.com')
out: response.body
```

Draft HTTP client helpers are implemented in v5.12.0. Broader networking and
server/runtime work still requires:

```text
custom header/body design
server package design
async/event-loop decisions
package/runtime boundary decisions
```

---

## Advanced Future Ideas

These are not promises.

Possible later features:

```text
closures
anonymous functions
pattern matching
async
concurrency
error values
result types
generic structs
traits
WASM backend
self-hosted compiler experiments
```

Hold these until the simple language is strong.

Rejected until proven necessary:

```text
inheritance
macro system
operator overloading
complex implicit conversions
reflection-heavy runtime
magic package behavior
```

---

## Design Rule For Every Future Feature

Every feature must pass this test:

```text
Can it be explained with one small example?
Does it keep code readable at a glance?
Does it avoid magic?
Does it compile predictably to C/native output?
Can it produce clear errors?
Can it be tested?
Can it be documented without special pleading?
```

Example accepted:

```walk
const: max_users = 100
```

Example rejected:

```text
a clever shortcut that hides behavior
```

---

## Stability Labels

Every feature should have a stability label.

```text
experimental
  may change anytime

draft
  likely shape, but may still change

stable
  part of the compatibility promise

deprecated
  still works, but should be replaced

removed
  no longer accepted
```

Example:

```text
structs: experimental in v2
methods: experimental in v2.1
generic functions: experimental in v2.2
basic modules: stable in v1
```

---

## Release Principles

Each release should answer:

```text
What changed?
What broke?
What was added?
What was removed?
What is still experimental?
How does a user upgrade?
```

Release artifacts:

```text
walk-linux-amd64
walk-linux-arm64
walk-darwin-amd64
walk-darwin-arm64
walk-windows-amd64.exe
checksums
release notes
```

Install goal:

```bash
walk version
walk build examples/hello.walk -o build/hello
./build/hello
```

---

## Real Language Definition

WalkLang becomes a real language when this is true:

```text
A stranger can install walk,
read the docs,
write a .walk file,
compile it,
understand errors,
run tests,
format code,
build a small project,
and trust that stable code will not randomly break.
```

---

## Version Summary

```text
v0
  compile .walk to native exe
  core syntax + types + functions + loops

v0.1
  tests + REPL + diagnostics

v1
  stable modules + stdlib + docs + build flow

v1.1
  complete language contract + conformance tests

v1.2
  project mode + release/install workflow

v1.3
  standard library foundation

v1.4
  professional diagnostics + developer experience

v1.5
  compatibility release preparation

v1.6
  local function type inference

v1.7
  required-line stdin input

v1.8
  terminal game helpers

v2
  structs + data modeling

v2.1
  methods

v2.2
  stronger composition with simple generic functions

v3
  package ecosystem

v4
  LSP + editor tooling + debugger foundation + MVP docs generator

v4.1
  structured docs generator hardening

v5
  runtime/backend maturity

v5.1
  public docs + generated reference site

v5.12
  draft HTTP/HTML helpers + rich-runtime boundaries

v5.12.1
  generated docs search

later
  server networking + package/runtime tracks + playground + advanced features
```
