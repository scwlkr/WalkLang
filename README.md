# WalkLang

WalkLang is a small compiled language implemented in Go with a C backend.

This repo currently contains the v1 compiler path from `docs/ROADMAP.md`.
It proves the pipeline:

```text
.walk source -> Go compiler -> generated C -> native executable
```

## Current v1 Surface

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
- `math.sqrt` and `random.int`
- formatter command
- test runner with `test:` and `assert:`
- basic expression REPL
- line/column diagnostics
- stdlib polish for `math.pow`, `string.len`, `array.len`, and `time.now`
- `walk check` with `--warnings=off|default|error`
- release native builds with `--release`, `--cc`, and `--cflag`
- cross-platform CLI release script

## Use

Emit C:

```bash
go run ./cmd/walk emit-c examples/hello.walk -o build/hello.c
```

Build a native executable:

```bash
go run ./cmd/walk build examples/hello.walk -o build/hello
./build/hello
```

Run the v0 representative program:

```bash
go run ./cmd/walk build examples/v0.walk -o build/v0
./build/v0
```

Run v0.1 tests:

```bash
go run ./cmd/walk test examples/v0_1_tests.walk
```

Run the v1 module example:

```bash
go run ./cmd/walk build examples/v1.walk -o build/v1 --release
./build/v1
```

Check warnings:

```bash
go run ./cmd/walk check examples/v1.walk
go run ./cmd/walk check --warnings=error examples/v1.walk
```

Open the REPL:

```bash
go run ./cmd/walk repl
```

Format a file:

```bash
go run ./cmd/walk fmt examples/hello.walk
```

Run tests:

```bash
go test ./...
```

Build cross-platform CLI release artifacts:

```bash
scripts/release.sh v1
```
