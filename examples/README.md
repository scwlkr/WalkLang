# WalkLang Examples

This directory contains small programs and project-mode demos that are meant to
run through the current `walk` compiler.

## Single-File Examples

- `hello.walk`: smallest runnable hello-style program.
- `stable.walk`: stable module and standard-library smoke example.
- `compiler_tracer.walk`: compiler tracer-bullet program.
- `compiler_tests.walk`: test-runner example.

## Project Examples

- `tinychain/`: a small blockchain-style ledger project with a module, tests,
  and a README that records the language gaps it exposes.

Run TinyChain from the repository root:

```bash
make walk
cd examples/tinychain
../../build/walk test
../../build/walk run src/main.walk
```
