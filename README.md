# WalkLang

WalkLang is a small compiled language implemented in Go with a C backend.

This repo currently contains the v1.3 standard library foundation, v1.2 project mode, and the v1 language contract from `docs/ROADMAP.md`.
It proves the pipeline:

```text
.walk source -> Go compiler -> generated C -> native executable
```

## Current v1.3 Surface

Supported now:

- `.walk` source files compiled by the Go compiler
- generated C and native build through `cc`
- sibling-file user modules with `imp:` / `exp:`
- indentation blocks
- `var:`, `const:`, assignment, and `out:`
- `int`, `float`, `bool`, `string`, and nullable `null` values
- inferred and explicit basic types
- prefix `+`, `-`, `*`, `/`, `^`
- prefix comparisons and boolean operators
- `if:` / `else:`
- `while:`, `repeat:`, `for:`, `break:`, and `continue:`
- `func:` / `return:`, recursion, and function values
- arrays, indexing, and array element assignment
- `imp:` for built-in and user modules with exported function validation
- formatter command
- test runner with `test:` and `assert:`
- v1.3 stable stdlib APIs: `math.sqrt`, `math.pow`, `string.len`, `array.len`, `time.now`, `random.int`, and `testing.assert`
- basic expression REPL
- line/column diagnostics
- `walk check` with `--warnings=off|default|error`
- release native builds with `--release`, `--cc`, and `--cflag`
- user-local `walk` command install
- v1 stress script
- cross-platform CLI release script
- stable language contract docs
- pass/fail conformance fixtures
- generated C snapshot tests
- `walk init` project scaffolding
- `walk.toml` project config
- project-mode `walk build`, `walk check`, `walk test`, `walk fmt`, and `walk clean`
- project tests that can import modules from `src/`
- examples covered as testable fixtures
- GitHub Actions CI for tests, stress, and release artifacts

Contract docs:

- `docs/SPEC.md`
- `docs/SYNTAX.md`
- `docs/STDLIB.md`
- `docs/ERRORS.md`
- `docs/DESIGN_RULES.md`
- `docs/COMPATIBILITY.md`
- `docs/PROJECTS.md`
- `docs/STATUS.md`

Not stable yet:

- file/json/matrix stdlib APIs
- structs, methods, traits, or package manager behavior

## Use

Install the local `walk` command:

```bash
scripts/install-local.sh v1.3-local
walk version
```

Create and build a project:

```bash
walk init hello
cd hello
walk check
walk build
./build/hello
walk test
walk fmt
walk clean
```

Emit C:

```bash
walk emit-c examples/hello.walk -o build/hello.c
```

Build a native executable:

```bash
walk build examples/hello.walk -o build/hello
./build/hello
```

Run the v0 representative program:

```bash
walk build examples/v0.walk -o build/v0
./build/v0
```

Run v0.1 tests:

```bash
walk test examples/v0_1_tests.walk
```

Run the v1 module example:

```bash
walk build examples/v1.walk -o build/v1 --release
./build/v1
```

Check warnings:

```bash
walk check examples/v1.walk
walk check --warnings=error examples/v1.walk
```

Open the REPL:

```bash
go run ./cmd/walk repl
```

Format a file:

```bash
walk fmt examples/hello.walk
```

Run tests:

```bash
go test ./...
```

Stress v1:

```bash
scripts/stress-v1.sh
```

Build cross-platform CLI release artifacts:

```bash
scripts/release.sh v1.3.0
```
