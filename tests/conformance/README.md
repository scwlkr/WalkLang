# WalkLang Conformance Oracle

This directory records the current reference compiler behavior as the oracle for
the systems compiler port.

The manifest is `manifest.tsv`. Each non-comment row has these tab-separated
fields:

```text
id	kind	mode	source	cwd	stdin	native
```

- `id` is the stable expected-artifact key under `expected/`.
- `kind` is `pass`, `fail`, `compat`, `snapshot`, or `walktop`.
- `mode` selects the reference command shape.
- `source` is the fixture or fixture group covered by the row.
- `cwd` is the working directory for project-mode rows, or `.`.
- `stdin` is `-` unless the fixture needs deterministic input.
- `native` is `yes` when the row proves native executable behavior.

## Record The Oracle

Build the current reference compiler and refresh expected artifacts:

```bash
go build -trimpath -ldflags "-X main.version=v5.14.1" -o build/walk-ref ./cmd/walk
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --record
```

Recording validates that the manifest still covers every current pass fixture,
fail fixture, compatibility fixture, snapshot fixture, and walktop fixture
group before writing expected output.

`tests/fail/private_math.walk` is a support module imported by
`tests/fail/private_module_func.walk`, not a failing entry fixture. The oracle
therefore records the failing importing fixture, which also proves the support
module is loaded by the reference compiler.

## Verify The Oracle

Verify the reference compiler against the recorded oracle:

```bash
WALK_REF=$PWD/build/walk-ref tests/conformance/run.sh --verify
```

When a candidate compiler exists, verify both compilers against the same oracle:

```bash
WALK_REF=$PWD/build/walk-ref \
WALK_CANDIDATE=$PWD/build/walk-candidate \
tests/conformance/run.sh --verify
```

The runner compares stdout, stderr, success/failure status, and generated C for
snapshot rows. It intentionally ignores temp-path stdout from `emit-c` because
the generated C content is the contract for those rows.

## Candidate Slices

The systems port can verify staged C++ candidate surfaces without requiring
every later command to be ported at once:

```bash
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --parse
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --check
WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --fail-diagnostics
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --emit-c
WALK_REF=$PWD/build/walk-ref WALK_CANDIDATE=$PWD/build/walk-cpp tests/conformance/run.sh --native
```

`--emit-c` covers generated-C snapshot rows. `--native` covers rows marked
`native=yes`, including current pass, compatibility, and walktop native proofs.
