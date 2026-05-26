# WalkLang Professional Tooling

WalkLang includes first-party tooling for normal editor and documentation
workflows without changing the language surface.

## Language Server

Start the language server:

```bash
walk lsp
```

The server speaks JSON-RPC over stdio and supports:

```text
document diagnostics
format document
hover type info
go to definition
find references
completion
rename symbol
```

Diagnostics reuse the compiler parser, module loader, package cache checks, and checker. Open files are checked using their project context when a `walk.toml` is available.

## VS Code

The first-party VS Code extension scaffold lives in:

```text
editors/vscode/
```

It contributes:

```text
.walk file association
TextMate syntax highlighting
editor indentation/comment rules
WalkLang LSP startup through walk lsp
```

Install the extension during local development:

```bash
cd editors/vscode
npm install
npx @vscode/vsce package
code --install-extension walklang-0.1.0.vsix
```

The extension expects `walk` on `PATH`. Override the server binary with:

```json
{
  "walklang.serverPath": "/absolute/path/to/walk"
}
```

## Neovim

Neovim support lives in:

```text
editors/neovim/
```

It includes:

```text
ftdetect/walk.vim
syntax/walk.vim
ftplugin/walk.vim
```

Use the README in that directory for a minimal `vim.lsp.start` setup that runs `walk lsp`.

## Formatter Integration

Editor formatting calls the same formatter as:

```bash
walk fmt <source.walk>
```

Project formatting still works from the project root:

```bash
walk fmt
```

## Project-Wide Check

Inside a project, the editor and command line share the same project search behavior:

```bash
walk check --warnings=error
```

The project check validates the configured entry file, `tests/*_test.walk`, local modules, and locked package dependencies.

## Docs Generator

Generate Markdown API documentation:

```bash
walk docs src/main.walk
walk docs -o docs/api.md src/main.walk
```

Generate a machine-readable docs index:

```bash
walk docs --format json -o docs/api.json src/main.walk
```

Require structured `///` docs for every generated public symbol:

```bash
walk docs --strict -o docs/api.md src/main.walk
walk docs --strict --format json -o docs/api.json src/main.walk
```

Inside a project, omit the source file to document the configured entry:

```bash
walk docs -o docs/api.md
```

The generator includes loaded user modules, top-level structs, functions, and `exp:` exports. By default it remains compatible with signature-only sources. In strict mode, public symbols need structured comments with `Summary`, `Example`, and `Since`; functions with parameters need `Params`, and non-void functions need `Returns`.

## Debugger Foundation

Generate a deterministic source symbol map:

```bash
walk debug-map src/main.walk
walk debug-map -o build/debug-map.json src/main.walk
```

The map records source files, symbol names, symbol kinds, and line/column locations. It is a foundation for debugger adapters and generated-C source mapping in later backend work.

## Non-Goals

Professional tooling does not add:

```text
JetBrains plugin implementation
step-through debugger adapter
generated C source maps
remote package registry integration
new language syntax
```
