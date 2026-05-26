# Draft Networking

WalkLang networking is draft-only. It is implemented for native CLI experiments,
not as part of the stable feature set.

## Scope

The current draft `http` module includes:

```walk
imp: http

var: response = http.get('http://127.0.0.1:8080/health')
out: response.ok
out: response.status
out: response.body
out: response.error
```

```text
http.get(url) -> HttpResult
http.post(url, body) -> HttpResult
http.request(method, url, body) -> HttpResult
```

`HttpResult` is:

```walk
struct: HttpResult
    ok bool
    status int
    body string
    error string
```

## Runtime Backend

The C backend keeps release builds portable by delegating draft HTTP requests to
the system `curl` executable at runtime. WalkLang does not link libcurl or ship
a TLS stack in generated C yet.

This is a deliberate draft dependency decision:

- release cross-builds still need only Go and the target C toolchain
- native HTTP programs need `curl` available on `PATH` at runtime
- TLS behavior is owned by the host `curl` build and host certificate store
- missing `curl`, network failures, DNS failures, TLS failures, and timeout
  failures return `HttpResult` data instead of runtime-stopping

## Security Policy

Draft HTTP uses argv-style process execution. It does not invoke a shell.

Default policy:

- redirects are followed
- timeout is 10 seconds
- response bodies are capped at 1 MiB by the runtime request command
- response bodies are UTF-8 text only
- status codes `200` through `399` set `ok true`
- other status codes set `ok false`, preserve `body`, and set
  `error 'http status'`
- empty method or URL returns a recoverable error before any process spawn

Not included yet:

- custom headers
- cookies or a cookie jar
- authentication helpers
- streaming responses
- binary bodies
- server APIs
- async/event-loop APIs

## Testing Policy

Network tests must not depend on public network availability. Compiler/runtime
tests use local loopback HTTP servers and skip only when the draft runtime
backend dependency, `curl`, is unavailable.
The systems compiler runtime-module conformance fixture also covers
deterministic HTTP failure result data for empty method and empty URL inputs
through the active C++/C `walk` compiler.
