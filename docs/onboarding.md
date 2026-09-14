# WalkLang Onboarding Guide

This guide helps new contributors set up their environment, build the compiler,
run tests, and make their first contribution.

## Environment

WalkLang compiles `.walk` source through generated C into native executables.
The compiler itself is written in C++ and requires:

- `make`
- A C++20 compiler
- A native C compiler available as `cc`

The current release is `v6.4.1`. See [docs/STATUS.md](STATUS.md) for the
current state and [docs/ROADMAP.md](ROADMAP.md) for direction.

## Install

Two install paths are supported. See [docs/INSTALL.md](INSTALL.md) for the
full instructions.

### Release artifact install

The v6.4.1 release provides prebuilt binaries for **macOS Apple Silicon
only**. Other systems should use the source install below.

```bash
# Download and install (macOS Apple Silicon)
(
  cd "$(mktemp -d)"
  curl -fLO https://github.com/scwlkr/WalkLang/releases/download/v6.4.1/SHA256SUMS
  curl -fLO https://github.com/scwlkr/WalkLang/releases/download/v6.4.1/walk-v6.4.1-darwin-arm64
  curl -fLO https://github.com/scwlkr/WalkLang/releases/download/v6.4.1/walktop-v6.4.1-darwin-arm64
  curl -fLO https://github.com/scwlkr/WalkLang/releases/download/v6.4.1/walk-runtime-v6.4.1.tar.gz
  shasum -a 256 -c SHA256SUMS
  mkdir -p ~/.local/bin ~/.local/lib/walk
  cp walk-v6.4.1-darwin-arm64 ~/.local/bin/walk
  cp walktop-v6.4.1-darwin-arm64 ~/.local/bin/walktop
  tar -xzf walk-runtime-v6.4.1.tar.gz -C ~/.local/lib/walk
  chmod +x ~/.local/bin/walk ~/.local/bin/walktop
)
export PATH="$HOME/.local/bin:$PATH"
walk version
NO_COLOR=1 walktop --once
```

### Source install

```bash
git clone https://github.com/scwlkr/WalkLang.git
cd WalkLang
scripts/install-local.sh local
export PATH="$HOME/.local/bin:$PATH"
walk version
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
```

By default the script writes `walk` and `walktop` to `~/.local/bin`. Override
with `WALK_INSTALL_DIR` and `WALK_RUNTIME_INSTALL_DIR` as needed.

## Build The Compiler

From the repository root:

```bash
make clean
make walk WALK_VERSION=dev
```

This produces `build/walk`, the development compiler binary.

## Run Tests

The standard test suite:

```bash
make test
make conformance
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
```

Smoke test after install:

```bash
walk run playground/route_ranker.walk
walk build examples/hello.walk -o build/hello
./build/hello
walk check --warnings=error examples/stable.walk
walk test examples/compiler_tests.walk
walk build tests/pass/structs.walk -o build/structs
walk build tests/pass/methods.walk -o build/methods
walk build tests/pass/generics.walk -o build/generics
(cd examples/tinychain && ../../build/walk test && ../../build/walk run src/main.walk)
NO_COLOR=1 walktop --once
```

Expected `examples/hello.walk` output:

```text
3
hello from WalkLang
true
```

Expected `playground/route_ranker.walk` output:

```text
Best route:
Market Mile
Score:
28
```

## Documentation Checks

Docs are part of the product. Before shipping docs changes:

```bash
git diff --check
scripts/check-docs-site.sh
```

For docs that mention generated API output, also prove the command works:

```bash
make walk
./build/walk docs --strict -o build/api.md examples/stable.walk
./build/walk docs --strict --format json -o build/api.json examples/stable.walk
scripts/check-docs-site.sh
```

## Contributing

Read [CONTRIBUTING.md](../CONTRIBUTING.md) before making changes. Key points:

1. **Read first:** [docs/STATUS.md](STATUS.md), [docs/ROADMAP.md](ROADMAP.md),
   and [docs/DESIGN_RULES.md](DESIGN_RULES.md).
2. **Keep changes narrow.** If a feature is experimental, document that
   boundary instead of presenting it as stable.
3. **Run the full check suite** before sending changes:

   ```bash
   make clean
   make walk WALK_VERSION=dev
   make test
   make conformance
   WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
   scripts/check-docs-site.sh
   git diff --check
   ```

4. **Update docs with code changes.** Follow
   [docs/DOCS_STYLE_GUIDE.md](DOCS_STYLE_GUIDE.md). Generated API reference
   docs must come from `walk docs`; do not hand-write files that should be
   generated from structured comments.
5. **Preserve compatibility.** Stable v1 behavior is covered by
   [docs/COMPATIBILITY.md](COMPATIBILITY.md) and the compatibility fixtures
   under `tests/compat/`. Do not break stable syntax, standard-library
   behavior, or CLI expectations without a migration note and an explicit
   compatibility decision.

## Key Documentation

| Page | Purpose |
|------|---------|
| [docs/INSTALL.md](INSTALL.md) | Full install instructions |
| [docs/README.md](README.md) | Docs front door and map |
| [docs/SPEC.md](SPEC.md) | Stable feature specification |
| [docs/SYNTAX.md](SYNTAX.md) | Language syntax |
| [docs/STDLIB.md](STDLIB.md) | Standard library |
| [docs/PROJECTS.md](PROJECTS.md) | Project mode and packages |
| [docs/TOOLING.md](TOOLING.md) | Editor tooling, LSP, docs generation |
| [docs/ARCHITECTURE.md](ARCHITECTURE.md) | Architecture overview |
| [docs/DESIGN_RULES.md](DESIGN_RULES.md) | Design rules |
| [docs/COMPATIBILITY.md](COMPATIBILITY.md) | Compatibility promise |
| [docs/DOCS_STYLE_GUIDE.md](DOCS_STYLE_GUIDE.md) | Docs writing rules |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | Contribution guidelines |

## Getting Help

The public docs site is at `walklang.wlkrlabs.com/docs`. Until public
community channels exist, use this repository's issue tracker for bugs and
design questions.
