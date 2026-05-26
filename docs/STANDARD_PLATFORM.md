# WalkLang Standard Platform

Status: CLI standard-platform slice implemented for `walktop`; future platform
areas remain planned.

WalkLang is intended to become a general use language and personal foundation
language for the owner's projects. The product shape is one language with one
official standard platform, kept in one monorepo and verified through one
release gate.

This document records the standard platform model. The first CLI slice is
implemented through `walktop`; future platform areas remain planned until their
own proof slices ship.

## Platform Model

WalkLang should grow through a small explicit core plus a standard platform.
The standard platform is organized around user-facing capability areas instead
of backend-shaped implementation buckets.

Initial capability areas:

```text
CLI
TUI
database
web app
desktop app
game
automation
```

Official standard platform areas stay in this monorepo. They are not separate
official repos, independently versioned packages, or disconnected plugins.

## Proof Parity

Every official standard platform area needs the same discipline as core
language work:

```text
spec or design doc
source docs
positive tests
negative tests when invalid forms matter
fixture mode for host-dependent behavior
generated reference docs when APIs are exposed
native build or runtime proof
release/install proof when tools are shipped
status and release-note updates
```

Draft means the API may change. It does not mean untested, undocumented, or
half-wired.

## First Area: CLI

The first standard platform area is CLI. CLI is the best first area because it
already exercises the current draft process, file, terminal, parse, and IO
surface without requiring web, database, desktop, game, or full TUI runtime
commitments.

The CLI area should prove:

```text
arguments
environment access
configuration
file IO
process execution
terminal-safe output
color-forward output with plain fallback
deterministic tests
docs and install flow
```

## First Tool: walktop

`walktop` is the first CLI proof app and an official standalone WalkLang tool.
It is a small system monitor, similar in spirit to a tiny `htop` or `btop`, but
it stays in CLI scope. It does not require live keyboard input or commit
WalkLang to a full TUI framework.

`walktop` source lives at:

```text
tools/walktop/
  README.md
  walk.toml
  src/main.walk
  src/walktop.walk
  tests/main_test.walk
  testdata/basic/
```

`walktop` is built from WalkLang source into a standalone native command and
installed by the normal local install flow after it passes the release gate.

## walktop v0 Scope

Minimum command surface:

```text
walktop --once
walktop --frames 5
walktop --fixture tools/walktop/testdata/basic
```

Minimum display:

```text
title and version
OS label
CPU or load bar
memory bar
disk bar
process count
spinner or frame counter
clear warning/error row when data is missing
```

Quality bar:

```text
compact dashboard
aligned metric rows
stable-width bars
intentional color on capable terminals
plain deterministic output for NO_COLOR, redirected output, and tests
no raw command output
no noisy ASCII art
no placeholder/debug layout
```

## Data Source

`walktop` gathers data by calling existing OS commands through WalkLang
process APIs and parsing text output. This proves CLI capability before adding
a dedicated WalkLang `system` module.

Live mode may call real OS commands. Tests must use fixture mode so parsing,
rendering, animation frames, failure paths, and fallback output do not depend on
the current machine's CPU, memory, disk, or process state.

## Terminal Styling

`walktop` uses WalkLang terminal APIs for color, style, clear, and cursor
behavior wherever possible. Raw ANSI strings should not be used as an app-local
shortcut.

If `walktop` needs a small missing terminal primitive to meet the release-quality
bar, add that primitive through the normal proof loop. Do not build a broad TUI
framework in the CLI slice.

## Implemented Slice

The first implementation slice shipped:

- `tools/walktop/` as a real WalkLang project.
- `walktop --once`, `walktop --frames 5`, and `walktop --fixture tools/walktop/testdata/basic`.
- Fixture-mode tests for parse, render, argument, and warning behavior.
- End-to-end native build/run proof for `walktop --once --fixture ...`.
- Install and release scripts that build `walktop` from WalkLang source.
- Docs, generated docs, status, and release notes for the shipped slice.

No new terminal primitive was required for this slice.
