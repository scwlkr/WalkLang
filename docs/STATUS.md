# WalkLang Status

Current version: v2.0 data modeling.

State: v2 is complete against `docs/ROADMAP.md`: `struct:` declarations compile, struct values can be created with positional constructors, fields can be read and assigned through mutable roots, structs work as function parameters and return values, arrays can hold structs and update indexed fields, modules can declare structs and return them through exported functions, and field diagnostics cover missing constructor values, unknown fields, wrong field types, and const-root assignment. Structs are documented as experimental v2 surface in `docs/V2.md`; v2.1 methods and v2.2 composition remain future roadmap phases. The v1.5 compatibility contract remains documented in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

Verification on 2026-05-22: `go test ./cmd/walk -run TestV15CompatibilitySuite`, `go test ./...`, `go build -o build/walk ./cmd/walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `./build/walk check --warnings=error tests/pass/structs.walk`, `./build/walk build tests/pass/structs.walk -o <temp>/structs` plus output comparison, `./build/walk check --warnings=error tests/pass/struct_modules.walk`, `./build/walk build tests/pass/struct_modules.walk -o <temp>/struct_modules` plus output comparison, `scripts/release.sh v2.0.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions and disables unnecessary Go dependency caching for this no-`go.sum` module.

Next: v2.1 methods, after the v2 struct surface stays explainable and diagnostics remain clear.
