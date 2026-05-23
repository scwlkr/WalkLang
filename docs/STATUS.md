# WalkLang Status

Stable language contract: v1.9.

Current compiler/tooling/docs release: v5.8.0 draft local file text IO.

Experimental implemented language surface: v2.2.

State: v5.8.0 starts `IO_PLAN.md` Roadmap Phase 2, Local Filesystem IO, with a
draft UTF-8 text-file slice. Draft `file.read`, `file.write`, and
`file.exists` are now importable compiler APIs. `file.read` returns a
runtime-owned string, `file.write` is an explicit `do:` effect that overwrites
the target path, and `file.exists` returns host path existence as `bool`.
Relative and absolute paths are passed to the host OS without normalization or
`~` expansion; relative paths resolve against the native process current
working directory. This first file slice is fail-stop: missing files,
permission errors, empty read/write paths, invalid UTF-8 reads, embedded null
bytes on read, and write failures stop the native program with a clear runtime
error. Stable language contract remains v1.9. `file.append`, directory/path
helpers, `process.chdir`, recoverable file result structs, JSON, process
spawning, terminal raw mode, HTTP, and browser targets remain gated by
`IO_PLAN.md`.

v5.8.0 verification on 2026-05-23: focused file IO tests first failed because
`imp: file` was not implemented, then
`go test ./cmd/walk -run
'TestDraftFileTextIOReadsWritesAndChecksExistence|TestDraftFileReadMissingFileFailsClearly|TestDraftFileReadInvalidUTF8FailsClearly|TestDoEffectConsoleAndProcessFoundation|TestRuntimeOwnedTextInputAndParseResults|TestReadLineReportsImmediateEOFAsData|TestV13GeneratedCSnapshots|TestV13FailFixturesHaveExpectedDiagnostics|TestV19ReleaseDocsArePresent'
-count=1` passed after implementation and generated C snapshot refresh; full
`go test -count=1 ./...` passed; `go build -trimpath -ldflags "-X
main.version=v5.8.0" -o build/walk ./cmd/walk` passed; `./build/walk version`
reported `v5.8.0`; `./build/walk check --warnings=error
tests/pass/do_effects.walk` passed; `WALK_BIN=$PWD/build/walk
scripts/stress-v1.sh` passed and reported `v1.9 stress ok`;
`scripts/build-docs-site.sh` passed; `scripts/check-docs-site.sh` passed after
staging generated docs; `scripts/release.sh v5.8.0 <temp>/release` produced 5
platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS`
reported 5 checksum lines; and the host release binary reported `v5.8.0` from
`walk version`.

State: v5.7.0 completes the `IO_PLAN.md` Phase 2 runtime-owned text
input/parse slice as draft compiler APIs. The stable language contract remains
v1.9. Draft `IOReadResult`, `ParseIntResult`, `ParseFloatResult`, and
`ParseBoolResult` structs now expose the concrete recoverable result shape
`ok`, `value`, and `error`. Draft `io.read_line()` and `io.read_all()` read
runtime-owned stdin text, with immediate `io.read_line()` EOF returned as
`error 'eof'`; draft `parse.int`, `parse.float`, and `parse.bool` return
invalid conversion as data instead of nullable-only values or runtime stops.
`time.sleep` remains a separate candidate time helper. File IO, directory/path
helpers, process spawning, terminal raw mode, JSON, HTTP, and web/graphics
targets remain gated by `IO_PLAN.md`.

v5.7.0 verification on 2026-05-23: focused IO/parse tests failed before
implementation because `parse` was not a built-in module and `io.read_line`
was unknown; after implementation, `go test ./cmd/walk -run
'TestRuntimeOwnedTextInputAndParseResults|TestReadLineReportsImmediateEOFAsData|TestV13GeneratedCSnapshots|TestV13FailFixturesHaveExpectedDiagnostics|TestV19ReleaseDocsArePresent'
-count=1` passed; full `go test -count=1 ./...` passed; `go build -trimpath
-ldflags "-X main.version=v5.7.0" -o build/walk ./cmd/walk` passed;
`./build/walk version` reported `v5.7.0`; `./build/walk check
--warnings=error tests/pass/do_effects.walk` passed; `WALK_BIN=$PWD/build/walk
scripts/stress-v1.sh` passed and reported `v1.9 stress ok`;
`scripts/build-docs-site.sh` passed; `scripts/check-docs-site.sh` passed after
staging generated docs; `scripts/release.sh v5.7.0 <temp>/release` produced 5
platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS`
reported 5 checksum lines; and the host release binary reported `v5.7.0` from
`walk version`.

