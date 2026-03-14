#!/usr/bin/env zsh

set -eu -o pipefail

[[ "$(arch)" == "arm64" ]] && arch="arm64" || arch="x86_64"
basedir=$(dirname "$0")/..
maa_debug=${MAA_DEBUG:-0}
pushd ${basedir}

build_arch() {
    // remove previous artifacts
    rm -rf build-$1
    [[ $1 = "arm64" ]] && triplet="arm64-osx" || triplet="x64-osx"

    python3 tools/maadeps-download.py --cache-asset ${triplet}

    if [[ -n $(which ccache) ]]; then
        export CMAKE_C_COMPILER_LAUNCHER=ccache
        export CMAKE_CXX_COMPILER_LAUNCHER=ccache
    fi

    cmake -B build-$1 -GNinja -DCMAKE_BUILD_TYPE=$2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_OSX_ARCHITECTURES=$1
    cmake --build build-$1
    cmake --install build-$1 --prefix install-$1
}

if [[ ${maa_debug} -eq 1 ]]; then
    build_arch "${arch}" "Debug"
elif [[ ${maa_debug} -eq 0 ]]; then
    build_arch "arm64" "Release"
    build_arch "x86_64" "Release"
else
    echo "Invalid MAA_DEBUG value: ${maa_debug}, please set to 0 or 1."
    exit 1
fi

make -f tools/xcframework.makefile MAA_DEBUG=${maa_debug} -j$(sysctl -n hw.ncpu) all

popd
