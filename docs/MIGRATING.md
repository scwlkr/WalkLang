# WalkLang Migration Guide

## v1.8 to v1.9

v1.9 adds string interpolation for simple display text. Existing v1.8 source should not need changes.

New code can put display expressions inside single-quoted strings:

```walk
imp: string

var: word = 'paddle'
var: wordLength = string.len(word)
out: 'the secret word is {wordLength} characters long'
```

Interpolation accepts `int`, `float`, `bool`, `string`, and nullable string values. Use doubled braces when the output should contain literal braces:

```walk
out: '{{word}}'
```

## v1.7 to v1.8

v1.8 adds stable helpers for simple terminal games and other string/array-heavy programs. Existing v1.7 source should not need changes.

New code can index strings by zero-based byte position:

```walk
out: 'walk'[1]
```

The `string`, `array`, and `random` modules add small helpers:

```walk
imp: string
imp: array
imp: random

var: guessed array[string] = []
var: words = ['dog', 'cat']
guessed = array.push(guessed, 'w')

out: string.contains('walk', 'al')
out: string.concat('walk', 'lang')
out: array.contains(guessed, 'w')
out: random.choice(words)
```

`array.push` returns a new array. Assign it back when the program should keep the appended value.

## v1.6 to v1.7

v1.7 adds the stable `in:` expression for required-line stdin input. Existing v1.6 source should not need changes.

New code can read one line from stdin:

```walk
var: name = in:
```

Use an optional string prompt when interactive programs need one:

```walk
var: name = in: 'Name? '
```

`in:` returns `string`, strips the final line ending, preserves other whitespace, and runtime-stops if stdin reaches EOF before any text is read. It does not add typed input helpers such as `in-int:`.

## v1.5 to v1.6

v1.6 adds local function type inference. Existing v1.5 source should not need changes.

Typed function signatures remain valid:

```walk
func: add(a int, b int) int
    return: + a b
```

New code may omit parameter and return types when the function body proves the types clearly:

```walk
func: power_four(n)
    return: ^ n 4
```

Ambiguous helper functions still need annotations. WalkLang does not infer ordinary function signatures from later call sites:

```walk
func: identity(value int)
    return: value
```

## v2.1 to v2.2

v2.2 adds experimental simple generic functions. Existing v2.1 source should not need changes.

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

## What Changed In v2.2

- Generic function declarations are accepted, such as `func: first[T](items array[T]) T`.
- Generic calls infer type arguments from ordinary call arguments.
- Generic functions can compose with arrays, structs, methods, and user-module exports.
- Generated C specializes concrete generic calls into predictable ordinary functions.

## What Is Still Future

Traits, interfaces, generic structs, generic methods, explicit type-argument calls, and named-field constructors are not part of v2.2.

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
