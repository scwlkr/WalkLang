#!/usr/bin/env sh
set -eu

version="${1:-v1}"
out_dir="${2:-dist}"

mkdir -p "$out_dir"

targets="
darwin/arm64
darwin/amd64
linux/amd64
linux/arm64
windows/amd64
"

for target in $targets; do
    goos="${target%/*}"
    goarch="${target#*/}"
    name="walk-${version}-${goos}-${goarch}"
    if [ "$goos" = "windows" ]; then
        name="${name}.exe"
    fi
    echo "$out_dir/$name"
    GOOS="$goos" GOARCH="$goarch" go build -trimpath -ldflags "-s -w -X main.version=${version}" -o "$out_dir/$name" ./cmd/walk
done

