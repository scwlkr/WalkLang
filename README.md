# WalkLang

WalkLang is a small compiled language implemented in Go with a C backend.

This repo currently contains the initial v0 compiler tracer bullet from
`docs/ROADMAP.md`. It proves the pipeline:

```text
.walk source -> Go compiler -> generated C -> native executable
```

## Current Compiler Slice

Supported now:

- top-level `var:`, `const:`, assignment, and `out:` statements
- `int`, `float`, `bool`, and `string` literals
- inferred and explicit basic types
- prefix `+`, `-`, `*`, `/`, `^`
- prefix comparisons and boolean operators
- generated C and native build through `cc`

Not implemented yet:

- indentation blocks
- `if`, loops, functions, arrays, imports, exports, nullable values, and the formatter
- the v0 standard library

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

Run tests:

```bash
go test ./...
```
