#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
install_dir="${WALK_INSTALL_DIR:-"$HOME/.local/bin"}"
version="${WALK_VERSION:-${1:-local}}"
binary="$install_dir/walk"

mkdir -p "$install_dir"

cd "$repo_root"
go build -trimpath -ldflags "-X main.version=$version" -o "$binary" ./cmd/walk

echo "$binary"
"$binary" version

case ":$PATH:" in
    *":$install_dir:"*) ;;
    *)
        echo "warning: $install_dir is not on PATH"
        echo "add this to your shell profile:"
        echo "export PATH=\"$install_dir:\$PATH\""
        ;;
esac

