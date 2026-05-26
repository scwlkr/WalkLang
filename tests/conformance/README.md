# WalkLang Conformance Oracle

This directory stores the recorded behavior that was captured from the former
reference compiler before the Phase 11 removal. Those expected artifacts are now
the oracle for the active C++/C `walk` compiler.

The manifest is `manifest.tsv`. Each non-comment row has these tab-separated
fields:

```text
id	kind	mode	source	cwd	stdin	native
```

- `id` is the stable expected-artifact key under `expected/`.
- `kind` is `pass`, `fail`, `compat`, `runtime`, `snapshot`, or `walktop`.
- `mode` selects the command shape.
- `source` is the fixture or fixture group covered by the row.
- `cwd` is the working directory for project-mode rows, or `.`.
- `stdin` is `-` unless the fixture needs deterministic input.
- `native` is `yes` when the row proves native executable behavior.

## Verify The Oracle

Build the active compiler and verify it against the recorded oracle:

```bash
make walk WALK_VERSION=v6.0.0
make conformance WALK_VERSION=v6.0.0
```

The runner compares stdout, stderr, success/failure status, and generated C for
snapshot rows. It intentionally ignores temp-path stdout from `emit-c` because
the generated C content is the contract for those rows.

`tests/fail/private_math.walk` is a support module imported by
`tests/fail/private_module_func.walk`, not a failing entry fixture. The oracle
therefore verifies the failing importing fixture, which also proves the support
module is loaded.

## Maintenance

`--record` remains available for deliberate oracle updates:

```bash
WALK_REF=$PWD/build/walk tests/conformance/run.sh --record
```

Do not record over expected artifacts as a shortcut around a failing candidate.
First prove the behavior change is intentional, update the relevant language
contract, and keep generated C snapshots reviewable.
