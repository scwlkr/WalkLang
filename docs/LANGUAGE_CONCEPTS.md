# WalkLang Language Concepts And Documentation Standard

This document defines the words WalkLang docs must use when they describe the
language. It also defines why language docs exist and how new docs are made.

Rule:

```text
If a language doc needs a new concept word, define it here first.
If a doc claims behavior, the same change must include proof.
If there is no proof, the doc must say future, planned, experimental, or draft.
```

`docs/SPEC.md` remains the stable language contract. This document is the
terminology and documentation standard that keeps the rest of the docs coherent.

---

## Purpose

WalkLang docs have three jobs:

1. Define the language surface with exact terms.
2. Teach that surface without changing its meaning.
3. Prove the claimed behavior with tests, fixtures, generated output, or native
   execution.

Docs are not marketing copy and not loose notes. A doc either defines a
language rule, explains how to use a rule, records compatibility, or records a
future direction that is clearly labeled as not implemented.

---

## Stability Words

Use these words exactly.

```text
stable
  Part of the v1 compatibility promise. Stable behavior must be in the stable
  docs and must have compatibility proof.

draft
  Implemented or intentionally shaped, but not compatibility-protected yet.
  Draft behavior must be labeled draft wherever it appears.

experimental
  Implemented outside the stable v1 contract. It may change or be removed.

planned
  Designed or intended, but not implemented. Planned behavior must not be
  presented as accepted syntax or an available API.

deprecated
  Still accepted, but has a documented replacement and migration path.

removed
  No longer accepted by the current compiler.
```

Do not use soft substitutes such as "basically supported", "kind of stable",
"mostly works", or "probably available".

---

## Firm Language Concepts

Use these terms consistently across all language docs.

### Source File

A source file is UTF-8 text ending in `.walk`. A source file contains zero or
more statements. Blank lines are ignored. Comments are not statements.

### Line

A line is one physical source line. WalkLang statement structure is line-based;
semicolons are not a statement separator.

### Token

A token is the smallest lexical unit the lexer emits. Current token classes are
name, number, string, and symbol. Comments and whitespace separate or remove
tokens; they are not AST nodes.

### Keyword

A keyword is a reserved word with grammar meaning. Keywords cannot be used as
user-defined names. Examples include `var`, `const`, `func`, `return`, `if`,
`else`, `for`, `in`, `test`, and `assert`.

### Identifier Or Name

An identifier is a user-written symbolic name. WalkLang implementation and
existing docs often say name. Public docs may say identifier when discussing
lexical structure, but the concept is the same unless a doc explicitly says
otherwise.

Identifier rules:

```text
letters, digits, and _
must not be a reserved keyword
must not start with a digit
```

### Literal

A literal is source syntax that directly creates a value. Stable literals
include int, float, bool, string, and `null`.

### Value

A value is runtime data produced by evaluating an expression. Every value has a
type. Examples: `1` is an `int` value, `'walk'` is a `string` value, and
`[1, 2]` is an `array[int]` value.

### Type

A type is the compile-time category assigned to a value or expression. Stable
types include:

```text
int
float
bool
string
string?
array[T]
func(T...) R
```

`void` is an internal function return marker, not a value type. Struct and
generic types exist in the experimental v2 surface unless promoted by a later
stable contract change.

### Expression

An expression is syntax that the checker assigns a type to. Most expressions
produce values. Effect calls are typed too, but effect calls may only be used
where the language allows them, such as under the draft `do:` statement.

Examples:

```text
literal
name
in:
prefix operator expression
function call
module call
array literal
index expression
field access
grouped expression
```

### Statement

A statement is one complete line-level action in a program. Statements drive
program structure and control flow. Examples include `var:`, `const:`, `out:`,
`func:`, `return:`, `if:`, `while:`, `for:`, `test:`, and assignment.

### Declaration

A declaration is a statement that introduces or exposes a named surface.
WalkLang declarations include:

```text
var:
const:
func:
struct:
imp:
exp:
test:
```

Not every declaration has the same lifetime. `imp:` introduces an imported
module namespace. `exp:` exposes an existing module symbol. `test:` introduces a
test case for `walk test`.

### Block

A block is an indented list of child statements owned by a parent statement.
Indentation is syntax. Tabs are invalid indentation.

### Scope

A scope is a compile-time name-resolution region. A scope is not a runtime
object. Function parameters and block-local declarations live in scopes.
Nested scopes may read outer names, and shadowing warnings are part of the
diagnostic surface.

### Function

A function is a named reusable block declared with `func:`. Stable functions may
have typed parameters and a typed return. Obvious local function types may be
inferred only from the function body; later call sites do not determine a
function signature.

### Module

A module is an imported namespace. Built-in modules are compiler-provided.
User modules are `.walk` files imported with `imp:`. Package modules are resolved
through project/package rules.

Docs must distinguish:

```text
stable built-in module
draft built-in module
user module
package module
```

### Collection

A collection is a value that contains other values. Stable v1 collections are
arrays. Struct values are experimental data-modeling values in v2. Maps, sets,
and tuples are not current WalkLang concepts.

