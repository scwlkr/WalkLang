# WalkLang Package Ecosystem

WalkLang includes a local package workflow on top of project mode and user
modules.

Packages are ordinary WalkLang projects with:

```text
walk.toml
README.md
src/<package>/<module>.walk
tests/*_test.walk
```

The package manager is intentionally local and file-backed. It does not add a
remote public registry protocol yet.

## Create A Package

Create a package project:

```bash
walk package init geometry
cd geometry
```

Generated layout:

```text
geometry/
  README.md
  walk.toml
  src/
    main.walk
    geometry/
      core.walk
  tests/
    main_test.walk
  build/
```

Package names must contain only letters, numbers, and `_`, and must start with a letter or `_`. This keeps package names usable as import namespaces.
`std` is reserved as a future first-party collection root. Publishing also
rejects package names reserved for current or future built-in roots, including
current built-in module names such as `math`, `string`, `array`, `io`, `file`,
`process`, `json`, `term`, `http`, and `html`.

## Package Modules

Package modules are imported with dotted names:

```walk
imp: geometry.core

out: geometry.core.double(3)
```

The first segment is the collection root. In `imp: geometry.core`, the
collection root is `geometry`, the module path is `geometry/core.walk`, and the
generated C prefix is `geometry__core__`.

The import maps to:

```text
src/geometry/core.walk
```

Exports still use `exp:` inside the module.

```walk
func: double(x int) int
    return: * x 2

exp: double
```

## Pin Dependencies

Project dependencies live in `walk.toml`.

```toml
name = "shape_app"
version = "0.1.0"
entry = "src/main.walk"

[build]
output = "build/shape_app"
mode = "debug"

[dependencies]
geometry = "0.1.0"
```

Dependency versions must use `MAJOR.MINOR.PATCH`.

## Resolve Dependencies

Resolve from a local registry directory:

```bash
walk package resolve ../registry
```

The resolver copies packages into:

```text
.walk/packages/<name>/<version>/
```

It writes `walk.lock` with each package name, version, and checksum. Project `walk check`, `walk test`, and `walk build` require the lock and verify cached package contents against it.

If the cache changes after locking, project commands fail until dependencies are resolved again.

## Publish A Package

Publish to a local registry directory:

```bash
walk package publish ../registry
```

Publish rules:

```text
README.md is required and must be non-empty
package name must be import-safe
package name must not be a reserved built-in collection root
package version must be MAJOR.MINOR.PATCH
walk check --warnings=error must pass
walk test --warnings=error must pass
existing registry versions are not overwritten
build/, .walk/, .git/, and walk.lock are not published
```

The published package lands at:

```text
<registry>/<name>/<version>/
```

## Reproducibility

Package reproducibility comes from:

```text
explicit dependency versions in walk.toml
locked package checksums in walk.lock
local package cache verification before build/check/test
deterministic package source lookup
publish-time check and test gates
```

## Non-Goals

The package ecosystem does not add:

```text
remote registry protocol
package account authentication
version range solving
multiple versions of the same package in one build
automatic package download during build
```
