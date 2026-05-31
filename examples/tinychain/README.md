# TinyChain

TinyChain is a small WalkLang project that models a toy blockchain-style ledger.
It is designed to show off current language features and expose the next useful
tools WalkLang should grow.

This is not a secure blockchain. It uses a deterministic toy hash and a tiny
proof rule so the whole system stays readable as WalkLang source.

## Run

From this directory:

```bash
../../build/walk run src/main.walk
../../build/walk test
../../build/walk build
./build/tinychain
```

Or from the repository root:

```bash
build/walk run examples/tinychain/src/main.walk
cd examples/tinychain && ../../build/walk test
```

## What It Shows

- `struct:` values for `Transaction` and `Block`.
- Arrays of structs.
- Module exports through `chain.walk`.
- String interpolation and string helpers.
- A deterministic mining loop with `while:`, `for:`, and early `return:`.
- Project mode with `walk.toml`, `src/`, and `tests/`.

## Language Gaps Exposed

- A real `crypto.hash` module would remove the demo's toy hash.
- `array.push` currently supports stable primitive arrays, not arrays of
  structs, so the demo builds a fixed block array after mining.
- Multiline array literals would make larger example data sets easier to read.
- Stable byte arrays would make hash and encoding APIs cleaner than plain
  strings and ints.
- Stable `file` and `json` APIs would let the ledger persist blocks without
  depending on draft modules.
- A stable command-argument API would let the demo accept difficulty or
  transactions from the command line.