### Error

Use diagnostic for compiler-reported errors and warnings. Use runtime failure
for native program failures after compilation. Do not use error loosely when the
doc means warning, failed assertion, recoverable result data, or runtime stop.

### Recoverable Result Data

Recoverable result data is an ordinary runtime value that reports an ordinary
documented failure without stopping the native program. It is not a compiler
diagnostic, not a failed assertion, and not a runtime failure.

The current draft result shape is concrete per API:

```text
ok bool
value T
error string
```

`ok` is true only when `value` is meaningful. `error` is `''` on success and a
short machine-testable code on ordinary recoverable failure. Draft APIs may use
module-specific fields when the successful value needs a clearer name, such as
`HttpResult.body` or `ProcessResult.stdout`.

Allocation failure, invalid compiler state, and unrecoverable backend/runtime
failures may still runtime-stop. Do not describe recoverable result data as
exceptions, thrown errors, or hidden control flow.

### Standard Library API

A standard library API is a built-in module function or built-in result type
documented in `docs/STDLIB.md`. A standard library API must name its module,
function, parameter types, return type, effect status, stability, runtime
behavior, and failure behavior.

---

## Required Spec Spine

`docs/SPEC.md` should stay concise and link outward for detail. Its stable
language spine should use this order unless a future docs-standard update
changes it:

1. Lexical structure
2. Types
3. Values
4. Expressions
5. Statements
6. Declarations
7. Functions
8. Modules
9. Errors
10. Standard library

Detailed examples belong in focused docs such as `docs/SYNTAX.md`,
`docs/STDLIB.md`, `docs/ERRORS.md`, `docs/COMPATIBILITY.md`, and version or
topic pages.

---

## Documentation Roles

Each doc must have one primary role.

```text
SPEC.md
  Stable contract. Small, normative, link-heavy.

SYNTAX.md
  Human-readable syntax guide. Examples are stable unless labeled otherwise.

STDLIB.md
  Built-in modules, functions, result structs, effects, stability, and failure
  behavior.

ERRORS.md
  Diagnostic shape, categories, warnings, and runtime failures.

COMPATIBILITY.md
  Compatibility promise and the proof surface for stable behavior.

MIGRATING.md
  User-facing upgrade steps between versions.

DEPRECATION.md
  Deprecated and removed behavior lifecycle.

DESIGN_RULES.md
  Rules for accepting or rejecting new language design.

ROADMAP.md and IO_PLAN.md
  Planned or phased future direction. They must not present planned behavior as
  current behavior.

STATUS.md
  Current project state, verification, and next step.

Generated reference docs
  Output from source comments and real compiler metadata, not hand-written API
  guesses.
```

---

## TDD-Synchronous Documentation Rule

Language docs and implementation move together.

For every new or changed language behavior, the same work slice must include:

1. The smallest contract or draft-doc update that names the behavior.
2. A positive proof that the behavior works through the public compiler or CLI.
3. A negative proof for invalid forms when invalid forms matter.
4. Generated C snapshot coverage when backend output matters.
5. Formatter coverage when syntax shape changes.
6. Status/release notes when the change affects the current version.

Do not write a broad docs pass first and defer tests. Do not implement a
language feature first and leave docs as follow-up. The unit of work is the
behavior slice: doc, test, implementation, verification.

---

## Proof Matrix

Use this matrix when adding or changing docs.

```text
Lexical structure
  Proof: lexer/parser test or fail fixture for invalid tokens.

Types and values
  Proof: checker pass/fail fixtures and native run when runtime representation
  matters.

Expressions
  Proof: pass fixture or focused compiler test; fail fixture for invalid
  operand/type/placement rules.

Statements and declarations
  Proof: parser/checker test, pass fixture, fail fixture for invalid placement,
  and formatter test when spacing or indentation changes.

Blocks and scopes
  Proof: nested pass fixture plus shadowing, visibility, or invalid indentation
  diagnostics when relevant.

Functions
  Proof: call, return, inference, and missing-return tests through real compile
  or run paths.

Modules
  Proof: import/export pass fixture and private/missing/cycle diagnostics when
  relevant.

Collections
  Proof: construction, indexing/access, mutation rules, homogeneous typing, and
  runtime execution when emitted code matters.

Diagnostics and runtime failures
  Proof: asserted first diagnostic line or native executable failure output.

Standard library APIs
  Proof: built-in registry entry, stdlib docs, positive native runtime test,
  negative argument/effect diagnostics, and generated C/runtime coverage when
  the backend changes.

Generated reference docs
  Proof: `walk docs --strict`, JSON output when required, and docs-site check.
```

---

## Change Checklist

Before merging a language-doc change:

```text
1. Uses only defined terminology from this document.
2. Marks each feature stable, draft, experimental, planned, deprecated, or
   removed.
3. Links details instead of duplicating long explanations.
4. Includes proof in tests, fixtures, snapshots, generated docs, or native run
   output.
5. Updates STATUS.md when the project state changes.
6. Rebuilds/checks the docs site when docs or site pages change.
```

If any item is missing, the doc must say exactly what is unproven or future.
