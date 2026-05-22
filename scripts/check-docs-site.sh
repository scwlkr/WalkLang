#!/usr/bin/env sh
set -eu

scripts/build-docs-site.sh
git diff --exit-code -- docs/reference public
