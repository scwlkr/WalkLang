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

mkdir -p "$install_dir"

cd "$repo_root"
go build -trimpath -ldflags "-X main.version=$version" -o "$binary" ./cmd/walk

echo "$binary"
"$binary" version

"$binary" build --mode release --warnings=error tools/walktop/src/main.walk -o "$work_dir/walktop" >/dev/null
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
