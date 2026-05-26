# WalkLang Context

WalkLang is a small compiled language project. This context records project
language that should stay consistent across docs, roadmap, release, and tooling
work.

## Language

**General Use Language**:
WalkLang's intended role as the primary language for the owner's projects,
from small CLI tools through internal apps, TUIs, native desktop apps,
database-backed tools, games, and project-specific products.
_Avoid_: scripting language, beginner language, toy language

**Personal Foundation Language**:
A language designed to become the base layer for the owner's own software
ecosystem before it tries to serve every outside programming niche.
_Avoid_: niche demo language, learning-only language

**Standard Platform**:
The one official WalkLang product surface that combines the small language core,
standard library, tooling, runtime support, docs, tests, and release gates.
_Avoid_: scattered tracks, separate platform projects, plugin pile

**Standard Platform Monorepo**:
The repository structure rule that WalkLang's language core, official standard
platform areas, docs, tests, generated references, runtime support, and release
gates live together in one monorepo.
_Avoid_: separate official platform repo, split standard platform package

**Standard Platform Area**:
An official capability area inside the one WalkLang standard platform, such as
CLI, TUI, web app, native desktop, database, game, automation, or rich runtime
support.
_Avoid_: runtime track, random extension, optional afterthought, disconnected plugin

**Capability Area**:
A user-facing way to organize the standard platform around what the developer
is trying to build, such as CLI, TUI, web app, desktop app, database-backed
tool, game, or automation.
_Avoid_: backend layer, implementation bucket, runtime subsystem

**CLI Standard Platform Area**:
The first official standard platform area. It covers building command-line
programs with arguments, environment access, files, process execution,
terminal-safe output, configuration, diagnostics, testing, docs, and release
proof through the same WalkLang monorepo gate.
_Avoid_: random process helpers, shell-script replacement only

**CLI Proof App**:
A real WalkLang-built command-line application used to prove the CLI Standard
Platform Area end to end with arguments, environment access, configuration,
file IO, process calls, terminal output, simple non-interactive animation,
tests, docs, and release proof.
_Avoid_: hello-world CLI, toy fixture, isolated helper demo, static report only, live TUI app

**System Monitor Proof App**:
The first CLI Proof App, named `walktop`. It is a small WalkLang-built system
monitor, similar in spirit to a tiny `htop`/`btop`, used to prove CLI platform
capabilities without committing to live keyboard input or full TUI behavior.
_Avoid_: repo-status proof app, hello-world proof app, live TUI proof app

**Official WalkLang Tool**:
A real command shipped for use with WalkLang, built and verified through the
monorepo release gate. An official tool may also serve as a proof app, but it is
not example-only or tutorial-only.
_Avoid_: demo-only app, examples-only tool, toy shipped command

**Standalone WalkLang Tool**:
An official tool exposed as its own native command built from WalkLang source
and installed by the WalkLang release/install flow, rather than as a Go-side
`walk` compiler subcommand.
_Avoid_: compiler subcommand proof, Go-only tool extension

**Official Tool Install Contract**:
The rule that official WalkLang tools are installed by the normal WalkLang
install flow once they exist and pass the release gate. A platform-specific
build failure must fail clearly instead of silently skipping the tool.
_Avoid_: separate manual install for official tools, silent skipped tool

**Official Tool Source Path**:
The monorepo location for shipped WalkLang-built tools. Official tool source
uses `tools/<tool-name>/` with its own WalkLang project files, tests or
testdata, and local README.
_Avoid_: examples-only source path, Go cmd path for WalkLang-built tools

**OS Command Data Source**:
The first data-source policy for the System Monitor Proof App. It gathers live
or test fixture data by calling existing operating-system commands through
WalkLang process APIs, then parsing their text output, before any dedicated
WalkLang `system` module exists.
_Avoid_: dedicated system API first, hidden native monitor backend

**System Monitor Fixture Mode**:
The deterministic test mode for the System Monitor Proof App. Live mode may call
real OS commands, but tests use fixture commands or fixture files so parsing,
rendering, animation frames, failure paths, and fallback output do not depend on
the current machine's live CPU, memory, disk, or process state.
_Avoid_: live-metric-only tests, host-dependent proof

