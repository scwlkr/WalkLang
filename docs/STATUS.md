# WalkLang Status

Current version: v3 package ecosystem.

State: v3 is complete against `docs/ROADMAP.md`: package projects can be created with `walk package init`, package manifests pin exact semantic dependency versions in `[dependencies]`, `walk package resolve` copies packages from a local registry into `.walk/packages/` and writes `walk.lock`, project check/test/build commands verify package cache checksums before using dependencies, dotted package imports such as `imp: geometry.core` resolve to cached package modules, and `walk package publish` requires documented library code plus passing check/test gates before writing `<registry>/<name>/<version>/`. The local-registry package surface is documented in `docs/V3.md` and `docs/PROJECTS.md`. Structs, methods, and generic functions remain documented as experimental v2 surface in `docs/V2.md`. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: focused `go test ./internal/parser ./internal/checker ./cmd/walk -run 'TestV3|TestV22|TestV21|TestV20|TestV15|TestV14|TestFormatter|TestV13|TestV12'`, `go test -count=1 ./...`, `go build -o build/walk ./cmd/walk`, a real local package registry smoke using `./build/walk package init`, `./build/walk package publish`, `./build/walk package resolve`, `./build/walk check --warnings=error`, `./build/walk test --warnings=error`, and `./build/walk build`, package executable output comparison, `go test ./cmd/walk -run TestV15CompatibilitySuite`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v3.0.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v4 professional tooling, after the local package ecosystem has stayed reproducible and package diagnostics remain clear.
