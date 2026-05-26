# WalkLang Editor Tooling

This directory contains first-party editor integration scaffolds for the current
WalkLang toolchain.

```text
vscode/
  VS Code extension with syntax highlighting and LSP startup
neovim/
  Neovim filetype, syntax, formatter, and LSP setup notes
```

Both integrations expect an installed `walk` binary and use `walk lsp` for editor diagnostics, formatting, navigation, completion, and rename support.
