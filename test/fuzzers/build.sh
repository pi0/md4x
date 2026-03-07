#!/bin/sh
# Build all fuzzer harnesses with clang + LibFuzzer + sanitizers.
#
# Usage:
#   ./test/fuzzers/build.sh                  # build all fuzzers
#   ./test/fuzzers/build.sh fuzz-mdhtml      # build a single fuzzer
#
# Requirements:
#   - clang with LibFuzzer support
#   - libyaml (for ast/meta fuzzers)
#
# Run a fuzzer:
#   ./fuzz-mdhtml  test/fuzzers/seed-corpus/
#   ./fuzz-mdast   test/fuzzers/seed-corpus/
#   ./fuzz-mdheal  test/fuzzers/seed-corpus/

set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FUZZ_DIR="$ROOT/test/fuzzers"
OUT_DIR="${FUZZ_OUT_DIR:-$ROOT/fuzz-out}"
CC="${CC:-clang}"
SANITIZERS="${SANITIZERS:-fuzzer,address,undefined}"

SRC="$ROOT/src"
RENDERERS="$SRC/renderers"
CFLAGS="-fsanitize=$SANITIZERS -I$SRC -I$RENDERERS -DMD4X_USE_UTF8 -g -O1"

mkdir -p "$OUT_DIR"

build_html() {
    echo "Building fuzz-mdhtml..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdhtml.c" \
        "$SRC/md4x.c" "$SRC/entity.c" \
        "$RENDERERS/md4x-html.c" \
        "$RENDERERS/md4x-heal.c" \
        -lyaml \
        -o "$OUT_DIR/fuzz-mdhtml"
}

build_ast() {
    echo "Building fuzz-mdast..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdast.c" \
        "$SRC/md4x.c" "$SRC/entity.c" \
        "$RENDERERS/md4x-ast.c" \
        "$RENDERERS/md4x-heal.c" \
        -lyaml \
        -o "$OUT_DIR/fuzz-mdast"
}

build_ansi() {
    echo "Building fuzz-mdansi..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdansi.c" \
        "$SRC/md4x.c" "$SRC/entity.c" \
        "$RENDERERS/md4x-ansi.c" \
        "$RENDERERS/md4x-heal.c" \
        -o "$OUT_DIR/fuzz-mdansi"
}

build_text() {
    echo "Building fuzz-mdtext..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdtext.c" \
        "$SRC/md4x.c" "$SRC/entity.c" \
        "$RENDERERS/md4x-text.c" \
        "$RENDERERS/md4x-heal.c" \
        -o "$OUT_DIR/fuzz-mdtext"
}

build_meta() {
    echo "Building fuzz-mdmeta..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdmeta.c" \
        "$SRC/md4x.c" "$SRC/entity.c" \
        "$RENDERERS/md4x-meta.c" \
        "$RENDERERS/md4x-heal.c" \
        -lyaml \
        -o "$OUT_DIR/fuzz-mdmeta"
}

build_heal() {
    echo "Building fuzz-mdheal..."
    $CC $CFLAGS \
        "$FUZZ_DIR/fuzz-mdheal.c" \
        "$RENDERERS/md4x-heal.c" \
        -o "$OUT_DIR/fuzz-mdheal"
}

if [ $# -eq 0 ]; then
    build_html
    build_ast
    build_ansi
    build_text
    build_meta
    build_heal
    echo "All fuzzers built in $OUT_DIR/"
else
    for target in "$@"; do
        case "$target" in
            fuzz-mdhtml|html)  build_html ;;
            fuzz-mdast|ast)    build_ast ;;
            fuzz-mdansi|ansi)  build_ansi ;;
            fuzz-mdtext|text)  build_text ;;
            fuzz-mdmeta|meta)  build_meta ;;
            fuzz-mdheal|heal)  build_heal ;;
            *) echo "Unknown target: $target" >&2; exit 1 ;;
        esac
    done
fi
