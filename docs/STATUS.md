# WalkLang Status

Current version: v4.1 documentation generator hardening.

State: v4.1 is a narrow pre-v5 docs-generator slice, not a broad docs overhaul. `walk docs` still generates Markdown API docs by default, now from a shared docs index; `walk docs --format json` writes the same symbol data as machine-readable JSON; `walk docs --strict` fails when generated public symbols are missing structured `///` documentation; and `examples/math_extra.walk` documents existing symbols end-to-end. v5 remains runtime/backend maturity. The v4 tooling surface is documented in `docs/V4.md`, and the structured-doc rules live in `docs/DOCS_STYLE_GUIDE.md`. The v3 package ecosystem remains complete and documented in `docs/V3.md` and `docs/PROJECTS.md`. Structs, methods, and generic functions remain documented as experimental v2 surface in `docs/V2.md`. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: focused `go test ./cmd/walk -run TestV4 -count=1`, roadmap slice `go test ./internal/parser ./internal/checker ./cmd/walk -run 'TestV4|TestV3|TestV22|TestV21|TestV20|TestV15|TestV14|TestFormatter|TestV13|TestV12' -count=1`, full `go test -count=1 ./...`, `go build -o build/walk ./cmd/walk`, VS Code extension syntax plus package/language/grammar JSON validation, `./build/walk docs -o build/v4-api.md examples/v1.walk`, `./build/walk debug-map -o build/v4-debug-map.json examples/v1.walk`, a real `./build/walk lsp` initialize handshake, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v4.0.0 <temp>/release`, `SHA256SUMS` line count check, host release binary `walk version`, and `git diff --check` all passed.

v4.1 verification on 2026-05-22: focused `go test ./cmd/walk -run 'TestV4DocsAndDebugMapCommands|TestV4LSPDiagnosticsFormattingAndCompletion' -count=1`, lexer/parser `go test ./internal/lexer ./internal/parser -count=1`, focused `go test ./cmd/walk -run TestV4 -count=1`, full `go test -count=1 ./...`, `go build -o build/walk ./cmd/walk`, `./build/walk docs --strict -o build/v4.1-api.md examples/v1.walk`, `./build/walk docs --strict --format json -o build/v4.1-api.json examples/v1.walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v4.1.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v5 runtime/backend maturity.
