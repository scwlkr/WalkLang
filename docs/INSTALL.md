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
walktop-v5.14.1-<release-host-os>-<release-host-arch>
SHA256SUMS
```

`scripts/release.sh` cross-builds the `walk` compiler artifacts and builds one
current-host `walktop` artifact from WalkLang source. Source install always
builds the local `walktop` binary.

Install on macOS or Linux:

```bash
mkdir -p ~/.local/bin
cp walk-v5.14.1-<os>-<arch> ~/.local/bin/walk
cp walktop-v5.14.1-<os>-<arch> ~/.local/bin/walktop
chmod +x ~/.local/bin/walk ~/.local/bin/walktop
walk version
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

Source installs require Go and a native C compiler available as `cc`.

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

The command writes the compiler platform binaries, the current-host `walktop`
binary, and `dist/SHA256SUMS`.
