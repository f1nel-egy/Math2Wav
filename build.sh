#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/src"
LIBS="$SRC/libs"
BUILD="$ROOT/build"
OUT="$BUILD/math2wav"

CC="${CC:-cc}"
SOURCES=(
    "$SRC/main.c"
    "$LIBS/tinyexpr.c"
    "$LIBS/math2wav.c"
    "$LIBS/ssf.c"
)
INCLUDES=(-I"$SRC" -I"$LIBS")
LIBRARIES=(-lm)
QUIET=(-Wno-array-bounds -Wno-format-truncation)

CMD="${1:-build}"

case "$CMD" in
    clean)
        rm -rf "$BUILD"
        echo "cleaned $BUILD"
        exit 0
        ;;
    debug)
        CFLAGS=(-g -O1 -Wall -Wextra "${QUIET[@]}" -fsanitize=address,undefined)
        ;;
    build)
        CFLAGS=(-O2 -Wall "${QUIET[@]}")
        ;;
    run)
        CFLAGS=(-O2 -Wall "${QUIET[@]}")
        shift || true
        ;;
    -h|--help)
        sed -n '3,13p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
        exit 0
        ;;
    *)
        echo "unknown command: $CMD (try: build, debug, clean, run)" >&2
        exit 1
        ;;
esac

mkdir -p "$BUILD"

echo "  CC/LD   $OUT"
"$CC" "${CFLAGS[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" "${LIBRARIES[@]}" -o "$OUT"
echo "Math2Wav built at: $OUT"

if [ "$CMD" = "run" ]; then
    echo "  RUN     $OUT $*"
    exec "$OUT" "$@"
fi
