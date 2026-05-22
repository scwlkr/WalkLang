# WalkLang Release Notes

## v1.5.0 - Compatibility Release Preparation

Date: 2026-05-22

v1.5 prepares WalkLang for a stable v1.x line. It does not intentionally change the v1 language syntax or stable standard-library behavior.

### Added

- Versioned v1 compatibility policy in `docs/COMPATIBILITY.md`.
- Official install instructions in `docs/INSTALL.md`.
- Migration guide in `docs/MIGRATING.md`.
- Deprecation policy in `docs/DEPRECATION.md`.
- v1 compatibility fixtures under `tests/compat/v1/`.
- `TestV15CompatibilitySuite...` tests that compile/run stable v1 programs and check representative stable diagnostics.

### Changed

- `README.md` and `docs/V1.md` now describe the current surface as v1.5.
- CI release artifact generation now uses `v1.5.0`.
- `scripts/stress-v1.sh` reports the v1.5 stress path.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

The following remain outside the v1.5 compatibility promise:

```text
file/json/matrix APIs
structs
methods
traits
interfaces
closures
package manager behavior
debugger and full LSP behavior
```

### Upgrade

Install or build the v1.5 CLI, then run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
```

Project users should also run:

```bash
walk check --warnings=error
walk test
walk build
```

