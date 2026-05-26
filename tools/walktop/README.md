# walktop

`walktop` is the first official standalone WalkLang-built tool. It is a compact
CLI system monitor that proves the CLI standard platform slice with arguments,
fixture-mode data, file IO, process calls, parsing, terminal styling, tests,
and native build output.

Run from the repository root after building `walk`:

```bash
walk build --mode release --warnings=error tools/walktop/src/main.walk -o build/walktop
NO_COLOR=1 build/walktop --once --fixture tools/walktop/testdata/basic
build/walktop --once
build/walktop --frames 5
```

Fixture mode reads deterministic command-output files from a directory. Live
mode calls OS commands first and then parses their text output.
