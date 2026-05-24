# WalkLang Release Notes

## Unreleased

No unreleased changes.

## v5.12.3 - Sidebar Navigation Cleanup

Date: 2026-05-24

v5.12.3 cleans up the generated docs sidebar so the public navigation is
organized by reader task and document role instead of exposed release
milestones. The stable language contract remains v1.9 and the draft runtime API
surface is unchanged from v5.12.0.

### Improved

- Sidebar links now use topic labels such as `Syntax`, `Specification`,
  `Standard Library`, `API Reference`, `Architecture`, and `Purpose` instead
  of version-heavy page titles.
- The primary sidebar groups now follow the documentation roles: Start,
  Language, Reference, Tools, Project, and Releases.
- Historical `V1`, `V2`, and `V3` milestone pages remain generated and
  linkable, but they no longer appear as top-level sidebar entries.
- Release notes now appear in the sidebar as `Version History`.

### Notes

- Page titles, generated pages, search metadata, and release history remain
  intact. This release changes navigation presentation, not language behavior.
- The stable language contract remains v1.9.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.12.3 dist
```

## v5.12.2 - Search Result Previews

Date: 2026-05-24

v5.12.2 improves the generated docs search result cards while keeping the
stable language contract at v1.9 and the draft runtime API surface unchanged
from v5.12.0.

### Improved

- Generated search index entries are now section-level records with parent doc
  context, group labels, section anchors, readable summaries, and full cleaned
  search text.
- Search result cards now show a section title, parent doc/group context, and a
  prose preview instead of a raw sliced Markdown snippet.
- Ranking now boosts section headings, section summaries, and API-shaped
  matches such as `array.push`, so targeted searches land on the most useful
  docs section first.
- Markdown link targets are stripped from search previews so list summaries keep
  human labels without leaking file names.

### Notes

- Search remains static and local to the generated docs site.
- The stable language contract remains v1.9.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.12.2 dist
```

## v5.12.1 - Docs Search

Date: 2026-05-24

v5.12.1 adds a small generated docs search experience while keeping the stable
language contract at v1.9 and the draft runtime API surface unchanged from
v5.12.0.

### Added

- Sidebar docs search box on every generated static docs page.
- Generated `docs/search.json` index containing page titles, URLs, and compact
  source text from the docs Markdown files.
- Small client-side search script that ranks title and body matches and links
  directly to matching docs pages.
- Site-generator tests covering the search script link and searchable index
  contents.

### Notes

- Search is intentionally static and local to the generated site. It does not
  use a hosted search service or require a backend.
- The stable language contract remains v1.9.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.12.1 dist
```

## v5.12.0 - Draft Network And Rich Runtime IO

Date: 2026-05-23

v5.12.0 completes `IO_PLAN.md` Roadmap Phase 5 as draft compiler APIs and
design docs while keeping the stable language contract at v1.9.

### Added

- Draft `http.get`, `http.post`, and `http.request` helpers.
- Draft `HttpResult` for recoverable HTTP status, body, and error data.
- Draft `html.escape`, `html.h1`, `html.p`, and `html.button` text helpers.
- Networking security, timeout, response-size, TLS/backend, and no-public-network
  test policy in `docs/NETWORKING.md`.
- Rich runtime design boundaries for HTML helpers, web servers, native graphics,
  WASM/browser backend, and compiler explorer/playground integration in
  `docs/RICH_RUNTIMES.md`.
- Native local-loopback HTTP tests and HTML escaping tests for the draft Phase 5
  surface.

### Notes

- The stable language contract remains v1.9.
- Draft HTTP delegates to the system `curl` executable at runtime instead of
  linking a C TLS library into generated output.
- Draft HTTP does not invoke a shell, follows redirects, uses a 10 second
  timeout, caps response bodies at 1 MiB, and treats response bodies as UTF-8
  text.
- HTTP status codes `200` through `399` set `ok true`; other status codes
  preserve the body and return `ok false` with `error 'http status'`.
- `html` helpers generate escaped strings only; they do not start a web server,
  run a browser, attach assets, or own a DOM.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.12.0 dist
```

## v5.11.0 - Draft Terminal UX IO

