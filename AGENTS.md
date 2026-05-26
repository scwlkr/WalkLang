# Overview

WalkLang is a compiled, indentation-based language with obvious, deterministic syntax.
Compiler target: `.walk` → C++ compiler → generated C + Walk C runtime → native executable

# General Guide

- After each non-trivial project conversation, update `docs/STATUS.md` with the current version, verification, and next step.
- After each successfully completed task proceed with commiting/syncing changes and going through the new release flow
- Every feature should be backed by tdd, it is unacceptable to have a feature not proven as either working/failed. e.g. if you have a random.choice function you must always verify that it is truly random via tdd.

# Language Documentation Standard

- Treat `docs/LANGUAGE_CONCEPTS.md` as the canonical vocabulary and docs-governance standard before creating or reshaping language docs.
- New language docs must use the concepts defined there, choose one clear doc role, and label behavior as stable, draft, experimental, planned, deprecated, or removed.
- Language docs and implementation move in the same TDD slice: update the contract or draft doc, add positive proof, add negative proof when invalid forms matter, update formatter/snapshots when syntax or backend output changes, then verify through the public compiler or CLI.
- Do not claim a syntax form, type rule, runtime behavior, or standard-library API exists unless repo tests, fixtures, generated docs, or native execution prove it.

# Agent Context Shortcuts

For named `docs/ROADMAP.md` phase work, read only:

- `docs/STATUS.md`
- the named roadmap phase section
- adjacent phase headings when needed for boundaries
- the implementation files and tests relevant to that phase

Do not reload public scwlkr context docs unless access, integrations, secrets, or missing repo context make them necessary.

For ordinary WalkLang roadmap phases, default to Tall Talents `plan-execution` plus `verification-gate`. Inspect the full Tall Talents index only if the task type changes, a named skill/talent is requested, or there is evidence another talent may apply.

# Side Notes

- WalkLang source code will be located on github
- WalkLang will have an official minimal website at walklang.wlkrlabs.com
