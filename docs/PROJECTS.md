# WalkLang Project Mode

Project mode starts when a directory contains `walk.toml`.

Create a project:

```bash
walk init hello
cd hello
```

Generated layout:

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

## Config

`walk.toml` is intentionally small:

```toml
name = "hello"
version = "0.1.0"
entry = "src/main.walk"

[build]
output = "build/hello"
release = false
```

Rules:

- `name` is required and may contain letters, numbers, `_`, and `-`.
- `version` defaults to `0.1.0`.
- `entry` defaults to `src/main.walk`.
- `build.output` defaults to `build/<name>`.
- project paths must be relative and stay inside the project root.

## Commands

Inside a project:

```bash
walk check
walk build
walk test
walk fmt
walk clean
```

Behavior:

- `walk check` checks the entry file and `tests/*_test.walk`.
- `walk build` compiles the configured entry and writes the native executable plus generated C next to `build.output`.
- `walk test` runs every `tests/*_test.walk`.
- `walk fmt` formats `.walk` files under the entry directory and `tests/`.
- `walk clean` removes `build/` when the configured output lives there.

Single-file commands still work:

```bash
walk build examples/hello.walk -o build/hello
walk test examples/v0_1_tests.walk
walk fmt examples/hello.walk
```

## Module Search

Project tests can import modules from `src/`.

```walk
imp: math_extra

test: 'cube works'
    assert: == math_extra.cube(3) 27
```

The importing file's directory is searched first. The project entry directory is searched next.
