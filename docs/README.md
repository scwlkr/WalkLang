# WalkLang Documentation

WalkLang docs are built into a static site for `walklang.wlkrlabs.com/docs`.
This directory remains the source for the hosted documentation path.

Use this page as the docs front door. The root README explains the repository;
these docs explain how to install, learn, use, verify, and evolve WalkLang.

## Get Started

- [Install WalkLang](INSTALL.md)
- [Build projects](PROJECTS.md)
- [Learn the syntax](SYNTAX.md)
- [Read the language specification](SPEC.md)

## Use The Language

- [Language concepts and docs standard](LANGUAGE_CONCEPTS.md)
- [Standard library](STDLIB.md)
- [Diagnostics and warnings](ERRORS.md)
- [Compatibility promise](COMPATIBILITY.md)
- [Migration guide](MIGRATING.md)
- [Deprecation policy](DEPRECATION.md)

## Use The Tools

- [Project mode and packages](PROJECTS.md)
- [Editor tooling, LSP, docs generation, and debug maps](TOOLING.md)
- [Runtime and backend model](RUNTIME_BACKEND.md)
- [Draft networking](NETWORKING.md)
- [Rich runtime tracks](RICH_RUNTIMES.md)
- [Architecture](ARCHITECTURE.md)

## Current Version

WalkLang is currently `v5.13.1`. That single project version covers the
compiler, tooling, backend, release artifacts, docs, and implemented language
surface.

Features inside `v5.13.1` use maturity labels:

- stable: core syntax, diagnostics, modules, tests, and standard-library helpers
- draft: `do:`, `defer:`, `io`, `parse`, `process`, `file`, `dir`, `path`,
  `json`, `term`, `http`, and `html`
- experimental: structs, methods, and simple generic functions

- [Stable feature specification](SPEC.md)
- [Standard library](STDLIB.md)
- [Compatibility promise](COMPATIBILITY.md)
- [Release notes](RELEASE_NOTES.md)
- [Current status](STATUS.md)

## Project Direction

- [Roadmap](ROADMAP.md)
- [Explicit systems track](EXPLICIT_SYSTEMS_TRACK.md)
- [Design rules](DESIGN_RULES.md)
- [Purpose](PURPOSE.md)
- [Docs style guide](DOCS_STYLE_GUIDE.md)

## Generated Reference Docs

WalkLang supports rustdoc-style structured comments on public symbols. Generate
Markdown and JSON reference output from real source with:

```bash
scripts/build-docs-site.sh
```

Reference docs under `docs/reference/` are generated artifacts. The static site
under `public/` publishes the rendered docs pages, generated API reference, raw
Markdown, and `api.json` from the same compiler output.

## Hosting Contract

The public docs home is configured for:

```text
walklang.wlkrlabs.com/docs
```

The target public scheme is HTTPS once GitHub Pages issues the custom-domain
certificate. `docs/STATUS.md` records the current live HTTP/HTTPS state.

Use stable, relative links inside this directory so the same Markdown works in
GitHub and on the hosted site. Run `scripts/check-docs-site.sh` before
publishing docs changes.
