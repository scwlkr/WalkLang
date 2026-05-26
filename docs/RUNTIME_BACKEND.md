# WalkLang Runtime and Backend Maturity

WalkLang keeps C as the primary backend and tightens the runtime model around
generated output that can be built, inspected, and shipped.

## Backend Contract

Generated C must stay:

```text
predictable
portable
debuggable
readable enough to inspect
```

WalkLang source still follows the same pipeline:

```text
.walk -> generated C + Walk C runtime -> native executable
```

As of v6.0.0, the C++/C compiler uses the same backend shape for source-file
builds:

```text
.walk -> C++ parser/checker -> typed IR -> generated C + Walk C runtime -> native executable
```

`walk emit-c`, `walk build`, `walk run`, and `walk test` are implemented
through that C++ IR and C emitter. Project, package, formatter, docs, LSP, and
REPL workflows also run through the same C++/C toolchain.

## Build Modes

Builds have an explicit mode. Debug is the default and emits native compiler
flags suitable for inspection:

```bash
walk build main.walk -o build/main --mode debug
walk run main.walk --debug
```

Debug mode adds:

```text
-g
-O0
```

Release mode keeps optimized native builds:

```bash
walk build main.walk -o build/main --mode release
walk run main.walk --release
```

Release mode adds:

```text
-O3
-DNDEBUG
```

`--release` remains a compatibility alias for `--mode release`; `--debug` is
an alias for `--mode debug`. Custom compiler flags still work and are appended
after the selected mode flags:

```bash
walk build main.walk -o build/main --mode release --cflag -DWALK_TRACE=0
```

Use `--cc` or `WALK_CC` to choose the native compiler.

## Runtime Layer

Generated C includes `walk_runtime.h` and calls the stable internal
`walk_rt_*` ABI. `walk build` links emitted C with:

```text
runtime/walk_runtime.c
runtime/platform/walk_platform_posix.c or runtime/platform/walk_platform_windows.c
```

`walk emit-c` stays useful as an inspection format by emitting a short comment
that names the runtime C files required for a native build.

The runtime header defines Walk scalar aliases, runtime-owned array structs,
draft result structs, and helper entrypoints for printing, strings, arrays,
files, directories, paths, processes, JSON, terminal helpers, HTTP, and HTML.
The platform layer contains host-specific terminal, file-existence, directory,
path separator, cwd, chdir, and temporary-path behavior.

The helper layer keeps compiler-emitted code predictable without exposing
pointers, allocation calls, or runtime ownership to WalkLang source.
Phase 11 proves the draft runtime families through native conformance fixtures
under `tests/runtime_modules/`.

## Memory Model

WalkLang still has no source-level memory management.

```text
no malloc syntax
no free syntax
no pointer syntax
no public garbage collector promise
```

Array literals allocate item storage through the runtime helper. That storage is
owned by the generated program for the process lifetime, so arrays can be
returned from functions without pointing at expired stack memory.

Example:

```walk
func: numbers() array[int]
    var: nums = [4, 5, 6]
    return: nums

var: got = numbers()
out: got[0]
```

The generated C keeps the allocation explicit in runtime calls and keeps user
statements tagged with source comments.

## Generated C Inspectability

Generated C starts with a short backend comment, a `walk_runtime.h` include,
and a link comment. Emitted source statements include comments like:

```c
/* source: main.walk:6:1 */
```

These comments do not change runtime behavior. They make emitted functions and `main` easier to inspect while preserving predictable C output and snapshot coverage.
The C++ emitter preserves this format for the current snapshot fixtures so
generated-C diffs stay deterministic during the port.

## Draft Runtime Backends

Draft IO helpers grow the C runtime without changing the stable feature set.

Current draft runtime families include:

```text
console and stdin helpers
local files, directories, and paths
process execution
JSON text validation
terminal UX helpers
HTTP client helpers
HTML text helpers
```

Draft HTTP deliberately delegates to the system `curl` executable at runtime
instead of linking a C TLS library into generated output. This keeps release
cross-builds portable while the API remains draft. Programs that use `http`
need `curl` available on `PATH`; missing backend, DNS, TLS, timeout, HTTP
status, and size-limit failures are returned through `HttpResult` values.
Conformance fixtures keep empty-method and empty-URL failure results
deterministic for the active C++/C `walk` compiler.

Rich runtimes stay outside the core runtime layer:

```text
web server runtime
native graphics
WASM/browser backend
compiler explorer/playground integration
```

Those tracks require separate package or backend designs before implementation.
See [Draft networking](NETWORKING.md) and [Rich runtime tracks](RICH_RUNTIMES.md).

## Non-Goals

Runtime and backend maturity does not add:

```text
LLVM backend
WASM backend
manual memory management syntax
garbage collection
ownership annotations in WalkLang source
step-through debugger adapter
```
