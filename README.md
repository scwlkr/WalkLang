# WalkLang

WalkLang is a small compiled language implemented in Go with a C backend.

This repo currently contains the v0 compiler path from `docs/ROADMAP.md`.
It proves the pipeline:

```text
.walk source -> Go compiler -> generated C -> native executable
```

## Current v0 Surface

Supported now:

- `.walk` source files compiled by the Go compiler
- generated C and native build through `cc`
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
- `imp:` for built-in modules and `exp:` validation
- `math.sqrt` and `random.int`
- formatter command

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

Format a file:

```bash
go run ./cmd/walk fmt examples/hello.walk
```

Run tests:

```bash
go test ./...
```
