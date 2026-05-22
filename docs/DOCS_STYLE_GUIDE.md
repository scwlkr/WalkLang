# WalkLang Docs Style Guide

Build docs in narrow, source-grounded slices. The style target is Rust-like:
short front doors, clear version boundaries, practical examples, and generated
reference pages that come from documented public symbols instead of handwritten
API lists.

The planned hosted docs entry point is:

```text
https://walklang.wlkrlabs.com/docs
```

## Page Roles

Use one primary role per page:

- **Root README:** repository identity, quick start, source install, help,
  contribution, license, and trademark.
- **docs/README.md:** hosted docs front door and map.
- **Tutorial/how-to docs:** task-first guidance with runnable commands.
- **Reference docs:** generated from `walk docs` and structured comments.
- **Explanation docs:** architecture, design rules, compatibility, roadmap, and
  release context.

Do not turn one page into all of these at once.

## Writing Rules

- Lead with what the page is for.
- Use present tense and active voice.
- Prefer short sections with one concept each.
- Keep examples small enough to run or inspect.
- Mark experimental, draft, stable, deprecated, and removed features clearly.
- Link to adjacent docs instead of duplicating long explanations.
- Do not claim a command, website route, package workflow, or language feature
  exists unless the repo proves it.
- Keep root README prose short. Put detailed usage in `docs/`.

## Generated Reference Pipeline

Docs are written as structured comments directly above public symbols. A
generator extracts those comments plus real symbol metadata from source and
registries, writes Markdown and JSON reference docs, and CI should fail when
public symbols are undocumented or generated docs are stale.

Required pipeline:

```text
source code/registries
  -> doc extractor
  -> docs.json
  -> Markdown renderer
  -> docs/reference/*.md
  -> docs-check in CI
```

Required comment format:

```walk
/// Summary: One sentence.
/// Params:
/// - x: meaning
/// Returns: meaning
/// Example:
/// ```walk
/// ...
/// ```
/// Since: 0.1.0
```

Required JSON fields:

```text
kind, name, path, signature, summary, params, returns, examples, since
```

## Current Commands

```bash
walk docs --strict -o docs/reference/api.md src/main.walk
walk docs --strict --format json -o docs/reference/api.json src/main.walk
```

Future stable commands:

```bash
make docs
make docs-check
```

## Reference Rules

- Public symbol without docs = failure.
- Missing required field = failure.
- Stale generated docs = failure.
- Reference docs are generated, not hand-written.
- Start by documenting 2-3 existing public symbols end-to-end.

## Docs QA

Before shipping a docs pass:

```bash
git diff --check
go test -count=1 ./...
```

For docs that mention generated API output, also prove the command works against
a real source file:

```bash
go build -o build/walk ./cmd/walk
./build/walk docs --strict -o build/api.md examples/v1.walk
./build/walk docs --strict --format json -o build/api.json examples/v1.walk
```

For hosted docs changes, verify local Markdown links and keep links relative
inside `docs/` unless the target is intentionally external.
