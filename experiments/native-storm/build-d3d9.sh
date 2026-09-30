#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CACHE_DIR="$SCRIPT_DIR/.cache/d3d9"
SOURCE_DIR="$CACHE_DIR/warpowers-dxvk"
HEADER_SOURCE_DIR="$CACHE_DIR/dxvk-native"
BUILD_DIR="$CACHE_DIR/warpowers-build"
SDL_PREFIX="$CACHE_DIR/sdl2-fixed"
SDK_DIR="$CACHE_DIR/sdk"
GLSLANG_DIR="$CACHE_DIR/sdk/glslang/bin"
MOLTENVK_DIR="$SDK_DIR/MoltenVK"
MOLTENVK_DYLIB="$MOLTENVK_DIR/MoltenVK/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
PROBE_SOURCE="$SCRIPT_DIR/d3d9-probe.cpp"
PROBE_BINARY="$CACHE_DIR/d3d9-probe"
BUILD_STAMP="$BUILD_DIR/.native-storm-inputs"

DXVK_REPOSITORY="https://github.com/abhishekpradhan/warpowers-dxvk.git"
HEADER_REPOSITORY="https://github.com/Joshua-Ashton/dxvk-native.git"
EXPECTED_DXVK_REV="e73e9f276d1bf42a6362f04490b447ceeb7eefef"
EXPECTED_HEADER_REV="a2dc99c407340432d4ba5bfa29efa685c27942ea"

MOLTENVK_ARCHIVE="$CACHE_DIR/MoltenVK-macos-v1.4.2.tar"
MOLTENVK_URL="https://github.com/KhronosGroup/MoltenVK/releases/download/v1.4.2/MoltenVK-macos.tar"
MOLTENVK_SHA256="f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e"
MOLTENVK_DYLIB_SHA256="aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f"
GLSLANG_ARCHIVE="$CACHE_DIR/glslang-16.6.0-macos-universal-release.tar.gz"
GLSLANG_URL="https://github.com/KhronosGroup/glslang/releases/download/16.6.0/glslang-16.6.0-macos-universal-release.tar.gz"
GLSLANG_SHA256="4f8c05f2888a73ed0b7b274bc524af9130e09ae94b43c0f3f9ed83cfae852834"
GLSLANG_BINARY_SHA256="18c23a68d6f4a21e822a8dcee212a2a448a66494b8f58aaf1655ef6328543d11"

MESON=/opt/homebrew/bin/meson
NINJA=/opt/homebrew/bin/ninja
PKG_CONFIG=/opt/homebrew/bin/pkg-config
CXX=/usr/bin/clang++

for required in git curl tar shasum "$MESON" "$NINJA" "$PKG_CONFIG" "$CXX" \
                "$SCRIPT_DIR/build-sdl2.sh" "$PROBE_SOURCE"; do
  if ! command -v "$required" >/dev/null 2>&1 && [[ ! -e "$required" ]]; then
      echo "missing prerequisite: $required" >&2
      exit 1
  fi
done

mkdir -p "$CACHE_DIR" "$SDK_DIR"

clone_pinned() {
  local repository=$1
  local directory=$2
  local revision=$3
  if [[ ! -d "$directory/.git" ]]; then
    git clone --no-checkout "$repository" "$directory"
    git -C "$directory" checkout --detach "$revision"
  fi
  local actual
  actual=$(git -C "$directory" rev-parse HEAD)
  if [[ "$actual" != "$revision" ]]; then
    echo "unexpected source revision in $directory: $actual" >&2
    exit 1
  fi
}

download_verified() {
  local url=$1
  local destination=$2
  local expected=$3
  if [[ -f "$destination" ]]; then
    local actual
    actual=$(shasum -a 256 "$destination" | awk '{print $1}')
    if [[ "$actual" == "$expected" ]]; then
      return
    fi
    echo "checksum mismatch for existing $destination: $actual" >&2
    exit 1
  fi
  curl --fail --location --retry 3 --output "$destination.part" "$url"
  local actual
  actual=$(shasum -a 256 "$destination.part" | awk '{print $1}')
  if [[ "$actual" != "$expected" ]]; then
    echo "checksum mismatch for downloaded $destination: $actual" >&2
    exit 1
  fi
  mv "$destination.part" "$destination"
}

clone_pinned "$DXVK_REPOSITORY" "$SOURCE_DIR" "$EXPECTED_DXVK_REV"
clone_pinned "$HEADER_REPOSITORY" "$HEADER_SOURCE_DIR" "$EXPECTED_HEADER_REV"
git -C "$SOURCE_DIR" submodule update --init --recursive
for directory in "$SOURCE_DIR" "$HEADER_SOURCE_DIR"; do
  source_changes=$(git -C "$directory" status --short --untracked-files=no)
  if [[ -n "$source_changes" ]]; then
    echo "unexpected tracked source changes in $directory:" >&2
    echo "$source_changes" >&2
    exit 1
  fi
done

download_verified "$MOLTENVK_URL" "$MOLTENVK_ARCHIVE" "$MOLTENVK_SHA256"
download_verified "$GLSLANG_URL" "$GLSLANG_ARCHIVE" "$GLSLANG_SHA256"

if [[ ! -f "$MOLTENVK_DYLIB" ]]; then
  mkdir -p "$MOLTENVK_DIR"
  tar -xf "$MOLTENVK_ARCHIVE" -C "$MOLTENVK_DIR"
fi
if [[ ! -x "$GLSLANG_DIR/glslang" ]]; then
  mkdir -p "$SDK_DIR/glslang"
  tar -xzf "$GLSLANG_ARCHIVE" -C "$SDK_DIR/glslang"