IO planning update on 2026-05-23: `IO_PLAN.md` now uses a five-phase
implementation roadmap for future planning and handoffs: CLI text IO, local
filesystem IO, process and data interop, terminal UX, and network/rich
runtimes. The older Phase 0-10 notes remain as detailed dependency notes, not
the primary roadmap. Roadmap Phase 1 is done as draft compiler APIs in
v5.7.0; the next implementation milestone is Roadmap Phase 2, Local Filesystem
IO. Its first gates are path policy, UTF-8 file policy, fail-stop versus
recoverable file API shape, temp-directory tests, and cwd mutation policy for
`process.chdir`.

State: v5.6.0 completes `IO_PLAN.md` Phase 0 and the implementable Phase 1
console/process foundation as draft compiler APIs. The stable language contract
remains v1.9. Draft `do:` effect statements now accept registered effect calls;
draft `io.write`, `io.write_line`, and `io.error_line` cover stdout/stderr line
helpers; draft `process.args`, `process.arg_count`, `process.env`,
`process.cwd`, and `process.exit` cover the first process helpers; and new IO
functions are recorded in a built-in API registry with draft/effect/runtime
metadata. Later IO phases remain gated on their documented prerequisites,
especially recoverable error results, text/path policy, richer data modeling,
and networking security policy.

v5.6.0 verification on 2026-05-22: the focused IO/process tests failed before
implementation because `do:` was unsupported and `io` was not a built-in
module; after implementation, `go test ./cmd/walk ./internal/format -run
'TestDoEffectConsoleAndProcessFoundation|TestProcessExitEffectExitsWithCode|TestV13PassFixturesBuildAndRun|TestV13FailFixturesHaveExpectedDiagnostics|TestV19ReleaseDocsArePresent|TestFormatterNormalizesDoEffectStatement'
-count=1` passed; full `go test -count=1 ./...` passed after refreshing the
generated C snapshots for the new runtime helper block; `go build -trimpath
-ldflags "-X main.version=v5.6.0" -o build/walk ./cmd/walk` passed;
`./build/walk version` reported `v5.6.0`; `./build/walk check
--warnings=error tests/pass/do_effects.walk` passed; `./build/walk
tests/pass/do_effects.walk` printed `loading` and `done`; `WALK_BIN=$PWD/build/walk
scripts/stress-v1.sh` passed and reported `v1.9 stress ok`;
`scripts/build-docs-site.sh` passed; `scripts/check-docs-site.sh` passed after
staging generated docs; `scripts/release.sh v5.6.0 <temp>/release` produced 5
platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS`
reported 5 checksum lines; and the host release binary reported `v5.6.0` from
`walk version`.

State: v5.5.0 adds the v1.9 stable string interpolation surface. Single-quoted
strings can include `{expression}` display values, interpolation supports
`int`, `float`, `bool`, `string`, and nullable string values, and doubled braces
such as `{{word}}` print literal braces.

v5.5.0 verification on 2026-05-22: focused interpolation conformance failed
before implementation because strings printed literally and bad interpolation
values were accepted; after the fix, `go test ./cmd/walk -run
'TestV13PassFixturesBuildAndRun|TestV13FailFixturesHaveExpectedDiagnostics'
-count=1` passed; `go test ./cmd/walk -run
'TestV13PassFixturesBuildAndRun|TestV19CompatibilitySuite' -count=1` passed;
full `go test -count=1 ./...` passed; `go build -trimpath -ldflags "-X
main.version=v5.5.0" -o build/walk ./cmd/walk` passed; `./build/walk version`
reported `v5.5.0`; `./build/walk check --warnings=error
tests/pass/interpolation.walk` passed; `./build/walk
tests/pass/interpolation.walk` printed the expected interpolated lines;
`WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed and reported `v1.9
stress ok`; `scripts/release.sh v5.5.0 <temp>/release` produced 5 platform
artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS` reported 5
checksum lines; the host release binary reported `v5.5.0` from `walk version`;
`scripts/build-docs-site.sh` passed; `scripts/check-docs-site.sh` passed; and
`git diff --check --cached` passed for the staged release diff. Full worktree
`git diff --check` is still blocked by the pre-existing unstaged
`playground/hangman-v2.walk` trailing whitespace.