**Release-Quality Proof App**:
The quality bar for proof apps that ship as official WalkLang tools. A proof app
must look and behave like something worth releasing: polished terminal output,
clear layout, intentional errors, deterministic test modes, and no raw debug
dump feel.
_Avoid_: embarrassing demo, rough proof of concept, debug-output tool

**Color-Forward Terminal Output**:
The presentation rule for release-quality terminal tools: use intentional color
by default when the terminal supports it, while preserving plain deterministic
fallback output for `NO_COLOR`, redirected output, and tests.
_Avoid_: colorless by default, unreadable color noise, raw ANSI everywhere

**Terminal API Styling**:
The styling rule for official terminal tools. Terminal color, style, clear, and
cursor behavior should use WalkLang terminal APIs where possible; raw ANSI
strings are only acceptable when they expose a missing API that should be
promoted into the platform.
_Avoid_: app-local ANSI hacks, bypassing terminal APIs

**Pressure-Tested Terminal Primitive**:
A small terminal API added because `walktop` or another official tool directly
needs it to meet the release-quality bar. It must use the normal proof loop and
must not expand into a broad TUI framework by default.
_Avoid_: speculative TUI framework, unused terminal primitive

**Impressive By Default**:
The quality expectation that official proof tools should make WalkLang feel
capable immediately, without requiring special flags or manual styling to look
release-worthy.
_Avoid_: opt-in polish, bare debug layout, placeholder interface

**Platform Proof Parity**:
The rule that official standard platform areas must be specified, tested,
documented, and release-verified with the same discipline as core language
features so imports, runtime helpers, docs, and generated output do not drift
apart.
_Avoid_: best-effort platform testing, docs-only platform support

**Project Version**:
The single public version number for WalkLang, covering the compiler, tooling,
docs, release artifacts, and implemented language surface.
_Avoid_: language version, compiler version, release version, version layer

**Feature Status**:
A maturity label attached to a feature inside a Project Version.
_Avoid_: stable version, unstable version, experimental version

**Historical Version Reference**:
A past version number kept only when recording release or migration history.
_Avoid_: current language version, current compiler version

**Purpose-Based Developer Path**:
A source, fixture, script, or docs filename named for what it does instead of
the old milestone that first introduced it.
_Avoid_: milestone filename, versioned fixture path

**Stable Feature**:
A feature in the current Project Version that should keep working unless a
future release explicitly changes it.
_Avoid_: stable language contract version

**Draft Feature**:
A feature in the current Project Version that is implemented but may change
before it is treated as stable.
_Avoid_: draft version, unstable version

**Experimental Feature**:
A feature in the current Project Version that is implemented for exploration
and is not yet part of the expected stable user surface.
_Avoid_: experimental language version

## Relationships

- A **Project Version** contains many features.
- Each feature may have one **Feature Status**.
- A **Stable Feature**, **Draft Feature**, and **Experimental Feature** are
  different statuses inside the same **Project Version**, not separate version
  lines.
- A **Historical Version Reference** may appear in release notes or migration
  history, but it should not be used to describe current WalkLang status.
- A **Personal Foundation Language** can still be a **General Use Language**:
  the initial design center is the owner's project ecosystem, not a narrow
  scripting-only or beginner-only audience.
- A **General Use Language** should grow through a small explicit core plus one
  **Standard Platform** made of **Standard Platform Areas**, not by turning every
  project capability into core language syntax.
- The **Standard Platform** is a **Standard Platform Monorepo**. Official
  platform areas stay in the main repo and main release gate.
- A **Standard Platform Area** should be organized as a **Capability Area**:
  docs, tests, examples, and proof gates answer what a developer can build, not
  which internal backend layer happens to implement it.
- The first **Standard Platform Area** is the **CLI Standard Platform Area**.
  It should prove the platform-area model before TUI, database, web app,
  desktop, game, and automation areas are promoted.
