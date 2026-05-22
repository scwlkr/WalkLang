# WalkLang Status

Current version: v1.2 project mode.

State: v1.2 is complete against `docs/ROADMAP.md`: `walk init`, `walk.toml`, project-mode build/check/test/fmt/clean, multi-file project module search, example fixtures, release artifact checksums, and GitHub Actions CI are present. The v1.1 language contract remains the stable language surface.

Verification on 2026-05-22: `go test ./...`, temp `WALK_BIN` + `scripts/stress-v1.sh`, `scripts/release.sh v1.2.0 <temp-dir>`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Next: v1.3 standard library foundation (`time.now`, `random.int`, file/json/matrix drafts, and stdlib consistency work).
