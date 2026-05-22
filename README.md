# WalkLang

WalkLang is a small compiled language implemented in Go with a C backend.

This repo currently contains the v4 professional tooling surface, v3 package ecosystem, v2.2 data modeling, methods, and simple generics, the v1.5 compatibility release preparation, v1.4 diagnostics and developer experience, the v1.3 standard library foundation, v1.2 project mode, and the v1 language contract from `docs/ROADMAP.md`.
It proves the pipeline:

```text
.walk source -> Go compiler -> generated C -> native executable
```

## Current Surface

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
- stable stdlib APIs: `math.sqrt`, `math.pow`, `string.len`, `array.len`, `time.now`, `random.int`, and `testing.assert`
- basic expression REPL
- professional diagnostics with line/column, source snippets, caret locations, and focused suggestions
- `walk check` with `--warnings=off|default|error`
- shadowed-name and unreachable-statement warnings
- release native builds with `--release`, `--cc`, and `--cflag`
- user-local `walk` command install
- v1 stress script
- cross-platform CLI release script
- stable language contract docs
- pass/fail conformance fixtures
- v1 compatibility fixtures
- generated C snapshot tests
- `walk init` project scaffolding
- `walk.toml` project config
- project-mode `walk build`, `walk check`, `walk test`, `walk fmt`, and `walk clean`
- project tests that can import modules from `src/`
- local package projects through `walk package init`
- pinned package dependencies in `walk.toml`
- locked package checksums in `walk.lock`
- local registry publish and resolve through `walk package publish` and `walk package resolve`
- dotted package imports such as `imp: geometry.core`
- `walk lsp` language server for editor diagnostics, formatting, hover, definition, references, completion, and rename
- VS Code extension scaffold with syntax highlighting and LSP startup
- Neovim filetype, syntax, formatter, and LSP setup files
- `walk docs` Markdown API documentation generator with structured-comment, JSON, and strict-check modes
- `walk debug-map` source symbol map for debugger-adapter groundwork
- examples covered as testable fixtures
- GitHub Actions CI for tests, stress, and release artifacts
- release notes, migration guide, deprecation policy, and official install instructions

Experimental v2 surface:

- `struct:` declarations with fixed typed fields
- positional struct constructors such as `User('Walker', 25)`
- dot field reads such as `user.name`
- field assignment through mutable roots such as `user.age = 26`
- structs as function parameters and return values
- arrays of structs, indexed struct fields, and field assignment through mutable array elements
- module-declared structs returned by exported module functions
- method declarations namespaced by receiver type, such as `func: User.is_adult(self User) bool`
- method calls on struct values, such as `user.is_adult()`
- method calls emitted as predictable receiver functions, such as `User__is_adult(user)`
- simple generic functions, such as `func: first[T](items array[T]) T`
- inferred generic calls over scalar, array, and struct values
- exported user-module generic functions

Contract docs:

- `docs/SPEC.md`
- `docs/V4.md`
- `docs/V3.md`
- `docs/V2.md`
- `docs/SYNTAX.md`
- `docs/STDLIB.md`
- `docs/ERRORS.md`
- `docs/DESIGN_RULES.md`
- `docs/COMPATIBILITY.md`
- `docs/DEPRECATION.md`
- `docs/INSTALL.md`
- `docs/MIGRATING.md`
- `docs/PROJECTS.md`
- `docs/RELEASE_NOTES.md`
- `docs/STATUS.md`

Not stable yet:

- file/json/matrix stdlib APIs
- traits
- remote public package registry behavior

## Use

Install the local `walk` command:

```bash
scripts/install-local.sh v4-local
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

Create, publish, and consume a local package:

```bash
walk package init geometry
cd geometry
walk package publish ../registry

cd ..
walk init shape_app
cd shape_app
# add [dependencies] geometry = "0.1.0" to walk.toml
walk package resolve ../registry
walk check
walk build
```

Start editor tooling:

```bash
walk lsp
```

Generate API docs and a debugger foundation map:

```bash
walk docs -o docs/api.md src/main.walk
walk docs --strict --format json -o docs/api.json src/main.walk
walk debug-map -o build/debug-map.json src/main.walk
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

Run the v2 struct fixture:

```bash
walk build tests/pass/structs.walk -o build/structs
./build/structs
```

Run the v2.1 method fixture:

```bash
walk build tests/pass/methods.walk -o build/methods
./build/methods
```

Run the v2.2 generic fixture:

```bash
walk build tests/pass/generics.walk -o build/generics
./build/generics
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

Run the focused v1 compatibility suite:

```bash
go test ./cmd/walk -run TestV15CompatibilitySuite
```

Stress v1:

```bash
scripts/stress-v1.sh
```

Build cross-platform CLI release artifacts:

```bash
scripts/release.sh v4.0.0
```