Date: 2026-05-23

v5.11.0 completes `IO_PLAN.md` Roadmap Phase 4 as draft compiler APIs while
keeping the stable language contract at v1.9.

### Added

- Draft `term.is_tty`, `term.color`, `term.background`, `term.style`,
  `term.reset`, `term.clear`, `term.move`, `term.width`, `term.height`, and
  `term.read_key` helpers.
- ANSI styling policy for TTY output, `NO_COLOR`, and explicit
  `CLICOLOR_FORCE` testing.
- Deterministic terminal width and height fallbacks through `COLUMNS`, `LINES`,
  `80`, and `24`.
- Recoverable non-interactive `term.read_key` behavior through `IOReadResult`.
- Native runtime tests for clean redirected output, forced ANSI output,
  dimensions fallback, non-interactive key reads, invalid terminal names, and
  invalid-use diagnostics.

### Notes

- The stable language contract remains v1.9.
- Terminal styling is opt-in and resettable through explicit `do:` effect calls.
- Redirected stdout remains clean by default because styling, movement, clear,
  and reset calls no-op when stdout is not a TTY.
- `term.read_key` uses raw terminal mode only for a single key read and restores
  the terminal before returning.
- HTTP, browser targets, graphics, and rich runtimes remain gated by later
  `IO_PLAN.md` roadmap phases.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.11.0 dist
```

## v5.10.0 - Draft Process And Data Interop IO

Date: 2026-05-23

v5.10.0 completes `IO_PLAN.md` Roadmap Phase 3 as draft compiler APIs while
keeping the stable language contract at v1.9.

### Added

- Draft `process.run`, `process.output`, and `process.run_shell` helpers.
- Draft `ProcessResult` and `ProcessOutputResult` structs for command status, stdout, stderr, and error data.
- Draft `json.parse`, `json.stringify`, `json.read`, and `json.write` helpers.
- Draft `JsonResult` for recoverable JSON parse/read failures.
- Native helper-command tests for argv-style process execution, stdout/stderr capture, non-zero status as data, and explicit shell execution.
- Native JSON tests for string escaping, compact validation, invalid JSON as data, and file-backed JSON read/write behavior.

### Notes

- The stable language contract remains v1.9.
- `process.run` is argv-style and does not invoke a shell.
- `process.run_shell` is intentionally explicit and shell-dependent; prefer `process.run` when arguments are known.
- Draft JSON APIs use compact validated JSON text as the interchange value until WalkLang has maps, dynamic values, or recursive generic JSON structs.
- Terminal raw mode, HTTP, and browser targets remain gated by later `IO_PLAN.md` roadmap phases.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.10.0 dist
```

## v5.9.0 - Draft Local Filesystem IO Completion

Date: 2026-05-23

v5.9.0 completes `IO_PLAN.md` Roadmap Phase 2 as draft compiler APIs while
keeping the stable language contract at v1.9.

### Added

- Draft `file.append`, `file.try_read`, `file.try_write`, and `file.try_append` helpers.
- Draft `FileReadResult` and `FileActionResult` structs for recoverable file operations.
- Draft `dir.list`, `dir.make`, and `dir.delete` helpers.
- Draft `path.join`, `path.base`, and `path.ext` helpers.
- Draft `process.chdir` effect helper.
- Native temp-directory tests covering append, directory creation/list/delete, path building, cwd mutation isolation, recoverable file results, and invalid-use diagnostics.

### Notes

