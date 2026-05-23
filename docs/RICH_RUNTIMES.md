# Rich Runtime Tracks

Rich runtimes are separate design tracks. They must not make core CLI, file,
process, JSON, terminal, or HTTP helpers carry browser, graphics, server, or
event-loop complexity.

## HTML Text Helpers

Status: draft compiler APIs implemented.

The `html` module only generates escaped text strings:

```text
html.escape(text) -> string
html.h1(text) -> string
html.p(text) -> string
html.button(text) -> string
```

These helpers do not write files, run a browser, start a server, or own assets.
Use `file.write` when a program wants to save generated HTML.

## Web Server Package

Status: future first-party package or alternate runtime.

Requirements before implementation:

- request/response type design
- routing design
- recoverable network errors
- timeout and body-size policy
- event-loop or blocking-concurrency decision
- security policy for headers, cookies, auth, path traversal, and local files

This should not be a core language keyword family.

## Native Graphics Package

Status: future package or alternate backend.

Requirements before implementation:

- windowing backend decision
- event loop design
- asset loading policy
- input model for keyboard and pointer state
- frame timing policy
- platform support matrix

Graphics must stay outside core IO until the package boundary and runtime model
are clear.

## WASM And Browser Backend

Status: future backend research.

Requirements before implementation:

- target selection syntax or CLI flag
- generated output contract
- DOM and JS interop boundary
- async and event-loop model
- package compatibility story
- browser testing strategy

Browser compilation is a backend decision, not an extension of `out:`,
`file`, or `http`.

## Compiler Explorer And Playground

Status: future tool integration.

Requirements before implementation:

- sandboxing and execution limits
- generated C display policy
- package availability policy
- hosted compiler version pinning
- privacy and telemetry policy
- abuse and resource controls

The playground can consume compiler APIs once they are stable enough, but it
must not force new runtime semantics into the language.
