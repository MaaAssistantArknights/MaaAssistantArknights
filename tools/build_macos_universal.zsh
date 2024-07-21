#!/usr/bin/env zsh

arch=$(arch)
basedir=$(dirname "$0")/..
pushd ${basedir}

build_arch() {
    [[ $1 = "arm64" ]] && triplet="arm64-osx" || triplet="x64-osx"

    if [ ! -d ${basedir}/MaaDeps/runtime/maa-${triplet} ]; then
        python3 maadeps-download.py ${triplet}
    fi

    if [[ -n $(which ccache) ]]; then
        ccache_arg="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    else
        ccache_arg=""
    fi

    cmake -B build-$1 -GNinja -DCMAKE_BUILD_TYPE=$2 -DCMAKE_OSX_ARCHITECTURES=$1 "${ccache_arg}"
    cmake --build build-$1
    cmake --install build-$1 --prefix install-$1
}

if [[ -n "${MAA_DEBUG}" ]]; then
    build_arch "${arch}" "Debug"
else
    build_arch "arm64" "Release"
    build_arch "x86_64" "Release"
fi

rm -rf build
mkdir build
for LIB_NAME in $(ls install-"${arch}" | grep .dylib); do
    if [[ -n "${MAA_DEBUG}" ]]; then
        mv install-"${arch}"/$LIB_NAME build/$LIB_NAME
    else
        lipo -create install-arm64/$LIB_NAME install-x86_64/$LIB_NAME -output build/$LIB_NAME
    fi
done
cp build-"${arch}"/compile_commands.json build

pushd build
xcodebuild -create-xcframework -library libMaaCore.dylib -headers ../include -output MaaCore.xcframework
xcodebuild -create-xcframework -library libfastdeploy_ppocr.dylib -output fastdeploy_ppocr.xcframework
xcodebuild -create-xcframework -library libonnxruntime.*.dylib -output ONNXRuntime.xcframework
xcodebuild -create-xcframework -library libopencv*.dylib -output OpenCV.xcframework
rm -rf *.dylib
popd

rm -rf install-arm64 install-x86_64
popd