- The stable language contract remains v1.9.
- Fail-stop file, directory, and cwd helpers still use clear `walk runtime error` messages for unrecoverable draft behavior.
- Recoverable `file.try_*` helpers report ordinary file/path/write/read failures as result structs; allocation failure still runtime-stops.
- JSON, process spawning, terminal raw mode, HTTP, and browser targets remain gated by later `IO_PLAN.md` roadmap phases.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.9.0 dist
```

## v5.8.0 - Draft Local File Text IO

Date: 2026-05-23

v5.8.0 starts the local filesystem roadmap phase with draft UTF-8 text file
helpers.

### Added

- Draft `file.read`, `file.write`, and `file.exists` helpers.
- Native temp-directory tests for read/write/existence behavior.
- Native negative tests for missing files and invalid UTF-8 reads.
- Phase 2 IO plan decisions for path handling, UTF-8 text policy, fail-stop file errors, temp-directory tests, and deferred cwd mutation.

### Notes

- The stable language contract remains v1.9.
- Draft file paths are passed to the host OS without normalization or `~` expansion. Relative paths resolve against the native process current working directory.
- Draft `file.read` and `file.write` are fail-stop APIs for now. Recoverable file result structs remain future Phase 2 work.
- `file.append`, directory/path helpers, `process.chdir`, JSON, process spawning, terminal raw mode, HTTP, and browser targets remain gated by `IO_PLAN.md`.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.8.0 dist
```

## v5.7.0 - Draft Recoverable Text IO

Date: 2026-05-23

v5.7.0 adds the next draft IO slice: recoverable stdin reads and parse helpers
that return explicit result structs instead of nullable-only values.

### Added

- Draft `IOReadResult`, `ParseIntResult`, `ParseFloatResult`, and `ParseBoolResult` structs with `ok`, `value`, and `error` fields.
- Draft `io.read_line()` and `io.read_all()` for runtime-owned stdin text.
- Draft `parse.int`, `parse.float`, and `parse.bool` helpers.
- Native runtime tests for stdin, EOF-as-data, successful parse results, and invalid parse input.
- Checker diagnostics for invalid draft `io.read_line` and `parse.int` calls.

### Notes

- The stable language contract remains v1.9.
- Draft parse helpers parse the whole input string and return invalid input as data.
- File IO, JSON, process spawning, terminal raw mode, HTTP, and browser targets remain gated by `IO_PLAN.md`.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.7.0 dist
```

## v5.6.0 - Draft IO Foundation

Date: 2026-05-22

v5.6.0 adds the first draft IO/process foundation without promoting it into the
stable v1.9 language contract.

### Added

- Draft `do:` effect statements for explicit side-effect module calls.
- Draft `io.write`, `io.write_line`, and `io.error_line`.
- Draft `process.args`, `process.arg_count`, `process.env`, `process.cwd`, and `process.exit`.
- A small built-in API registry for new IO/process functions, including effect and draft metadata.
- Conformance, fail-fixture, formatter, and native runtime tests for the draft IO/process surface.

### Notes

- `io` and `process` are importable draft modules in the current compiler.
- Runtime-created strings from the draft process helpers live for the native process lifetime.
- Broader IO remains gated on recoverable error and data-model decisions.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.6.0 dist
```

## v5.5.0 - String Interpolation

Date: 2026-05-22

v5.5.0 adds the v1.9 stable string interpolation syntax for simple display text.

### Added

- Single-quoted strings may include `{expression}` interpolation.
- Interpolation formats `int`, `float`, `bool`, `string`, and nullable string values.
- Doubled braces such as `{{word}}` output literal braces.
- Compatibility and conformance fixtures now cover interpolation output and unsupported interpolation values.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.5.0 dist
```

## v5.4.1 - Random Seed Fix

Date: 2026-05-22

v5.4.1 fixes the v1.8 random runtime so fresh native process launches do not start from the same default C `rand()` state.

### Fixed

- `random.choice(items)` now uses the same runtime-owned seeded PRNG as `random.int`, preventing the first choice from repeating across fresh command invocations just because the process was restarted.
- Added a regression test that compiles one `random.choice` program and executes the native binary repeatedly as fresh processes.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.4.1 dist
```

## v5.4.0 - Terminal Game Helpers

Date: 2026-05-22

v5.4.0 adds the v1.8 stable helpers needed for simple terminal games such as Hangman while keeping the existing expression, module, and C-backend model.

### Added

- `string.at(text, index)` and string indexing such as `word[0]`, both returning one-character strings.
- `string.contains(text, piece)` for substring checks.
- `string.concat(left, right)` for explicit string building without changing numeric `+`.
- `array.contains(items, item)` for stable native arrays.
- `array.push(items, item)`, which returns a new array with the item appended.
- Empty array literals when an explicit array annotation provides the element type, such as `var: guessed array[string] = []`.
- `random.choice(items)` for non-empty stable native arrays.
- A compiling `playground/hangman.walk` example.

