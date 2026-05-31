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

For the full color terminal showcase, run it in a real terminal or force ANSI
color for a scripted preview. `NO_COLOR` intentionally disables color, so unset
it when you want a forced-color capture:

```bash
env -u NO_COLOR CLICOLOR_FORCE=1 ../../build/walk run src/main.walk
```

Or from the repository root:

```bash
build/walk run examples/tinychain/src/main.walk
cd examples/tinychain && ../../build/walk test
```

Expected shape:

```text
+------------------------------------------------------------+
| TINYCHAIN                                                  |
| A tiny blockchain mined by WalkLang                        |
+------------------------------------------------------------+
network: local demo net
blocks mined: 2
difficulty: hash must satisfy math.remainder(hash, 11) == 0
...
== chain audit ==
[pass] linked chain valid: true
[tamper] rewrite block #1 previous_hash -> 123
[audit] tampered chain valid: false
```

## What It Shows

- `struct:` values for `Transaction` and `Block`.
- Arrays of structs.
- Module exports through `chain.walk`.
- String interpolation and string helpers.
- A deterministic mining loop with `while:`, `for:`, and early `return:`.
- Draft terminal styling through `term.color` and `term.style`, with clean
  redirected output.
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
