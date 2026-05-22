# WalkLang Status

Current version: v1.4 diagnostics and developer experience.

State: v1.4 is complete against `docs/ROADMAP.md`: compiler diagnostics keep the stable `file:line:column: category: message` first line and the CLI now adds source snippets, caret locations, and focused suggestions when obvious; exact tests cover important diagnostic and warning output; `walk check` remains the fast validation path before build; warning modes remain `--warnings=off|default|error`; stable warnings cover shadowed names and unreachable statements. v1.3 stable stdlib APIs remain `math.sqrt`, `math.pow`, `string.len`, `array.len`, `time.now`, `random.int`, and `testing.assert`; file/json/matrix APIs remain draft-only.

Verification on 2026-05-22: `go test ./...`, `go build -o build/walk ./cmd/walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v1.4.0 <temp-dir>`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Next: v1.5 compatibility release preparation.