### Changed

- README and docs front-door wording now separate the stable `v1.8` language contract from the current `v5.4.0` compiler/tooling/docs release.
- The v1 stress path now covers the terminal-game helpers.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.4.0 dist
```

## v5.3.0 - Stable Required-Line Input

Date: 2026-05-22

v5.3.0 adds the v1.7 stable `in:` expression for required-line stdin input.

### Added

- `in:` as a core expression that reads one required line from stdin and returns `string`.
- Optional `in:` prompts, such as `var: name = in: 'Name? '`, written to stdout without a newline and flushed before reading.
- Runtime input handling for empty lines, CRLF line endings, final unterminated lines, immediate EOF, stdin read failure, and allocation failure.
- Compatibility coverage for the stable v1 input surface.

### Changed

- README and docs front-door wording now separate the stable `v1.7` language contract from the current `v5.3.0` compiler/tooling/docs release.
- The stable syntax/spec docs now describe `in:` as a compatible v1.x improvement.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.3.0 dist
```

## v5.2.0 - Local Function Type Inference

Date: 2026-05-22

v5.2.0 adds the v1.6 local function type inference rule: obvious helper functions can omit parameter and return types, while ambiguous functions ask for annotations instead of guessing from call sites.

### Added

- Local function parameter and return type inference for function bodies such as `func: power_four(n)`.
- Clear diagnostics for ambiguous omitted parameter types, such as `cannot infer type for parameter value in function identity; add an annotation`.
- `walk run <source.walk>` compiles a single file to a temporary native executable, runs it, streams program input and output, and cleans up the temporary build directory.
- `walk <source.walk>` is a direct shorthand for `walk run <source.walk>`.

### Changed

- README and docs front-door wording now separate the stable `v1.6` language contract from the current `v5.2.0` compiler/tooling/docs release.
- README now includes a small WalkLang code example and a concise "What works today" section for public readers.
- The stable syntax/spec docs now describe local function inference as a compatible v1.x improvement.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.2.0 dist
```

## v5.1.0 - Public Docs And Reference Site

Date: 2026-05-22

v5.1.0 turns the docs and generated reference output into a repo-owned static
site for `walklang.wlkrlabs.com/docs`.

### Added

- `scripts/build-docs-site.sh` for rebuilding generated reference docs and the
  static site.
- `scripts/check-docs-site.sh` for stale generated-artifact and static-link
  checks.
- Generated `docs/reference/api.md` and `docs/reference/api.json` from
  structured comments in real WalkLang source.
- Static site output in `public/`, including the docs front door, rendered docs
  pages, API reference HTML, raw Markdown, JSON, assets, `.nojekyll`, and CNAME.
- GitHub Pages workflow that builds `public/` and deploys it from `main`.
- `docs/V5_1.md` describing the public docs and reference-site contract.

### Changed

- `walk docs` now renders repository-relative source paths when sources are
  under the current working directory, which keeps public reference output from
  exposing local absolute paths.
- CI now runs the docs-site check and labels release artifacts as `v5.1.0`.
- README and docs front doors now point at the hosted docs path as the public
  docs surface.

### Breaking Changes

None.

### Upgrade

Regenerate docs and release artifacts with:

```bash
scripts/build-docs-site.sh
scripts/check-docs-site.sh
scripts/release.sh v5.1.0 dist
```

## v5.0.0 - Runtime and Backend Maturity

Date: 2026-05-22

v5.0.0 keeps C as the primary backend and makes generated output easier to inspect, optimize, and reason about at runtime.

### Added

- `docs/V5.md` runtime/backend contract documentation.
- A small generated C runtime section with WalkLang scalar aliases, array structs, print helpers, string length helper, random helper, and array allocation helper.
- Source-location comments in emitted C statements, such as `/* source: main.walk:6:1 */`.
- Focused v5 conformance coverage for generated-C runtime helpers, source comments, and array returns.

### Changed

- `walk build --release` now uses `-O3 -DNDEBUG` for native C builds.
- Array literals now allocate item storage through the generated runtime helper instead of pointing array structs at stack-local item buffers.
- Generated C snapshots now cover the v5 runtime section and source comments.
- Official install and CI release artifact labels now target `v5.0.0`.

### Runtime And Memory

WalkLang source still has no `malloc`, `free`, pointer syntax, or public garbage collector promise. Array literal item storage is owned by the generated program for the process lifetime, which makes returned arrays predictable without adding source-level ownership syntax.

### Breaking Changes

None.

### Upgrade

Regenerate emitted C and release binaries with the v5 compiler:

```bash
walk build main.walk -o build/main --release
scripts/release.sh v5.0.0 dist
```

## v4.1.0 - Documentation Generator Hardening

Date: 2026-05-22

v4.1.0 keeps v5 focused on runtime/backend maturity while tightening the v4 docs generator into a small structured-doc slice.

### Added

- `///` structured comments above public WalkLang symbols.
- `walk docs --format json` for machine-readable docs index output.
- `walk docs --strict` for failing when generated public symbols are missing required docs fields.
- Structured docs for the v1 example module.
- Roadmap clarification that broad docs overhaul work waits until v5 runtime behavior is stable.