- The **CLI Standard Platform Area** should be proven by a **CLI Proof App** with
  animated terminal output, similar in spirit to a very small `htop`/`btop`
  style command, not by isolated demos of individual helper APIs. It should stay
  in CLI scope and avoid live keyboard/event-loop TUI behavior.
- The first **CLI Proof App** is the **System Monitor Proof App**, named
  `walktop`.
- `walktop` is an **Official WalkLang Tool** and **Standalone WalkLang Tool**,
  not example-only and not just a `walk` compiler subcommand.
- `walktop` follows the **Official Tool Install Contract**.
- `walktop` source lives under the **Official Tool Source Path**:
  `tools/walktop/`.
- The **System Monitor Proof App** uses **OS Command Data Source** first to
  prove process execution, text parsing, fallback behavior, and terminal output
  before adding any dedicated `system` module.
- The **System Monitor Proof App** needs **System Monitor Fixture Mode** for
  deterministic tests and CI.
- The **System Monitor Proof App** must meet the **Release-Quality Proof App**
  bar before it is installed as an official tool.
- `walktop` should use **Color-Forward Terminal Output** and be **Impressive By
  Default** while still respecting plain-output fallback rules.
- `walktop` should use **Terminal API Styling** rather than raw app-local ANSI
  strings wherever possible.
- Missing terminal support may become a **Pressure-Tested Terminal Primitive**
  only when `walktop` directly needs it; this must not become a broad TUI
  framework in the CLI slice.
- **Standard Platform Areas** need **Platform Proof Parity** so platform growth
  does not leave imports, docs, tests, generated C, or release artifacts out of
  sync.
- A **Purpose-Based Developer Path** should be used for normal developer
  workflows; old milestone labels should stay out of active source, fixture,
  script, and current docs paths.

## Example dialogue

> **Dev:** "Is WalkLang using a language version and a compiler version?"
> **Domain expert:** "No. WalkLang is on **Project Version** v5.14.0. Some
> features inside that project version are stable, draft, or experimental."

## Flagged ambiguities

- "version" previously meant release version, stable language contract version,
  and experimental language surface version. Resolved: WalkLang has one
  **Project Version**; feature maturity is described with **Feature Status**.
- Old version numbers are allowed only as **Historical Version References** in
  release notes and migration history.
- "scripting language" and "beginner language" understate the project vision.
  Resolved: WalkLang is intended to become a **General Use Language** and
  **Personal Foundation Language** for the owner's projects.
- "first-party tracks" sounded like parallel side projects. Resolved: use
  **Standard Platform Area** for official capability areas inside the one
  WalkLang standard platform, and require **Platform Proof Parity**.
- Official platform areas are not separate repos. Resolved: WalkLang uses a
  **Standard Platform Monorepo**.
- Platform-area organization should follow user-facing **Capability Areas**, not
  backend-shaped implementation buckets.
- The first official capability area is CLI.
- CLI support means a real **CLI Proof App** using the whole area with simple
  non-interactive terminal animation, not a toy hello-world command, static
  report only, or live TUI app.
- The first CLI proof app is `walktop`, a small system monitor.
- `walktop` is a real tool shipped for use with WalkLang, not just an example
  app.
- `walktop` ships as a standalone native command built from WalkLang source and
  installed by the WalkLang release/install flow.
- `walktop` is installed automatically by the normal local WalkLang install
  flow after it passes the release gate.
- `walktop` source lives in `tools/walktop/`, not `examples/` or Go `cmd/`.
- The first system monitor proof app reads machine data through OS commands
  before any dedicated WalkLang system API.
- System monitor tests use fixture mode instead of depending on live host
  metrics.
- `walktop` must look and behave like a release-quality tool, not an
  embarrassing proof-of-concept.
- `walktop` uses color by default on capable terminals and plain output for
  deterministic test, `NO_COLOR`, and redirected-output paths.
- `walktop` styling proves WalkLang terminal APIs instead of bypassing them
  with raw ANSI strings.
- New terminal APIs for `walktop` should be small pressure-tested primitives,
  not speculative TUI framework work.
- Developer-facing paths previously used milestone labels. Resolved: active
  paths use **Purpose-Based Developer Path** names.
