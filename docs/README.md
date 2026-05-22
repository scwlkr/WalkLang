# WalkLang Documentation

WalkLang docs are built into a static site for `walklang.wlkrlabs.com/docs`.
This directory remains the source for the hosted documentation path.

Use this page as the docs front door. The root README explains the repository;
these docs explain how to install, learn, use, verify, and evolve WalkLang.

## Get Started

- [Install WalkLang](INSTALL.md)
- [Build projects](PROJECTS.md)
- [Learn the v1 syntax](SYNTAX.md)
- [Read the v1 language contract](SPEC.md)

## Use The Language

- [Standard library](STDLIB.md)
- [Diagnostics and warnings](ERRORS.md)
- [Compatibility promise](COMPATIBILITY.md)
- [Migration guide](MIGRATING.md)
- [Deprecation policy](DEPRECATION.md)

## Use The Tools

- [Project mode and packages](PROJECTS.md)
- [Editor tooling, LSP, docs generation, and debug maps](V4.md)
- [Runtime and backend model](V5.md)
- [Architecture](ARCHITECTURE.md)

## Track Versions

WalkLang version names use separate stability layers. The stable language
contract is `v1.9`. The current compiler, tooling, backend, release, and docs
milestone is `v5.5.0`. Experimental v2 through v2.2 language features are
implemented, but they are not part of the stable v1 compatibility promise.

- [v1 / v1.9 stable contract](V1.md)
- [v2 data modeling, methods, and generics](V2.md)
- [v3 package ecosystem](V3.md)
- [v4 professional tooling](V4.md)
- [v5 runtime and backend maturity](V5.md)
- [v5.1 public docs and reference site](V5_1.md)
- [Release notes](RELEASE_NOTES.md)
- [Current status](STATUS.md)

## Project Direction

- [Roadmap](ROADMAP.md)
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
