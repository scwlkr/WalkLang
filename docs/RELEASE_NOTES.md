# WalkLang Release Notes

## v2.2.0 - Simple Generic Composition

Date: 2026-05-22

v2.2.0 adds experimental simple generic functions as the next composition step after structs and methods.

### Added

- Generic function declarations with type parameters, such as `func: first[T](items array[T]) T`.
- Call-site type inference for generic functions.
- Generic functions over scalar values, arrays, structs, and method-returning struct expressions.
- Exported user-module generic functions.
- Predictable C monomorphization for concrete generic calls.
- v2.2 generic pass/fail fixtures and formatter coverage.
- `docs/V2.md` generic function documentation.

### Changed

- `exp:` may now export generic functions from user modules.
- Formatter output keeps generic and array type brackets tight, such as `array[T]`.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

Structs, methods, and generic functions remain experimental in v2.2. The following remain future roadmap items:

```text
traits
interfaces
generic structs
generic methods
explicit type-argument calls
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

## v2.1.0 - Methods

Date: 2026-05-22

v2.1.0 adds experimental methods on top of the v2 struct surface. Methods are receiver functions, not class-based OOP.

### Added

- Method declarations with receiver syntax, such as `func: User.is_adult(self User) bool`.
- Method calls on struct values, such as `user.is_adult()`.
- Receiver-type namespacing so different structs may use the same method name.
- Type checking for method receivers and ordinary method arguments.
- Generated C lowering that keeps method calls explainable as receiver functions, such as `User__is_adult(user)`.
- v2.1 method pass/fail fixtures and formatter coverage.
- `docs/V2.md` method documentation.

### Changed

- Dotted calls now preserve receiver expressions so struct method calls can be distinguished from imported module calls.

### Breaking Changes

None.

### Removed

None.

### Experimental Or Draft

Structs and methods remain experimental in v2.1. The following remain future roadmap items:

```text
traits
interfaces
generic structs
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

## v2.0.0 - Data Modeling

Date: 2026-05-22

v2.0.0 adds experimental struct-based data modeling. The v1 compatibility contract remains documented separately in `docs/SPEC.md` and `docs/COMPATIBILITY.md`.

### Added

- `struct:` declarations with fixed typed fields.
- Positional struct constructors.
- Dot field reads and mutable field assignment.
- Structs as function parameters and return values.
- Arrays of structs, indexed field access, and mutable array-element fields.
- Module-declared structs returned by exported module functions.
- v2 struct pass/fail fixtures and formatter coverage.
- `docs/V2.md` for the experimental v2 data-modeling surface.

### Changed

- User modules may now contain `struct:` declarations at top level.
- `struct` is now a reserved word.
- Generated C includes typedefs for WalkLang structs and arrays of structs.

### Breaking Changes

- `struct` can no longer be used as a variable, function, field, or expression name.

### Removed

None.

### Experimental Or Draft

Structs are implemented but remain experimental in v2.0. The following remain future roadmap items:

```text
methods
traits
interfaces
generic structs
named-field constructors
file/json/matrix APIs
```

### Upgrade

Run:

```bash
walk check --warnings=error <your entry file>
walk test <your tests file>
walk build <your entry file> -o build/app
```

Rename any user-defined binding named `struct`.

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
