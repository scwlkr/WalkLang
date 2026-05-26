#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
install_dir="${WALK_INSTALL_DIR:-"$HOME/.local/bin"}"
version="${WALK_VERSION:-${1:-local}}"
binary="$install_dir/walk"
tool_binary="$install_dir/walktop"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/walklang-install.XXXXXX")"

cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

assert_no_removed_port_sources() {
    if ! command -v git >/dev/null 2>&1; then
        return
    fi
    if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        return
    fi
    if [ "$(git ls-files '*.go' 'go.mod' 'go.sum' '*.js')" != "" ]; then
        echo "install blocked: Go or JavaScript source is tracked in this port" >&2
        exit 1
    fi
}

mkdir -p "$install_dir"
install_root="$(CDPATH= cd -- "$install_dir/.." && pwd)"
runtime_install_dir="${WALK_RUNTIME_INSTALL_DIR:-"$install_root/lib/walk/runtime"}"

cd "$repo_root"
assert_no_removed_port_sources
make -s walk WALK_VERSION="$version"
cp build/walk "$binary"
chmod +x "$binary"
echo "$binary"
"$binary" version

mkdir -p "$runtime_install_dir/platform"
cp runtime/walk_runtime.h "$runtime_install_dir/walk_runtime.h"
cp runtime/walk_runtime.c "$runtime_install_dir/walk_runtime.c"
cp runtime/platform/walk_platform.h "$runtime_install_dir/platform/walk_platform.h"
cp runtime/platform/walk_platform_posix.c "$runtime_install_dir/platform/walk_platform_posix.c"
cp runtime/platform/walk_platform_windows.c "$runtime_install_dir/platform/walk_platform_windows.c"
echo "$runtime_install_dir"

build_driver="${WALK_BUILD_BIN:-$binary}"
WALK_RUNTIME_DIR="$runtime_install_dir" "$build_driver" build --mode release --warnings=error tools/walktop/src/main.walk -o "$work_dir/walktop" >/dev/null
cp "$work_dir/walktop" "$tool_binary"
chmod +x "$tool_binary"
NO_COLOR=1 "$tool_binary" --once --fixture "$repo_root/tools/walktop/testdata/basic" >/dev/null
echo "$tool_binary"

case ":$PATH:" in
    *":$install_dir:"*) ;;
    *)
        echo "warning: $install_dir is not on PATH"
        echo "add this to your shell profile:"
        echo "export PATH=\"$install_dir:\$PATH\""
        ;;
esac
