#!/usr/bin/env sh
set -eu

walk_bin="${WALK_BIN:-$PWD/build/walk}"

mkdir -p "$(dirname "$walk_bin")" docs/reference

if [ ! -x "$walk_bin" ]; then
    make -s walk WALK_VERSION="${WALK_VERSION:-dev}"
fi
"$walk_bin" docs --strict -o docs/reference/api.md examples/stable.walk
"$walk_bin" docs --strict --format json -o docs/reference/api.json examples/stable.walk
"$walk_bin" sitegen -docs docs -public public
