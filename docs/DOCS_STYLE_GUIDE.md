Build rustdoc-style docs.

Docs are written as structured comments directly above public symbols. A generator extracts those comments plus real symbol metadata from source/registries, writes Markdown reference docs, and CI fails when public symbols are undocumented or generated docs are stale.

Required pipeline:
source code/registries
  -> doc extractor
  -> docs.json
  -> Markdown renderer
  -> docs/reference/*.md
  -> docs-check in CI

Required comment format:
/// Summary: One sentence.
/// Params:
/// - x: meaning
/// Returns: meaning
/// Example:
/// ```walk
/// ...
/// ```
/// Since: 0.1.0

Required JSON fields:
kind, name, path, signature, summary, params, returns, examples, since

Required commands:
make docs
make docs-check

Rules:
- Public symbol without docs = failure.
- Missing required field = failure.
- Stale generated docs = failure.
- Reference docs are generated, not hand-written.
- Start by documenting 2–3 existing public symbols end-to-end.