fi

actual_moltenvk=$(shasum -a 256 "$MOLTENVK_DYLIB" | awk '{print $1}')
if [[ "$actual_moltenvk" != "$MOLTENVK_DYLIB_SHA256" ]]; then
  echo "checksum mismatch for extracted MoltenVK: $actual_moltenvk" >&2
  exit 1
fi
actual_glslang=$(shasum -a 256 "$GLSLANG_DIR/glslang" | awk '{print $1}')
if [[ "$actual_glslang" != "$GLSLANG_BINARY_SHA256" ]]; then
  echo "checksum mismatch for extracted glslang: $actual_glslang" >&2
  exit 1
fi
/usr/bin/lipo -verify_arch arm64 "$MOLTENVK_DYLIB"

"$SCRIPT_DIR/build-sdl2.sh"

actual_rev=$(git -C "$SOURCE_DIR" rev-parse HEAD)
if [[ "$actual_rev" != "$EXPECTED_DXVK_REV" ]]; then
  echo "unexpected DXVK source revision: $actual_rev" >&2
  exit 1
fi

header_rev=$(git -C "$HEADER_SOURCE_DIR" rev-parse HEAD)
if [[ "$header_rev" != "$EXPECTED_HEADER_REV" ]]; then
  echo "unexpected D3D9 header revision: $header_rev" >&2
  exit 1
fi

export PATH="$GLSLANG_DIR:/opt/homebrew/bin:/usr/bin:/bin"
export PKG_CONFIG_PATH="$SDL_PREFIX/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig"

meson_args=(
  --buildtype=release
  -Denable_d3d9=true
  -Denable_d3d8=false
  -Denable_d3d10=false
  -Denable_d3d11=false
  -Denable_dxgi=false
  -Dnative_sdl2=enabled
  -Dnative_sdl3=disabled
  -Dnative_glfw=disabled
)

desired_stamp="$EXPECTED_DXVK_REV SDL2=$(shasum -a 256 "$SDL_PREFIX/lib/libSDL2-2.0.0.dylib" | awk '{print $1}')"
if [[ -f "$BUILD_DIR/build.ninja" && (! -f "$BUILD_STAMP" || "$(cat "$BUILD_STAMP")" != "$desired_stamp") ]]; then
  "$MESON" setup --wipe "$BUILD_DIR" "$SOURCE_DIR" "${meson_args[@]}"
elif [[ -f "$BUILD_DIR/build.ninja" ]]; then
  "$MESON" setup --reconfigure "$BUILD_DIR" "$SOURCE_DIR" "${meson_args[@]}"
else
  "$MESON" setup "$BUILD_DIR" "$SOURCE_DIR" "${meson_args[@]}"
fi

jobs=$(/usr/sbin/sysctl -n hw.logicalcpu)
"$NINJA" -C "$BUILD_DIR" -j"$jobs"
printf '%s\n' "$desired_stamp" > "$BUILD_STAMP"

DXVK_D3D9="$BUILD_DIR/src/d3d9/libdxvk_d3d9.0.dylib"
if [[ ! -f "$DXVK_D3D9" ]]; then
  echo "build did not produce $DXVK_D3D9" >&2
  exit 1
fi

"$CXX" -std=c++17 -DDXVK_NATIVE -DDXVK_WSI_SDL2 \
  -I"$HEADER_SOURCE_DIR/include" \
  -I"$HEADER_SOURCE_DIR/include/native" \
  -I"$HEADER_SOURCE_DIR/include/native/windows" \
  -I"$HEADER_SOURCE_DIR/include/native/directx" \
  -I"$SDL_PREFIX/include/SDL2" \
  "$PROBE_SOURCE" \
  -L"$BUILD_DIR/src/d3d9" -ldxvk_d3d9 \
  -L"$SDL_PREFIX/lib" -lSDL2 \
  -Wl,-rpath,"$BUILD_DIR/src/d3d9" \
  -Wl,-rpath,"$SDL_PREFIX/lib" \
  -o "$PROBE_BINARY"

file "$DXVK_D3D9"
nm -gU "$DXVK_D3D9" | grep -E 'Direct3DCreate9(Ex)?$'
shasum -a 256 "$DXVK_D3D9"

DYLD_LIBRARY_PATH="$SDL_PREFIX/lib:$BUILD_DIR/src/d3d9:$(dirname "$MOLTENVK_DYLIB")" \
DXVK_WSI_DRIVER=SDL2 DXVK_LOG_LEVEL=warn MVK_CONFIG_LOG_LEVEL=1 \
  "$CACHE_DIR/sdl-cursor-probe" relative

DYLD_LIBRARY_PATH="$SDL_PREFIX/lib:$BUILD_DIR/src/d3d9:$(dirname "$MOLTENVK_DYLIB")" \
DXVK_WSI_DRIVER=SDL2 DXVK_LOG_LEVEL=warn MVK_CONFIG_LOG_LEVEL=1 \
  "$PROBE_BINARY" "$MOLTENVK_DYLIB" "$@"

echo "DXVK_D3D9=$DXVK_D3D9"
echo "DXVK_HEADERS=$HEADER_SOURCE_DIR/include/native/directx;$HEADER_SOURCE_DIR/include/native/windows"
echo "MOLTENVK_DYLIB=$MOLTENVK_DYLIB"
echo "SDL2_PREFIX=$SDL_PREFIX"
echo "STORM_DYLD_LIBRARY_PATH=$SDL_PREFIX/lib:$BUILD_DIR/src/d3d9:$(dirname "$MOLTENVK_DYLIB")"
