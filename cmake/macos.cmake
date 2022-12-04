set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)

if (BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif ()

if (MAA_DEPS_PATH AND EXISTS ${MAA_DEPS_PATH})
    message(STATUS "Using MAA_DEPS_PATH: ${MAA_DEPS_PATH}")
else ()
    message(STATUS "MAA_DEPS_PATH not set, using default")

    include(FetchContent)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(
        MAA_DEPS
        URL https://github.com/hguandl/maa-deps/releases/download/v20221204/maa-deps-macos-v20221204.tar.xz
        URL_HASH SHA256=5e193f80a74d1591531bf7de5afccc21b8579fb3363a0b089198cb75fbf89df2)
    FetchContent_MakeAvailable(MAA_DEPS)

    set(MAA_DEPS_PATH ${maa_deps_SOURCE_DIR} CACHE PATH "Path to maa-deps" FORCE)
endif ()

set(FastDeploy_LIBS
    "${MAA_DEPS_PATH}/lib/libfastdeploy.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_common.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_flatbuffers.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_framework.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_graph.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_mlas.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_optimizer.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_providers.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_session.a"
    "${MAA_DEPS_PATH}/lib/libonnxruntime_util.a"
    "${MAA_DEPS_PATH}/lib/libpaddle2onnx.1.0.4.dylib"
    "${MAA_DEPS_PATH}/lib/libprotobuf.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libabsl_city.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libabsl_hash.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libabsl_low_level_hash.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libabsl_raw_hash_set.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libabsl_throw_delegate.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libnsync_cpp.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libonnx.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libonnx_proto.a"
    "${MAA_DEPS_PATH}/lib/onnxruntime/external/libre2.a"
)