Docs brand polish update on 2026-05-22: the generated docs site is now dark-mode
only, uses a blue WalkLang accent family instead of the previous green tokens,
publishes the compact WalkLang icon as `/favicon.svg`, uses that compact mark in
the docs sidebar, and declares the same mark as the VS Code `walk` language icon
for compatible file icon themes.

Docs brand polish verification on 2026-05-22: focused site-generator tests cover
the root favicon link, compact sidebar icon, dark blue accent tokens, removed
green tokens, and existing VS Code language icon asset path; `go test ./scripts
-count=1` passed; `scripts/build-docs-site.sh` passed; and
`scripts/check-docs-site.sh` passed after regenerating `public/`.

Live docs TLS recovery on 2026-05-22: root cause was GitHub Pages serving the
custom domain over HTTP while no Pages TLS certificate existed for
`walklang.wlkrlabs.com`; DNS was already valid as the DNS-only
`walklang -> scwlkr.github.io` CNAME and the Pages health check reported the
domain as valid and HTTPS-eligible. Removed and re-added the Pages custom
domain to restart certificate provisioning, confirmed the new Let's Encrypt
certificate includes `walklang.wlkrlabs.com`, enabled HTTPS enforcement, and
triggered a fresh Pages deployment to clear the cached HTTP homepage. Live
verification passed with `http://walklang.wlkrlabs.com` returning `301` to
`https://walklang.wlkrlabs.com/`, `https://walklang.wlkrlabs.com` returning
`200`, GitHub Pages API reporting `https_enforced: true`, and Pages health
reporting `responds_to_https: true` plus `enforces_https: true`.

State: v5.4.1 fixes `random.choice` and `random.int` to use a runtime-owned
PRNG seeded once per native process instead of the C runtime's unseeded default
`rand()` state.

v5.4.1 verification on 2026-05-22: added `TestRandomChoiceSeedsFreshProcesses`
and confirmed it failed before the runtime fix because fresh native processes
all produced the same first `random.choice` value; after the fix, full `go test
-count=1 ./...` passed; `go build -trimpath -ldflags "-X main.version=v5.4.1"
-o build/walk ./cmd/walk` passed and `./build/walk version` reported
`v5.4.1`; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed and reported
`v1.8 stress ok`; `scripts/release.sh v5.4.1 <temp>/release` produced 5
platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS`
reported 5 checksum lines; the host release binary reported `v5.4.1` from
`walk version`; `scripts/install-local.sh v5.4.1` installed
`/Users/shanewalker/.local/bin/walk`; and twelve fresh installed
`walk hangman-v2.walk` runs from `playground/` produced multiple choices.

State: v5.4 is complete against the new v1.8 terminal-game helper surface.
Stable v1.8 adds `string.at`, string indexing with `word[0]`,
`string.contains`, `string.concat`, `array.contains`, `array.push`,
explicitly typed empty arrays such as `var: guessed array[string] = []`, and
`random.choice` for non-empty stable native arrays. The helpers keep the
existing expression/module model: `array.push` returns a new array and string
building uses `string.concat` instead of changing numeric `+`.

Hangman readiness update on 2026-05-22: `playground/hangman.walk` is now a
compiling terminal Hangman example using `in:`, `out:`, `if:`, `while:`,
functions, string indexing, contains checks, functional array push, and random
choice.

v5.4 verification on 2026-05-22: full `go test -count=1 ./...` passed;
`go build -o build/walk ./cmd/walk` passed; `WALK_BIN=$PWD/build/walk
scripts/stress-v1.sh` passed and reported `v1.8 stress ok`; `./build/walk
check --warnings=error playground/hangman.walk` passed; `./build/walk build
playground/hangman.walk -o build/hangman` plus a piped wrong-guess smoke run
passed; `scripts/build-docs-site.sh` passed; `scripts/release.sh v5.4.0
<temp>/release` produced 5 platform artifacts plus `SHA256SUMS`; `wc -l
<temp>/release/SHA256SUMS` reported 5 checksum lines; the host release binary
reported `v5.4.0` from `walk version`; and `git diff --check` passed.

State: v5.3 is complete against the new v1.7 stable `in:` input rule. `in:` is
a core expression that returns `string`, can be used anywhere an expression is
valid, reads exactly one required line from stdin, supports an optional string
prompt expression written to stdout without a newline, flushes stdout before
reading, strips only the final line ending, preserves all other characters,
returns `''` for an empty line, accepts a final unterminated line, and
runtime-stops on immediate EOF, stdin read failure, or allocation failure.

v5.3 verification on 2026-05-22: focused `go test ./cmd/walk ./internal/format -run 'TestInExpression|TestInPrompt|TestV17Formatter' -count=1` passed; full `go test -count=1 ./...` passed; `go build -o build/walk ./cmd/walk` passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed and reported `v1.7 stress ok`; `scripts/check-docs-site.sh` passed after generated site artifacts were staged; `scripts/release.sh v5.3.0 <temp>/release` produced 5 platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS` reported 5 checksum lines; the host release binary reported `v5.3.0` from `walk version`; and `git diff --check` passed.

