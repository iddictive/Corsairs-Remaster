#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CACHE_DIR="$SCRIPT_DIR/.cache/d3d9"
SOURCE_DIR="$CACHE_DIR/sdl2-source"
BUILD_DIR="$CACHE_DIR/sdl2-build"
INSTALL_DIR="$CACHE_DIR/sdl2-fixed"
PATCH_FILE="$SCRIPT_DIR/sdl2-invisible-cursor.patch"
PROBE_SOURCE="$SCRIPT_DIR/sdl-cursor-probe.cpp"
PROBE_BINARY="$CACHE_DIR/sdl-cursor-probe"

SDL_REPOSITORY="https://github.com/libsdl-org/SDL.git"
SDL_REVISION="5d249570393f7a37e037abf22cd6012a4cc56a71"

CMAKE=/opt/homebrew/bin/cmake
NINJA=/opt/homebrew/bin/ninja
CXX=/usr/bin/clang++

for required in git "$CMAKE" "$NINJA" "$CXX" "$PATCH_FILE" "$PROBE_SOURCE"; do
    if ! command -v "$required" >/dev/null 2>&1 && [[ ! -e "$required" ]]; then
        echo "missing prerequisite: $required" >&2
        exit 1
    fi
done

mkdir -p "$CACHE_DIR"
if [[ ! -d "$SOURCE_DIR/.git" ]]; then
    git clone --no-checkout "$SDL_REPOSITORY" "$SOURCE_DIR"
    git -C "$SOURCE_DIR" checkout --detach "$SDL_REVISION"
fi

actual_revision=$(git -C "$SOURCE_DIR" rev-parse HEAD)
if [[ "$actual_revision" != "$SDL_REVISION" ]]; then
    echo "unexpected SDL source revision: $actual_revision" >&2
    exit 1
fi

if git -C "$SOURCE_DIR" apply --check "$PATCH_FILE" 2>/dev/null; then
    git -C "$SOURCE_DIR" apply "$PATCH_FILE"
elif ! git -C "$SOURCE_DIR" apply --reverse --check "$PATCH_FILE" 2>/dev/null; then
    echo "SDL cursor patch neither applies nor matches the source tree" >&2
    exit 1
fi

unexpected_changes=$(git -C "$SOURCE_DIR" status --short | grep -v '^ M src/video/cocoa/SDL_cocoamouse.m$' || true)
if [[ -n "$unexpected_changes" ]]; then
    echo "unexpected SDL source changes:" >&2
    echo "$unexpected_changes" >&2
    exit 1
fi
git -C "$SOURCE_DIR" diff --check

"$CMAKE" -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST=OFF \
    -DSDL_TESTS=OFF

jobs=$(/usr/sbin/sysctl -n hw.logicalcpu)
"$CMAKE" --build "$BUILD_DIR" --target install -j"$jobs"

SDL_DYLIB="$INSTALL_DIR/lib/libSDL2-2.0.0.dylib"
if [[ ! -f "$SDL_DYLIB" ]]; then
    echo "build did not produce $SDL_DYLIB" >&2
    exit 1
fi

"$CXX" -std=c++17 \
    -I"$INSTALL_DIR/include/SDL2" \
    "$PROBE_SOURCE" \
    -L"$INSTALL_DIR/lib" -lSDL2 \
    -Wl,-rpath,"$INSTALL_DIR/lib" \
    -o "$PROBE_BINARY"

file "$SDL_DYLIB"
shasum -a 256 "$SDL_DYLIB"
echo "SDL2_PREFIX=$INSTALL_DIR"
echo "SDL2_DYLIB=$SDL_DYLIB"
echo "SDL_CURSOR_PROBE=$PROBE_BINARY"
