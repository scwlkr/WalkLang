# Overview

WalkLang is a compiled, indentation-based language with obvious, deterministic syntax.
Compiler target: `.walk` → C++ compiler → generated C + Walk C runtime → native executable

# General Guide

- After each non-trivial project conversation, update `docs/STATUS.md` with the current version, verification, and next step.
- After each successfully completed task proceed with commiting/syncing changes and going through the new release flow
- Prove changed behavior through the public compiler or CLI and capture the result.

# Language Documentation Standard

- Treat `docs/LANGUAGE_CONCEPTS.md` as the canonical vocabulary and docs-governance standard before creating or reshaping language docs.
- New language docs must use the concepts defined there, choose one clear doc role, and label behavior as stable, draft, experimental, planned, deprecated, or removed.
- Update the language contract or draft alongside implementation changes. Verify valid forms and expected errors through the public compiler or CLI.
- Do not claim a syntax form, type rule, runtime behavior, or standard-library API exists unless generated docs or native execution prove it.

# Agent Context Shortcuts

For named `docs/ROADMAP.md` phase work, read only:

- `docs/STATUS.md`
- the named roadmap phase section
- adjacent phase headings when needed for boundaries
- the implementation files and tests relevant to that phase

Do not reload public scwlkr context docs unless access, integrations, secrets, or missing repo context make them necessary.

Tall Talents is paused globally. For ordinary WalkLang roadmap phases, follow the local roadmap/status process directly; do not default to any Tall Talents unless scwlkr explicitly asks to resume or use it.

# Side Notes

- WalkLang source code will be located on github
- WalkLang will have an official minimal website at walklang.wlkr.si

## Linear work queue

- Team: **WLKR.SI** (`WLK`, displayed as `WLKRSI` in Linear).
- Project: [WalkLang](https://linear.app/wlkr-labs/project/walklang-5e79443d2710).
- Linear is the task source of truth. Existing GitHub issues and Markdown plans are historical context; this section supersedes older tracker or backlog guidance.
- Before starting substantive work, read the Linear issue and discussion and check for existing work. Find or create a Linear issue for substantive user-requested work, not every question or minor action.
- Keep status current, include the issue ID in branches and PRs, and post concise outcomes or blockers. Mark Done only when completion criteria are met.
- Do not maintain a competing Markdown backlog or import or sync GitHub issues.
