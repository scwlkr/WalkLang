# WalkLang Official Install Instructions

WalkLang supports two install paths:

```text
release artifact install
source install for local development
```

Use `walk version` after either path. The reported version should match the
release or local build label you installed.

## Release Artifact Install

Release artifacts are produced by `scripts/release.sh`. The v6.2.0 release
path is C++/C-only and writes current-host artifacts:

```text
walk-v6.2.0-<host-os>-<host-arch>
walk-runtime-v6.2.0.tar.gz
walktop-v6.2.0-<host-os>-<host-arch>
SHA256SUMS
```

The release script builds `walk` from C++ sources, packages the C runtime
source, builds `walktop` through the newly built `walk`, and checksums each
artifact.

Install on macOS or Linux:

```bash
mkdir -p ~/.local/bin ~/.local/lib/walk
cp walk-v6.2.0-<os>-<arch> ~/.local/bin/walk
cp walktop-v6.2.0-<os>-<arch> ~/.local/bin/walktop
tar -xzf walk-runtime-v6.2.0.tar.gz -C ~/.local/lib/walk
chmod +x ~/.local/bin/walk ~/.local/bin/walktop
walk version
NO_COLOR=1 walktop --once
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

Source installs require `make`, a C++20 compiler, and a native C compiler
available as `cc`.

```bash
scripts/install-local.sh local
walk version
NO_COLOR=1 walktop --once --fixture tools/walktop/testdata/basic
```

By default the script writes `walk` and `walktop` to `~/.local/bin`. Override
the install directory with:

```bash
WALK_INSTALL_DIR=/path/to/bin scripts/install-local.sh local
```

The runtime source is installed to `../lib/walk/runtime` relative to the install
directory. Override it with:

```bash
WALK_RUNTIME_INSTALL_DIR=/path/to/runtime scripts/install-local.sh local
```

Source install builds `walktop` through the installed `walk` compiler by
default. Maintainers can override the WalkLang build driver for diagnostics:

```bash
WALK_BUILD_BIN=build/walk scripts/install-local.sh local
```

When running an installed `walk` from outside the source tree, set
`WALK_RUNTIME_DIR` if the runtime source is not under the default local install
location:

```bash
WALK_RUNTIME_DIR=~/.local/lib/walk/runtime walk build src/main.walk -o build/app
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

Maintainers can produce the current-host release artifact set with:

```bash
make release VERSION=v6.2.0 OUT=dist
```

The command writes the current-host `walk` compiler binary, the runtime source
archive, the current-host `walktop` binary built by `walk`, and
`dist/SHA256SUMS`.

Release artifact generation builds `walktop` through `build/walk` by default.
Maintainers can override the WalkLang build driver for diagnostics:

```bash
WALK_RELEASE_BUILD_BIN=build/walk scripts/release.sh v6.2.0 dist
```
