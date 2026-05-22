# WalkLang Status

Current version: v4 professional tooling.

State: v4 is complete against `docs/ROADMAP.md`: `walk lsp` provides a stdio language server with document diagnostics, formatter integration, hover, go-to-definition, references, completion, and rename support; VS Code and Neovim editor scaffolds live under `editors/`; `walk docs` generates Markdown API docs from project or single-file sources; and `walk debug-map` writes a deterministic JSON source symbol map as debugger-adapter groundwork. The v4 tooling surface is documented in `docs/V4.md`. The v3 package ecosystem remains complete and documented in `docs/V3.md` and `docs/PROJECTS.md`. Structs, methods, and generic functions remain documented as experimental v2 surface in `docs/V2.md`. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: focused `go test ./cmd/walk -run TestV4 -count=1`, roadmap slice `go test ./internal/parser ./internal/checker ./cmd/walk -run 'TestV4|TestV3|TestV22|TestV21|TestV20|TestV15|TestV14|TestFormatter|TestV13|TestV12' -count=1`, full `go test -count=1 ./...`, `go build -o build/walk ./cmd/walk`, VS Code extension syntax plus package/language/grammar JSON validation, `./build/walk docs -o build/v4-api.md examples/v1.walk`, `./build/walk debug-map -o build/v4-debug-map.json examples/v1.walk`, a real `./build/walk lsp` initialize handshake, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v4.0.0 <temp>/release`, `SHA256SUMS` line count check, host release binary `walk version`, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v5 runtime/backend maturity.
