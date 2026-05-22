# WalkLang Status

Current version: v1.5 compatibility release preparation.

State: v1.5 is complete against `docs/ROADMAP.md`: the v1 compatibility promise is explicit, stable and experimental surfaces are separated, release notes, migration guidance, deprecation policy, and official install instructions exist, and the focused v1 compatibility suite covers representative stable programs, test syntax, user-module exports, stable stdlib APIs, and selected stable diagnostic first lines. v1.4 diagnostics remain stable: the first line keeps `file:line:column: category: message`, and CLI output can include snippets, caret locations, and focused suggestions. Stable stdlib APIs remain `math.sqrt`, `math.pow`, `string.len`, `array.len`, `time.now`, `random.int`, and `testing.assert`; file/json/matrix APIs remain draft-only.

Verification on 2026-05-22: `go test ./cmd/walk -run TestV15CompatibilitySuite`, `go test ./...`, `go build -o build/walk ./cmd/walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v1.5.0 <temp-dir>`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Next: v2 data modeling after the v1 language contract remains stable.
