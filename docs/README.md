# WalkLang Documentation

WalkLang docs are planned for <https://walklang.wlkrlabs.com/docs>. This
directory is the source for that hosted documentation path.

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

- [v1 / v1.5 stable contract](V1.md)
- [v2 data modeling, methods, and generics](V2.md)
- [v3 package ecosystem](V3.md)
- [v4 professional tooling](V4.md)
- [v5 runtime and backend maturity](V5.md)
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
walk docs --strict -o docs/reference/api.md src/main.walk
walk docs --strict --format json -o docs/reference/api.json src/main.walk
```

Reference docs under `docs/reference/` are generated artifacts. Keep source
comments and registries authoritative, then regenerate reference output from the
compiler.

## Hosting Contract

The planned public docs home is:

```text
https://walklang.wlkrlabs.com/docs
```

Use stable, relative links inside this directory so the same Markdown works in
GitHub and on the hosted site. Do not describe unshipped website routes as live
docs pages until the website publishes them.
