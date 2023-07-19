#!/usr/bin/env zsh

basedir=$(dirname "$0")/..
pushd ${basedir}

build_arch() {
    [[ $1 = "arm64" ]] && triplet="arm64-osx" || triplet="x64-osx"
    python3 maadeps-download.py ${triplet}

    cmake -B build-$1 -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=$1
    cmake --build build-$1
    cmake --install build-$1 --prefix install-$1
}

build_arch "arm64"
build_arch "x86_64"

rm -rf build
mkdir build
for LIB_NAME in $(ls install-arm64 | grep .dylib); do
    lipo -create install-arm64/$LIB_NAME install-x86_64/$LIB_NAME -output build/$LIB_NAME
done

pushd build
xcodebuild -create-xcframework -library libMaaCore.dylib -headers ../include -output MaaCore.xcframework
xcodebuild -create-xcframework -library libMaaDerpLearning.dylib -output MaaDerpLearning.xcframework
xcodebuild -create-xcframework -library libonnxruntime.*.dylib -output ONNXRuntime.xcframework
xcodebuild -create-xcframework -library libopencv*.dylib -output OpenCV.xcframework
rm -rf *.dylib
popd

rm -rf install-arm64 install-x86_64
popd
