# WalkLang Public Docs And Reference Site

WalkLang documentation is publishable as a static site without changing the
language surface.

## Site Surface

The static site builds into:

```text
public/
```

The hosted path is configured for:

```text
walklang.wlkrlabs.com/docs
```

The generated site includes:

```text
public/index.html
public/docs/index.html
public/docs/reference/api.html
public/docs/reference/api.md
public/docs/reference/api.json
```

`public/CNAME` sets the custom domain to `walklang.wlkrlabs.com` for static
hosts that honor CNAME files.

HTTPS enforcement depends on GitHub Pages issuing a certificate for the custom
domain. `docs/STATUS.md` records the current live HTTPS state.

## Reference Generation

The reference source remains real WalkLang code with structured comments:

```text
examples/stable.walk
examples/math_extra.walk
```

Build the site and generated reference artifacts with:

```bash
scripts/build-docs-site.sh
```

Check that generated artifacts are fresh and that static links resolve with:

```bash
scripts/check-docs-site.sh
```

## Public Path Safety

Generated reference docs use repository-relative source paths such as:

```text
examples/stable.walk
examples/math_extra.walk
```

Do not publish generated docs that contain local absolute paths.

## CI And Deploy

CI runs the docs-site check before building release artifacts. The Pages workflow
builds `public/` and uploads it as a GitHub Pages artifact from `main`.

## Non-Goals

The public docs and reference site do not add:

```text
networking
interactive playground
compiler explorer
online package index
new language syntax
```