IO planning update on 2026-05-22: `IO_PLAN.md` now records stable `out:` and
`in:` as the current baseline, the proposed module-first IO direction for most
future IO, the recommended `do:` effect-call decision point, runtime-owned
string and recoverable-error prerequisites, feature-by-feature implementation
costs, and a phased order for files, directories, JSON, networking, web, and
graphics.

IO planning verification on 2026-05-22: `scripts/build-docs-site.sh` passed;
full `go test -count=1 ./...` passed; `go build -o build/walk ./cmd/walk`
passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed and reported
`v1.6 stress ok`; `scripts/release.sh v5.2.0 <temp>/release` produced 5
platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS`
reported 5 checksum lines; and the host release binary reported `v5.2.0` from
`walk version`.

State: v5.2 is complete against the new v1.6 local function type inference rule. Ordinary functions may omit parameter and return types when the local body proves the types; ambiguous omitted parameter types now fail with a direct annotation diagnostic; normal typed signatures remain valid; inference does not use later call sites; and C remains the primary backend.

v5.2 verification on 2026-05-22: focused `go test ./cmd/walk -run 'TestFunctionTypeInference|TestV15ReleasePrepDocsArePresent' -count=1` passed; full `go test -count=1 ./...` passed; `go build -o build/walk ./cmd/walk` passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed and reported `v1.6 stress ok`; `scripts/check-docs-site.sh` passed after generated site artifacts were staged; `scripts/release.sh v5.2.0 <temp>/release` produced 5 platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS` reported 5 checksum lines; the host release binary reported `v5.2.0` from `walk version`; and `git diff --check` passed.

Live docs verification on 2026-05-22: `walklang.wlkrlabs.com` resolves through the DNS-only `walklang -> scwlkr.github.io` CNAME to GitHub Pages addresses; `http://walklang.wlkrlabs.com/docs/` returns `200 OK` from GitHub Pages and includes the v5.1 docs content; `http://walklang.wlkrlabs.com/docs/reference/api.json` returns repo-relative source paths. HTTPS is not enforced yet because GitHub Pages still reports `The certificate does not exist yet` when enabling HTTPS and `https://walklang.wlkrlabs.com/docs/` still fails certificate hostname validation.

Public positioning update on 2026-05-22: README and docs front-door wording now distinguish the stable `v1.5` language contract, the current `v5.1.0` compiler/tooling/docs release, and the experimental v2.2 language surface. README now includes a tiny WalkLang function example, a "What works today" section, and less inflated repository-scope wording. HTTPS remediation checked the live DNS CNAME, CAA records, Pages API state, and Pages HTTP response; DNS points at `scwlkr.github.io`, CAA allows Let's Encrypt, HTTP still returns `200 OK`, but GitHub still rejects HTTPS enforcement with `The certificate does not exist yet`. After the Pages deployment for commit `335c877` completed successfully, a second HTTPS enforcement attempt failed with the same certificate message and `https://walklang.wlkrlabs.com/docs/` still failed hostname validation. No DNS or repo-side blocker is visible; the remaining action is to retry once GitHub has issued the Pages certificate.

Public positioning verification on 2026-05-22: `scripts/build-docs-site.sh` passed; `scripts/check-docs-site.sh` passed after generated site artifacts were staged; `git diff --check` passed; full `go test -count=1 ./...` passed; `go build -o build/walk ./cmd/walk` passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed; `scripts/release.sh v5.1.0 <temp>/release` produced 5 platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS` reported 5 checksum lines; and the host release binary reported `v5.1.0` from `walk version`.

Docs setup on 2026-05-22: the root README now follows a Rust-style repository landing-page shape, `docs/README.md` is the canonical docs front door and source for the `https://walklang.wlkrlabs.com/docs` hosted path, `CONTRIBUTING.md` documents the local contribution and verification path, and `docs/DOCS_STYLE_GUIDE.md` records the source-grounded docs rules plus generated-reference expectations.