### Changed

- `walk docs` now renders Markdown from the same symbol index used for JSON output.
- The default `walk docs` path remains compatible with signature-only sources.

### Breaking Changes

None.

### Upgrade

Use the default Markdown path as before, or opt into strict generated reference docs:

```bash
walk docs -o docs/api.md src/main.walk
walk docs --strict --format json -o docs/api.json src/main.walk
```

## v4.0.0 - Professional Tooling

Date: 2026-05-22

v4.0.0 adds first-party editor and project tooling on top of the stable project/package workflow.

### Added

- `walk lsp` stdio language server.
- LSP document diagnostics, formatter integration, hover, go-to-definition, find references, completion, and rename.
- VS Code extension scaffold in `editors/vscode/` with syntax highlighting and LSP startup.
- Neovim filetype, syntax, formatter, and LSP setup files in `editors/neovim/`.
- `walk docs` Markdown API documentation generator.
- `walk debug-map` JSON source symbol map as debugger-adapter groundwork.
- v4 tooling documentation in `docs/V4.md`.
- Focused v4 tests for diagnostics, formatting, completion, navigation, rename, docs, and debug maps.

### Changed

- Module metadata now records resolved source paths so editor navigation can jump from imports and calls to module files.
- README current-surface docs now include the v4 professional tooling commands.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

The v4 editor layer is intentionally local-first. The following remain future roadmap items:

```text
JetBrains plugin implementation
step-through debugger adapter
generated C source maps
remote package registry integration
```

### Upgrade

Install or build the v4 `walk` binary, then point editor integrations at it:

```bash
walk lsp
walk docs -o docs/api.md
walk debug-map -o build/debug-map.json
```

## v3.0.0 - Package Ecosystem

Date: 2026-05-22

v3.0.0 adds a local, file-backed package ecosystem on top of project mode.

### Added

- `walk package init <name>` for package project scaffolding.
- `[dependencies]` in `walk.toml` with exact `MAJOR.MINOR.PATCH` pins.
- `walk package resolve <registry-dir>` for copying pinned dependencies into `.walk/packages/`.
- `walk.lock` with package names, versions, and checksums.
- Package cache verification before project `walk check`, `walk test`, and `walk build`.
- Dotted package imports such as `imp: geometry.core`.
- `walk package publish <registry-dir>` for local registry publishing.
- Publish-time `README.md`, check, and test gates.
- v3 package documentation in `docs/V3.md` and `docs/PROJECTS.md`.
- End-to-end v3 package lifecycle tests.

### Changed

- Module loading can resolve dotted module paths such as `geometry/core.walk`.
- Qualified call checking now treats the final dotted segment as the exported function and the preceding path as the imported module.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

The v3 package ecosystem is local-registry based. The following remain future roadmap items:

```text
remote public package registry
registry authentication
version range solving
multiple package versions in one build
automatic package download during build
```

### Upgrade

For project dependencies, pin exact versions and resolve before checking or building:

