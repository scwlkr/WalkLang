# WalkLang Status

Current version: v2.2 stronger composition.

State: v2.2 is complete against `docs/ROADMAP.md`: simple generic functions can be declared with type parameters, call sites infer concrete type arguments, generic functions compose with arrays, structs, methods, and exported user modules, type errors explain mismatched inferred arguments, and generated C remains predictable through monomorphized specialized functions. Structs, methods, and generic functions are documented as experimental v2 surface in `docs/V2.md`. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: `go test ./internal/format ./internal/parser ./internal/checker ./internal/emitter ./cmd/walk -run 'TestV22|TestFormatter|TestV21|TestV20'`, `go test ./...`, `go build -o build/walk ./cmd/walk`, `./build/walk check --warnings=error tests/pass/generics.walk`, `./build/walk check --warnings=error tests/pass/generics_modules.walk`, `./build/walk build tests/pass/generics.walk -o <temp>/generics` plus output comparison, `./build/walk build tests/pass/generics_modules.walk -o <temp>/generics_modules` plus output comparison, `go test ./cmd/walk -run TestV15CompatibilitySuite`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v2.2.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v3 package ecosystem, after the experimental v2.2 generic function surface stays explainable and diagnostics remain clear.
