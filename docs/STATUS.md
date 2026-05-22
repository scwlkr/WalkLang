# WalkLang Status

Current version: v1.3 standard library foundation.

State: v1.3 is complete against `docs/ROADMAP.md`: stable stdlib functions are `math.sqrt`, `math.pow`, `string.len`, `array.len`, `time.now`, `random.int`, and `testing.assert`; file/json/matrix APIs are documented as draft-only; stdlib fixtures run through conformance and the stress script; v1.2 project mode remains present.

Verification on 2026-05-22: `go test ./...`, `go build -o build/walk ./cmd/walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v1.3.0 <temp-dir>`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Next: v1.4 diagnostics and developer experience.
