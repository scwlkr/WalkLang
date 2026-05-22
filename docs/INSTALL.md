# WalkLang Official Install Instructions

WalkLang v5.0.0 supports two install paths:

```text
release artifact install
source install for local development
```

Use `walk version` after either path. The reported version should match the release or local build label you installed.

## Release Artifact Install

Release artifacts are produced by `scripts/release.sh` and named:

```text
walk-v5.0.0-darwin-arm64
walk-v5.0.0-darwin-amd64
walk-v5.0.0-linux-amd64
walk-v5.0.0-linux-arm64
walk-v5.0.0-windows-amd64.exe
SHA256SUMS
```

Install on macOS or Linux:

```bash
mkdir -p ~/.local/bin
cp walk-v5.0.0-<os>-<arch> ~/.local/bin/walk
chmod +x ~/.local/bin/walk
walk version
```

Install on Windows by placing `walk-v5.0.0-windows-amd64.exe` somewhere on `PATH` as `walk.exe`, then run:

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

Source installs require Go and a native C compiler available as `cc`.

```bash
scripts/install-local.sh v5-local
walk version
```

By default the script writes to `~/.local/bin/walk`. Override the install directory with:

```bash
WALK_INSTALL_DIR=/path/to/bin scripts/install-local.sh v5-local
```

## Smoke Test

After install:

```bash
walk run playground/route_ranker.walk
walk playground/route_ranker.walk
walk build examples/hello.walk -o build/hello
./build/hello
walk check --warnings=error examples/v1.walk
walk test examples/v0_1_tests.walk
walk build tests/pass/structs.walk -o build/structs
walk build tests/pass/methods.walk -o build/methods
walk build tests/pass/generics.walk -o build/generics
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
Library Lane
Score:
46
```

## Build Release Artifacts Locally

Maintainers can produce the release artifact set with:

```bash
scripts/release.sh v5.3.0 dist
```

The command writes the platform binaries and `dist/SHA256SUMS`.
