# WalkLang Official Install Instructions

WalkLang supports two install paths:

```text
release artifact install
source install for local development
```

Use `walk version` after either path. The reported version should match the release or local build label you installed.

## Release Artifact Install

Release artifacts are produced by `scripts/release.sh` and named:

```text
walk-v5.14.1-darwin-arm64
walk-v5.14.1-darwin-amd64
walk-v5.14.1-linux-amd64
walk-v5.14.1-linux-arm64
walk-v5.14.1-windows-amd64.exe
walk-cpp-v5.14.1-<release-host-os>-<release-host-arch>
walk-runtime-v5.14.1.tar.gz
walktop-v5.14.1-<release-host-os>-<release-host-arch>
SHA256SUMS
```

`scripts/release.sh` cross-builds the `walk` compiler artifacts and builds one
current-host `walk-cpp` port-candidate artifact plus one current-host `walktop`
artifact from WalkLang source. It also packages the C runtime source used by
native builds. Source install always installs the runtime source and builds the
local `walk-cpp` and `walktop` binaries.

Install on macOS or Linux:

```bash
mkdir -p ~/.local/bin
cp walk-v5.14.1-<os>-<arch> ~/.local/bin/walk
cp walk-cpp-v5.14.1-<os>-<arch> ~/.local/bin/walk-cpp
cp walktop-v5.14.1-<os>-<arch> ~/.local/bin/walktop
mkdir -p ~/.local/lib/walk
tar -xzf walk-runtime-v5.14.1.tar.gz -C ~/.local/lib/walk
chmod +x ~/.local/bin/walk ~/.local/bin/walk-cpp ~/.local/bin/walktop
walk version
walk-cpp version
NO_COLOR=1 walktop --once
```

Install on Windows by placing `walk-v5.14.1-windows-amd64.exe` somewhere on
`PATH` as `walk.exe`, then run:

```powershell
walk version
```

Verify checksums before installing:

```bash
shasum -a 256 -c SHA256SUMS
```

On systems with GNU coreutils, this also works:

```bash
sha256sum -c SHA256SUMS
```

## Source Install

Source installs require Go, `make`, a C++20 compiler, and a native C compiler
available as `cc`.

```bash
scripts/install-local.sh local
walk version
walk-cpp version
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
```

By default the script writes `walk`, `walk-cpp`, and `walktop` to
`~/.local/bin`. Override the install directory with:

```bash
WALK_INSTALL_DIR=/path/to/bin scripts/install-local.sh local
```

The runtime source is installed to `../lib/walk/runtime` relative to the install
directory. Override it with:

```bash
WALK_RUNTIME_INSTALL_DIR=/path/to/runtime scripts/install-local.sh local
```

`walk-cpp` is the systems compiler port candidate. As of Phase 7 it supports
single-file `check`, `emit-c`, `build`, `run`, and `test`, plus project mode
and local package commands: `init`, `fmt`, `clean`, project `check/build/test`,
and `package init/resolve/publish`. Later docs, debug-map, LSP, and REPL
commands still return a `not ported in this phase` diagnostic instead of
delegating to the Go reference compiler.

Maintainers can force source install to build `walktop` through the C++ port
candidate:

```bash
WALK_BUILD_BIN=build/walk-cpp scripts/install-local.sh local
```

When running an installed `walk-cpp` from outside the source tree, set
`WALK_RUNTIME_DIR` if the runtime source is not under the default local install
location:

```bash
WALK_RUNTIME_DIR=~/.local/lib/walk/runtime walk-cpp build src/main.walk -o build/app
```

## Smoke Test

After install:

```bash
walk run playground/route_ranker.walk
walk playground/route_ranker.walk
walk build examples/hello.walk -o build/hello
./build/hello
walk check --warnings=error examples/stable.walk
walk test examples/compiler_tests.walk
walk build tests/pass/structs.walk -o build/structs
walk build tests/pass/methods.walk -o build/methods
walk build tests/pass/generics.walk -o build/generics
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

## Build Release Artifacts Locally

Maintainers can produce the release artifact set with:

```bash
scripts/release.sh v5.14.1 dist
```

The command writes the compiler platform binaries, the runtime source archive,
the current-host `walk-cpp` port-candidate binary, the current-host `walktop`
binary, and `dist/SHA256SUMS`.

Maintainers can force release artifact generation to build `walktop` through
the C++ port candidate:

```bash
WALK_RELEASE_BUILD_BIN=build/walk-cpp scripts/release.sh v5.14.1 dist
```
