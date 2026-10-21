#!/bin/zsh

set -euo pipefail

build_root="${CORSAIRS_BUILD_ROOT:-${HOME}/Library/Application Support/CorsairsBuild}"
source_dir="${STORM_ENGINE_SOURCE:-${HOME}/05_Repo/StormEngine-Corsairs}"
build_dir="${STORM_ENGINE_BUILD_DIR:-${source_dir}/build/crossover-msvc}"
crossover_app="${CROSSOVER_APP:-${HOME}/Applications/CrossOver.app}"
cmake_bin="${CORSAIRS_CMAKE_BIN:-${build_root}/conan-home/.conan/data/cmake/3.30.5/_/_/package/cc471ed355602ffa7007968c83aa4e7e648a38b5/CMake.app/Contents/bin/cmake}"
msvc_bin="${build_root}/msvc-17.5-x64/bin/x64"
conan_bin="${build_root}/conan-1.60/bin"
toolchain="${build_root}/msvc-wine-toolchain.cmake"
build_jobs="${CORSAIRS_BUILD_JOBS:-1}"

if [[ ! "${build_jobs}" =~ '^[1-9][0-9]*$' ]]; then
  print -u2 "CORSAIRS_BUILD_JOBS must be a positive integer: ${build_jobs}"
  exit 1
fi

for required in "${cmake_bin}" "${msvc_bin}/cl" "${conan_bin}/conan" "${toolchain}"; do
  if [[ ! -e "${required}" ]]; then
    print -u2 "Build dependency not found: ${required}"
    exit 1
  fi
done

export CX_ROOT="${crossover_app}/Contents/SharedSupport/CrossOver"
export CX_BOTTLE="CORSAIRS_BUILD"
export CONAN_USER_HOME="${build_root}/conan-home"
export WINE_MSVC_RAW_STDOUT="${WINE_MSVC_RAW_STDOUT:-1}"
export PATH="${cmake_bin:h}:${msvc_bin}:${conan_bin}:${CX_ROOT}/CrossOver-Hosted Application:/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin"

"${cmake_bin}" \
  -S "${source_dir}" \
  -B "${build_dir}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="${toolchain}" \
  -DSTORM_ENABLE_CRASH_REPORTS=OFF \
  -DSTORM_ENABLE_STEAM=OFF \
  -DSTORM_ENABLE_SAFE_MODE=OFF \
  -DSTORM_USE_CONAN_SDL=ON

"${cmake_bin}" --build "${build_dir}" --target engine --parallel "${build_jobs}"

engine="${build_dir}/bin/engine.exe-1"
file "${engine}"
shasum -a 256 "${engine}"
watermark="$(strings -a "${engine}" | rg -m 1 'codex/corsairs-engine|Recon_release' || true)"
if [[ -z "${watermark}" ]]; then
  print -u2 "Engine watermark not found"
  exit 1
fi
print -r -- "${watermark}"
