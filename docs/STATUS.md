# WalkLang Status

Current version: v1.1 language contract.

State: v1.1 is complete against `docs/ROADMAP.md`: stable spec, syntax, stdlib, diagnostics, design rules, compatibility docs, pass/fail conformance fixtures, and generated C snapshots are present.

Verification on 2026-05-22: `go test ./...`, temp `WALK_BIN` + `scripts/stress-v1.sh`, `scripts/release.sh v1.1.0 <temp-dir>`, and `git diff --check` all passed.

Next: v1.2 project mode (`walk init`, project config, multi-file project builds, CI/release polish).
