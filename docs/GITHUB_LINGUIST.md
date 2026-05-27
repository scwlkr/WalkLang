# GitHub Linguist Recognition Prep

This repo keeps local prep for a future `github-linguist/linguist` pull request.
Do not submit that pull request until WalkLang has enough public GitHub usage to
meet Linguist's usage bar.

## Current Local Prep

- Official language name: `WalkLang`
- Official extension: `.walk`
- Language type: `programming`
- TextMate scope: `source.walk`
- Brand color: `#000088`
- Local grammar source: `editors/vscode/syntaxes/walk.tmLanguage.json`
- Draft Linguist language entry: `linguist/languages.yml`
- Draft Linguist grammar reference: `linguist/grammars.yml`
- Future PR samples: `samples/WalkLang/*.walk`

## Future Upstream Checklist

When public usage is strong enough:

1. Fork `github-linguist/linguist`.
2. Add the WalkLang entry from `linguist/languages.yml` to
   `lib/linguist/languages.yml`.
3. Add or vendor the TextMate grammar so `grammars.yml` exposes `source.walk`.
4. Copy the representative files from `samples/WalkLang/` into the upstream
   `samples/WalkLang/` directory.
5. Run Linguist's required ID, grammar, sample, and test workflow in that fork.
6. Open the PR with public GitHub search evidence and sample-license notes.

The root `.gitattributes` rule is only a local/future-facing override. GitHub
Linguist will not count unknown languages in repository statistics until the
language exists in Linguist's upstream `languages.yml`.
