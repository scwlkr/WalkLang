#!/usr/bin/env sh
set -eu

if [ "${1:-}" = "" ]; then
    echo "usage: scripts/release.sh <version> [out-dir]" >&2
    exit 2
fi

version="$1"
out_dir="${2:-dist}"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/walklang-release.XXXXXX")"

cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

mkdir -p "$out_dir"
checksums="$out_dir/SHA256SUMS"
: > "$checksums"

add_checksum() {
    name="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        (cd "$out_dir" && sha256sum "$name" >> SHA256SUMS)
    else
        (cd "$out_dir" && shasum -a 256 "$name" >> SHA256SUMS)
    fi
}

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
    add_checksum "$name"
done

runtime_name="walk-runtime-${version}.tar.gz"
tar -czf "$out_dir/$runtime_name" runtime
echo "$out_dir/$runtime_name"
add_checksum "$runtime_name"

host_goos="$(go env GOOS)"
host_goarch="$(go env GOARCH)"
host_ext=""
if [ "$host_goos" = "windows" ]; then
    host_ext=".exe"
fi

walk_cpp_name="walk-cpp-${version}-${host_goos}-${host_goarch}${host_ext}"
walk_cpp_path="$out_dir/$walk_cpp_name"
make -s walk WALK_VERSION="${version}"
cp build/walk-cpp "$walk_cpp_path"
chmod +x "$walk_cpp_path"
echo "$walk_cpp_path"
add_checksum "$walk_cpp_name"

walktop_name="walktop-${version}-${host_goos}-${host_goarch}${host_ext}"
walktop_path="$out_dir/$walktop_name"

go build -trimpath -ldflags "-X main.version=${version}" -o "$work_dir/walk" ./cmd/walk
build_driver="${WALK_RELEASE_BUILD_BIN:-$work_dir/walk}"
"$build_driver" build --mode release --warnings=error tools/walktop/src/main.walk -o "$work_dir/walktop${host_ext}" >/dev/null
cp "$work_dir/walktop${host_ext}" "$walktop_path"
echo "$walktop_path"
add_checksum "$walktop_name"

echo "$checksums"
