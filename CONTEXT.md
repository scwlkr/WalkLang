# WalkLang Context

WalkLang is a small compiled language project. This context records project
language that should stay consistent across docs, roadmap, release, and tooling
work.

## Language

**General Use Language**:
WalkLang's intended role as the primary language for the owner's projects,
from small CLI tools through internal apps, TUIs, native desktop apps,
database-backed tools, games, and project-specific products.
_Avoid_: scripting language, beginner language, toy language

**Personal Foundation Language**:
A language designed to become the base layer for the owner's own software
ecosystem before it tries to serve every outside programming niche.
_Avoid_: niche demo language, learning-only language

**Project Version**:
The single public version number for WalkLang, covering the compiler, tooling,
docs, release artifacts, and implemented language surface.
_Avoid_: language version, compiler version, release version, version layer

**Feature Status**:
A maturity label attached to a feature inside a Project Version.
_Avoid_: stable version, unstable version, experimental version

**Historical Version Reference**:
A past version number kept only when recording release or migration history.
_Avoid_: current language version, current compiler version

**Purpose-Based Developer Path**:
A source, fixture, script, or docs filename named for what it does instead of
the old milestone that first introduced it.
_Avoid_: milestone filename, versioned fixture path

**Stable Feature**:
A feature in the current Project Version that should keep working unless a
future release explicitly changes it.
_Avoid_: stable language contract version

**Draft Feature**:
A feature in the current Project Version that is implemented but may change
before it is treated as stable.
_Avoid_: draft version, unstable version

**Experimental Feature**:
A feature in the current Project Version that is implemented for exploration
and is not yet part of the expected stable user surface.
_Avoid_: experimental language version

## Relationships

- A **Project Version** contains many features.
- Each feature may have one **Feature Status**.
- A **Stable Feature**, **Draft Feature**, and **Experimental Feature** are
  different statuses inside the same **Project Version**, not separate version
  lines.
- A **Historical Version Reference** may appear in release notes or migration
  history, but it should not be used to describe current WalkLang status.
- A **Personal Foundation Language** can still be a **General Use Language**:
  the initial design center is the owner's project ecosystem, not a narrow
  scripting-only or beginner-only audience.
- A **Purpose-Based Developer Path** should be used for normal developer
  workflows; old milestone labels should stay out of active source, fixture,
  script, and current docs paths.

## Example dialogue

> **Dev:** "Is WalkLang using a language version and a compiler version?"
> **Domain expert:** "No. WalkLang is on **Project Version** v5.13.1. Some
> features inside that project version are stable, draft, or experimental."

## Flagged ambiguities

- "version" previously meant release version, stable language contract version,
  and experimental language surface version. Resolved: WalkLang has one
  **Project Version**; feature maturity is described with **Feature Status**.
- Old version numbers are allowed only as **Historical Version References** in
  release notes and migration history.
- "scripting language" and "beginner language" understate the project vision.
  Resolved: WalkLang is intended to become a **General Use Language** and
  **Personal Foundation Language** for the owner's projects.
- Developer-facing paths previously used milestone labels. Resolved: active
  paths use **Purpose-Based Developer Path** names.
