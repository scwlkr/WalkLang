# WalkLang Neovim

Copy or symlink this directory's `ftdetect/`, `syntax/`, and `ftplugin/` files into a Neovim runtime path.

Minimal LSP setup:

```lua
vim.api.nvim_create_autocmd("FileType", {
  pattern = "walk",
  callback = function()
    vim.lsp.start({
      name = "walklang",
      cmd = { "walk", "lsp" },
      root_dir = vim.fs.root(0, { "walk.toml", ".git" }) or vim.fn.getcwd(),
    })
  end,
})
```

The filetype plugin sets four-space indentation and adds a buffer-local `:WalkFmt` command when `walk` is executable.
