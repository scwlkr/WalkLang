# WalkLang Migration Guide

## v1.5 to v2.0

v2.0 adds experimental structs and reserves the `struct` keyword.

Recommended upgrade check:

```bash
walk version
walk check --warnings=error <entry.walk>
walk test <tests.walk>
walk build <entry.walk> -o build/app
```

For projects:

```bash
walk check --warnings=error
walk test
walk build
```

## What Changed In v2.0

- `struct:` declarations compile.
- Struct values can be constructed, read through dot fields, and assigned through mutable fields.
- Structs work as function parameters, return values, array elements, and values returned by module functions.
- User modules may contain top-level `struct:` declarations.

## Breaking Change In v2.0

`struct` is now reserved. Rename any user-defined variable, function, field, or expression name called `struct`.

Named-field constructors, methods, traits, interfaces, and generic structs are not part of v2.0.

## v1.4 to v1.5

v1.5 is a compatibility release-preparation pass. Stable v1.4 code should not need source changes.

Recommended upgrade check:

```bash
walk version
walk check --warnings=error <entry.walk>
walk test <tests.walk>
walk build <entry.walk> -o build/app
```

For projects:

```bash
walk check --warnings=error
walk test
walk build
```

## What Changed

- Compatibility policy is now explicit.
- Release notes, install instructions, and deprecation policy are now documented.
- Stable v1 behavior has a dedicated compatibility test suite.

## What Did Not Change

- No stable syntax was removed.
- No stable standard-library API was removed.
- No stable diagnostic category was removed.
- No project-mode command was removed.

## Stable Versus Draft

Before depending on a feature, check:

```text
docs/SPEC.md
docs/SYNTAX.md
docs/STDLIB.md
docs/ERRORS.md
docs/COMPATIBILITY.md
```

If a feature is accepted by the compiler but absent from those docs and the compatibility tests, treat it as experimental.

## Breaking Changes

Breaking changes to stable v1 behavior require one of these:

```text
a v2.0.0 language boundary
a documented safety or correctness fix
proof that the behavior was never part of the stable v1 surface
```

When a documented safety fix breaks stable code, release notes must name the break and this guide must describe the migration.