```bash
walk package resolve <registry-dir>
walk check --warnings=error
walk test --warnings=error
walk build
```

## v2.2.0 - Simple Generic Composition

Date: 2026-05-22

v2.2.0 adds experimental simple generic functions as the next composition step after structs and methods.

### Added

- Generic function declarations with type parameters, such as `func: first[T](items array[T]) T`.
- Call-site type inference for generic functions.
- Generic functions over scalar values, arrays, structs, and method-returning struct expressions.
- Exported user-module generic functions.
- Predictable C monomorphization for concrete generic calls.
- v2.2 generic pass/fail fixtures and formatter coverage.
- `docs/V2.md` generic function documentation.

### Changed

- `exp:` may now export generic functions from user modules.
- Formatter output keeps generic and array type brackets tight, such as `array[T]`.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

Structs, methods, and generic functions remain experimental in v2.2. The following remain future roadmap items:

```text
traits
interfaces
generic structs
generic methods
explicit type-argument calls
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

## v2.1.0 - Methods

Date: 2026-05-22

v2.1.0 adds experimental methods on top of the v2 struct surface. Methods are receiver functions, not class-based OOP.

### Added

- Method declarations with receiver syntax, such as `func: User.is_adult(self User) bool`.
- Method calls on struct values, such as `user.is_adult()`.
- Receiver-type namespacing so different structs may use the same method name.
- Type checking for method receivers and ordinary method arguments.
- Generated C lowering that keeps method calls explainable as receiver functions, such as `User__is_adult(user)`.
- v2.1 method pass/fail fixtures and formatter coverage.
- `docs/V2.md` method documentation.

### Changed

- Dotted calls now preserve receiver expressions so struct method calls can be distinguished from imported module calls.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

Structs and methods remain experimental in v2.1. The following remain future roadmap items:

```text
traits
interfaces
generic structs
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

## v2.0.0 - Data Modeling

Date: 2026-05-22

v2.0.0 adds experimental struct-based data modeling. The v1 compatibility contract remains documented separately in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

### Added

- `struct:` declarations with fixed typed fields.
- Positional struct constructors.
- Dot field reads and mutable field assignment.
- Structs as function parameters and return values.
- Arrays of structs, indexed field access, and mutable array-element fields.
- Module-declared structs returned by exported module functions.
- v2 struct pass/fail fixtures and formatter coverage.
- `docs/V2.md` for the experimental v2 data-modeling surface.

### Changed

- User modules may now contain `struct:` declarations at top level.
- `struct` is now a reserved word.
- Generated C includes typedefs for WalkLang structs and arrays of structs.

### Breaking Changes

- `struct` can no longer be used as a variable, function, field, or expression name.

### Removed

None.

### Experimental Or Draft

Structs are implemented but remain experimental in v2.0. The following remain future roadmap items:

```text
methods
traits
interfaces
generic structs
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

Rename any user-defined binding named `struct`.

## v1.5.0 - Compatibility Release Preparation

Date: 2026-05-22

v1.5 prepares WalkLang for a stable v1.x line. It does not intentionally change the v1 language syntax or stable standard-library behavior.

### Added

- Versioned v1 compatibility policy in `docs/COMPATIBILITY.md`.
- Official install instructions in `docs/INSTALL.md`.
- Migration guide in `docs/MIGRATING.md`.
- Deprecation policy in `docs/DEPRECATION.md`.
- v1 compatibility fixtures under `tests/compat/v1/`.
- `TestV15CompatibilitySuite...` tests that compile/run stable v1 programs and check representative stable diagnostics.

### Changed

- `README.md` and `docs/V1.md` now describe the current surface as v1.5.
- CI release artifact generation now uses `v1.5.0`.
- `scripts/stress-v1.sh` reports the v1.5 stress path.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

The following remain outside the v1.5 compatibility promise:

```text
file/json/matrix APIs
structs
methods
traits
interfaces
closures
package manager behavior
debugger and full LSP behavior
```

### Upgrade

Install or build the v1.5 CLI, then run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
```

Project users should also run:

```bash
walk check --warnings=error
walk test
walk build
```
