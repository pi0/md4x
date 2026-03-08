#!/usr/bin/env bash
# Vendor C sources into md4x-sys, publish both crates, then clean up.
# Usage: bash rust/publish.sh [--dry-run] [extra cargo-publish flags]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYS_DIR="$SCRIPT_DIR/md4x-sys"
VENDOR="$SYS_DIR/vendor/md4x"
SRC="$SCRIPT_DIR/../src"

cleanup() {
    rm -rf "$VENDOR"
}
trap cleanup EXIT

echo "==> Vendoring C sources into $VENDOR"
mkdir -p "$VENDOR/renderers"
cp "$SRC/md4x.c"    "$SRC/md4x.h"  \
   "$SRC/entity.c"  "$SRC/entity.h" \
   "$VENDOR/"
cp "$SRC/renderers/"*.c "$SRC/renderers/"*.h "$VENDOR/renderers/"

echo "==> Publishing md4x-sys"
cargo publish -p md4x-sys --locked "$@"

echo "==> Waiting for crates.io index to update..."
for i in $(seq 1 12); do
    sleep 10
    if cargo search md4x-sys --limit 1 2>/dev/null | grep -q "$(cargo metadata --no-deps --manifest-path "$SYS_DIR/Cargo.toml" --format-version 1 | python3 -c 'import sys,json; d=json.load(sys.stdin); print(d["packages"][0]["version"])')"; then
        echo "    md4x-sys is indexed."
        break
    fi
    echo "    Not indexed yet (attempt $i/12)..."
done

echo "==> Publishing md4x"
cargo publish -p md4x --locked "$@"

echo "==> Done."
