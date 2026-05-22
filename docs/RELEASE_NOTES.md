# WalkLang Release Notes

## Unreleased

No unreleased changes.

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
