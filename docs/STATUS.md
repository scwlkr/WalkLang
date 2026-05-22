# WalkLang Status

Current version: v2.1 methods.

State: v2.1 is complete against `docs/ROADMAP.md`: methods can be declared on structs with receiver syntax, method names are namespaced by receiver type, calls on struct values compile to predictable receiver functions, method calls type-check receiver and ordinary arguments, and diagnostics cover unknown methods, non-struct receivers, bad receiver parameters, and wrong method argument types. Structs and methods are documented as experimental v2 surface in `docs/V2.md`; v2.2 stronger composition remains a future roadmap phase. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: `go test ./internal/format ./cmd/walk -run 'TestV21|TestV20|TestV13Fail|TestV13Pass'`, `go test ./internal/parser ./internal/checker ./internal/emitter`, `go test ./...`, `go build -o build/walk ./cmd/walk`, `./build/walk check --warnings=error tests/pass/methods.walk`, `./build/walk build tests/pass/methods.walk -o <temp>/methods` plus output comparison, `go test ./cmd/walk -run TestV15CompatibilitySuite`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v2.1.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v2.2 stronger composition, after the v2.1 method surface stays explainable and diagnostics remain clear.