Docs setup verification on 2026-05-22: `curl -I --max-time 10 https://walklang.wlkrlabs.com/docs` failed with `Could not resolve host`, so the hosted docs URL remains a planned publish target; changed-doc local Markdown links passed; `git diff --check` passed; `go test -count=1 ./...` passed; `go build -o build/walk ./cmd/walk` passed; `./build/walk docs --strict -o build/docs-check-api.md examples/v1.walk` passed; `./build/walk docs --strict --format json -o build/docs-check-api.json examples/v1.walk` passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed; and `scripts/release.sh v5.0.0 <temp>/release` produced 5 platform artifacts plus 5 checksum lines.

v5 verification on 2026-05-22: baseline `go test -count=1 ./...` passed before implementation; focused `go test ./cmd/walk -run 'TestV5|TestV13GeneratedCSnapshots' -count=1` passed; full `go test -count=1 ./...` passed after the final code/test changes; `go build -o build/walk ./cmd/walk` passed; `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh` passed; `scripts/release.sh v5.0.0 <temp>/release` produced 5 platform artifacts plus `SHA256SUMS`; `wc -l <temp>/release/SHA256SUMS` reported 5 checksum lines; the host release binary reported `v5.0.0` from `walk version`; and `git diff --check` passed.

Previous v4.1 verification on 2026-05-22: focused `go test ./cmd/walk -run 'TestV4DocsAndDebugMapCommands|TestV4LSPDiagnosticsFormattingAndCompletion' -count=1`, lexer/parser `go test ./internal/lexer ./internal/parser -count=1`, focused `go test ./cmd/walk -run TestV4 -count=1`, full `go test -count=1 ./...`, `go build -o build/walk ./cmd/walk`, `./build/walk docs --strict -o build/v4.1-api.md examples/v1.walk`, `./build/walk docs --strict --format json -o build/v4.1-api.json examples/v1.walk`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v4.1.0 <temp>/release`, `SHA256SUMS` line count check, and `git diff --check` all passed.

Release CI maintenance on 2026-05-22: GitHub Actions workflow uses Node 24 first-party actions, disables unnecessary Go dependency caching for this no-`go.sum` module, runs `scripts/check-docs-site.sh`, and now publishes v5.4.1-labeled release artifacts.

Playground example update on 2026-05-22: `playground/route_ranker.walk` now demonstrates a compact route-ranking program with structs, arrays, loops, typed helper functions, prefix math, and scalar output. Verification passed with `./build/walk check --warnings=error playground/route_ranker.walk`, `./build/walk build playground/route_ranker.walk -o build/route_ranker`, and `./build/route_ranker`, which printed `Library Lane` with score `46`; broader verification passed with `go test -count=1 ./...`, `scripts/check-docs-site.sh`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v5.1.0 <temp>/release`, a 5-line `SHA256SUMS` check, host release binary `walk version` reporting `v5.1.0`, and `git diff --check`.

CLI run shortcut update on 2026-05-22: `walk run <source.walk>` now compiles a single file to a temporary native executable, runs it, streams program input and output, and removes the temporary build directory; `walk <source.walk>` is a direct shorthand for the same flow. Verification passed with focused `go test ./cmd/walk -run TestRunCommandRunsSingleFileAndDirectFileAlias -count=1`, `go build -o build/walk ./cmd/walk`, `./build/walk run playground/route_ranker.walk`, `./build/walk playground/route_ranker.walk`, full `go test -count=1 ./...`, `scripts/check-docs-site.sh`, `WALK_BIN=$PWD/build/walk scripts/stress-v1.sh`, `scripts/release.sh v5.1.0 <temp>/release`, a 5-line `SHA256SUMS` check, host release binary `walk version` reporting `v5.1.0`, and `git diff --check`.

Next: continue the remaining `IO_PLAN.md` Roadmap Phase 2 work with
`file.append`, directory/path helpers, `process.chdir`, and recoverable file
result structs before moving to Phase 3 process/data interop.
