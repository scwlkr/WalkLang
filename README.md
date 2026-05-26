# WalkLang

![WalkLang: a small compiled language with deterministic syntax](logo_WalkLang.svg)

[Getting started] | [Learn] | [Documentation] | [Contributing]

This repository contains the WalkLang compiler, language specification,
examples, tests, documentation, generated reference docs, and release tooling.

WalkLang compiles `.walk` source through generated C into native executables:

```text
.walk source -> C++ compiler -> generated C + Walk C runtime -> native executable
```

A small WalkLang program looks like this:

```walk
func: add(a, b)
    return: + a b

out: add(2, 3)
```

[Getting started]: docs/INSTALL.md
[Learn]: docs/README.md
[Documentation]: docs/README.md
[Contributing]: CONTRIBUTING.md

## Why WalkLang?

- **Predictability:** indentation-based syntax, explicit module boundaries, and
  a stable feature specification keep programs easy to read and reason about.
- **Native output:** WalkLang emits understandable C, then uses the system C
  compiler to build native executables.
- **Tooling:** project mode, tests, formatting, diagnostics, package workflows,
  editor support, API docs generation, and release scripts are part of the repo.

## Current Version

WalkLang is currently `v6.0.0-port-candidate`.

`v6.0.0-port-candidate` promotes the C++/C systems compiler port to the
repo-local `walk` binary and removes the Go reference implementation and
JavaScript docs assets. Features inside a release are labeled `stable`,
`draft`, `experimental`, or `planned` when their maturity matters.

The first CLI standard-platform slice remains `walktop`, an official
standalone WalkLang-built system monitor installed by the normal local install
flow.

## What Works Today

- `.walk` files compile through generated C into native executables.
- Stable syntax, diagnostics, modules, tests, and standard-library helpers are
  documented and compatibility-tested.
- Draft `do:`, `defer:`, `io`, `parse`, `process`, `file`, `dir`, `path`,
  `json`, `term`, `http`, and `html` helpers are implemented for current
  compiler experiments.
- Experimental structs, methods, and simple generic functions are implemented
  for current compiler experiments.
- `walk run`, direct `walk file.walk`, `walk build`, `walk check`, `walk test`,
  `walk fmt`, `walk clean`, `walk package`, `walk docs`, `walk debug-map`,
  `walk lsp`, `walk repl`, and `walk version` are implemented.
- `walktop` builds from `tools/walktop/src/main.walk` into a standalone native
  command with deterministic fixture mode and live OS-command mode.
- Project mode supports `walk init`, `walk.toml`, source/test layout, builds,
  checks, tests, formatting, explicit debug/release build modes, and cleanup.
- Static docs and generated API reference output are repo-owned and deployed
  through GitHub Pages; live custom-domain HTTPS state is tracked in
  [STATUS.md](docs/STATUS.md).

## Quick Start

Read [the install guide][Getting started], then run a `.walk` file directly:

```bash
scripts/install-local.sh local
walk run playground/route_ranker.walk
walk playground/route_ranker.walk
```

Then build a small project:

```bash
walk init hello
cd hello
walk check
walk build
./build/hello
walk test
```

## Installing From Source

If you want to install from this repository, see [INSTALL.md](docs/INSTALL.md).

## Getting Help

Start with the docs index at [docs/README.md](docs/README.md). The public docs
site is built from `docs/` and `public/` in this repository and configured for
`walklang.wlkrlabs.com/docs`. Until public community channels exist, use this
repository's issue tracker for bugs and design questions.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

For a project overview and implementation direction, see
[ARCHITECTURE.md](docs/ARCHITECTURE.md), [ROADMAP.md](docs/ROADMAP.md), and
[STATUS.md](docs/STATUS.md).

## License

WalkLang source code is distributed under the terms of the Apache License 2.0.

See [LICENSE](LICENSE) for details.

## Trademark

The WalkLang name, logo, and brand identity are protected project marks owned by
Shane Walker / WLKRLABS.

If you want to use the WalkLang name, logo, or brand identity, read
[TRADEMARKS.md](TRADEMARKS.md).
