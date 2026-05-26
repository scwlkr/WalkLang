# Contributing To WalkLang

WalkLang is early, but contributions should still preserve the project contract:
small language surface, predictable C output, clear diagnostics, and docs that
match the shipped compiler.

## Before You Change Code

Read the current status and roadmap:

- [docs/STATUS.md](docs/STATUS.md)
- [docs/ROADMAP.md](docs/ROADMAP.md)
- [docs/DESIGN_RULES.md](docs/DESIGN_RULES.md)

Keep changes narrow. If a feature is experimental, document that boundary instead
of presenting it as stable.

## Local Verification

Run the standard checks before sending changes:

```bash
go test -count=1 ./...
go build -o build/walk ./cmd/walk
WALK_BIN=$PWD/build/walk scripts/stress-compatibility.sh
git diff --check
```

For release artifact work, also run:

```bash
scripts/release.sh <version> dist
```

## Documentation

Docs are part of the product. Update the relevant page in `docs/` with the code
change, and follow [docs/DOCS_STYLE_GUIDE.md](docs/DOCS_STYLE_GUIDE.md).

Generated API reference docs must come from `walk docs`; do not hand-write files
that should be generated from structured comments.

## Compatibility

Stable v1 behavior is covered by [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md)
and the compatibility fixtures under `tests/compat/`. Do not break stable syntax,
standard-library behavior, or CLI expectations without a migration note and an
explicit compatibility decision.
