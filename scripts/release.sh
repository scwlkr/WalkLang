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

assert_no_removed_port_sources() {
    if ! command -v git >/dev/null 2>&1; then
        return
    fi
    if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        return
    fi
    if [ "$(git ls-files '*.go' 'go.mod' 'go.sum' '*.js')" != "" ]; then
        echo "release blocked: Go or JavaScript source is tracked in this port" >&2
        exit 1
    fi
}

add_checksum() {
    name="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        (cd "$out_dir" && sha256sum "$name" >> SHA256SUMS)
    else
        (cd "$out_dir" && shasum -a 256 "$name" >> SHA256SUMS)
    fi
}

host_os="$(uname -s | tr '[:upper:]' '[:lower:]')"
case "$host_os" in
    darwin|linux)
        ;;
    msys*|mingw*|cygwin*)
        host_os="windows"
        ;;
    *)
        echo "unsupported release host OS: $host_os" >&2
        exit 1
        ;;
esac

host_arch="$(uname -m)"
case "$host_arch" in
    x86_64|amd64)
        host_arch="amd64"
        ;;
    arm64|aarch64)
        host_arch="arm64"
        ;;
    *)
        echo "unsupported release host architecture: $host_arch" >&2
        exit 1
        ;;
esac

host_ext=""
if [ "$host_os" = "windows" ]; then
    host_ext=".exe"
fi

assert_no_removed_port_sources
make -s walk WALK_VERSION="${version}"

walk_name="walk-${version}-${host_os}-${host_arch}${host_ext}"
walk_path="$out_dir/$walk_name"
cp build/walk "$walk_path"
chmod +x "$walk_path"
echo "$walk_path"
add_checksum "$walk_name"

runtime_name="walk-runtime-${version}.tar.gz"
tar -czf "$out_dir/$runtime_name" runtime
echo "$out_dir/$runtime_name"
add_checksum "$runtime_name"

walktop_name="walktop-${version}-${host_os}-${host_arch}${host_ext}"
walktop_path="$out_dir/$walktop_name"

build_driver="${WALK_RELEASE_BUILD_BIN:-build/walk}"
WALK_RUNTIME_DIR="$PWD/runtime" "$build_driver" build --mode release --warnings=error tools/walktop/src/main.walk -o "$work_dir/walktop${host_ext}" >/dev/null
cp "$work_dir/walktop${host_ext}" "$walktop_path"
echo "$walktop_path"
add_checksum "$walktop_name"

echo "$checksums"
