#!/usr/bin/env sh
set -eu

if [ "${1:-}" = "" ]; then
    echo "usage: scripts/release.sh <version> [out-dir]" >&2
    exit 2
fi

version="$1"
out_dir="${2:-dist}"

mkdir -p "$out_dir"
checksums="$out_dir/SHA256SUMS"
: > "$checksums"

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
    path="$out_dir/$name"
    echo "$path"
    GOOS="$goos" GOARCH="$goarch" go build -trimpath -ldflags "-s -w -X main.version=${version}" -o "$out_dir/$name" ./cmd/walk
    if command -v sha256sum >/dev/null 2>&1; then
        (cd "$out_dir" && sha256sum "$name" >> SHA256SUMS)
    else
        (cd "$out_dir" && shasum -a 256 "$name" >> SHA256SUMS)
    fi
done

echo "$checksums"
