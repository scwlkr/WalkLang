#!/usr/bin/env sh
set -eu

walk_bin="${WALK_BIN:-$PWD/build/walk}"

mkdir -p "$(dirname "$walk_bin")" docs/reference

go build -o "$walk_bin" ./cmd/walk
"$walk_bin" docs --strict -o docs/reference/api.md examples/v1.walk
"$walk_bin" docs --strict --format json -o docs/reference/api.json examples/v1.walk
go run ./scripts/sitegen.go -docs docs -public public
