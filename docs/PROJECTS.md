# WalkLang Project Mode

Project mode starts when a directory contains `walk.toml`.

The active C++/C `walk` compiler supports the project workflow below.

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
mode = "debug"
```

Rules:

- `name` is required and may contain letters, numbers, `_`, and `-`.
- `version` defaults to `0.1.0`.
- `entry` defaults to `src/main.walk`.
- `build.output` defaults to `build/<name>`.
- `build.mode` may be `debug` or `release`; the default is `debug`.
- `build.release = true|false` is still accepted for compatibility. When both
  `build.mode` and `build.release` are present, `build.mode` wins and the
  compiler emits a warning.
- project paths must be relative and stay inside the project root.
- `[dependencies]` pins package dependencies by exact `MAJOR.MINOR.PATCH` version.

Package dependency example:

```toml
[dependencies]
geometry = "0.1.0"
```

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
walk run examples/hello.walk
walk examples/hello.walk
walk build examples/hello.walk -o build/hello
walk test examples/compiler_tests.walk
walk fmt examples/hello.walk
```

`walk run <source.walk>` compiles the file to a temporary native executable,
runs it, streams program input and output, and removes the temporary build
directory. Use `--` to pass arguments through to the program:

```bash
walk run examples/hello.walk -- alpha beta
```

`walk <source.walk>` is the shorthand for the same flow and accepts the same
`--` passthrough.

## Example Projects

`examples/tinychain/` is a small blockchain-style project with `walk.toml`,
`src/`, tests, and a local README. It shows structs, arrays of structs, module
exports, string interpolation, project tests, and a deterministic proof loop.
It also records useful language gaps exposed by the exercise: real hashing,
remainder, struct-array append, multiline arrays, stable persistence, byte
arrays, and command arguments.

Run it from the repository root with:

```bash
make walk
cd examples/tinychain
../../build/walk test
../../build/walk run src/main.walk
```

## Module Search

Project tests can import modules from `src/`.

```walk
imp: math_extra

test: 'cube works'
    assert: == math_extra.cube(3) 27
```

The importing file's directory is searched first. The project entry directory is searched next.

Package dependencies add locked package `src/` directories after local project search paths. Package imports use dotted module names:

```walk
imp: geometry.core

out: geometry.core.double(3)
```

The first segment of a dotted package import is the package collection root.
For `imp: geometry.core`, the collection root is `geometry`, the module path is
`geometry/core.walk`, and generated C names use the prefix
`geometry__core__`.

This import resolves from:

```text
.walk/packages/geometry/0.1.0/src/geometry/core.walk
```

## Packages

Create a package project:

```bash
walk package init geometry
```

Publish a documented package to a local registry:

```bash
cd geometry
walk package publish ../registry
```

Resolve an app's pinned dependencies from that registry:

```bash
cd ../shape_app
walk package resolve ../registry
```

Package behavior:

- `walk package publish` requires non-empty `README.md`.
- `std` is reserved as a future first-party collection root.
- `walk package publish` rejects package names reserved for current or future
  built-in roots, including `std` and current built-in module roots.
- Publish runs `walk check --warnings=error` and `walk test --warnings=error`.
- Published packages are copied to `<registry>/<name>/<version>/`.
- `walk package resolve` copies packages into `.walk/packages/` and writes `walk.lock`.
- `walk check`, `walk test`, and `walk build` verify package cache checksums before using dependencies.
- Build commands do not download packages automatically.
