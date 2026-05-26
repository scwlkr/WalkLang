# WalkLang Standard Platform Stays In One Monorepo

WalkLang is one language with one standard platform, so the language core,
official standard platform areas, docs, tests, generated references, runtime
support, and release gates stay together in this repository. The decision
favors coherence, proof parity, and synchronized releases over splitting
official platform capabilities into separate repos or independently versioned
packages.

## Consequences

The monorepo must still have internal boundaries. Standard platform areas such
as CLI, TUI, web app, native desktop, database, game, automation, and rich
runtime support should be organized as explicit areas with their own docs,
tests, import surfaces, runtime contracts, and generated reference coverage, but
they remain part of the same project version and release gate.
