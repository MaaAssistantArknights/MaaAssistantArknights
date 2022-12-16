

set(FASTDEPLOY_URL_PREFIX "https://github.com/MaaAssistantArknights/build-fastdeploy/releases/download")

set(FASTDEPLOY_TAG "g22325d23")

set(COMPRESSED_SUFFIX ".tar.gz")

if(WIN32)
    set(FASTDEPLOY_FILENAME "FastDeploy-Windows")
    set(FASTDEPLOY_CHECKSUM "83e23fd701c828b8e15e38806db65cde71c4fbba3482ca1e24ba23c44fb19028")
elseif(APPLE)
    if (CURRENT_OSX_ARCH STREQUAL "arm64")
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-arm64")
        set(FASTDEPLOY_CHECKSUM "e77cd3382859165dddcf98d71f433b815345baa11b3c2b01ce16bb2141f78a8a")
    else()
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-x86_64")
        set(FASTDEPLOY_CHECKSUM "7f1d5c4be83a479b819c33b094db5db565ae14c07bf0977b6a9b7a68d6b2a013")
    endif()
else()
    set(FASTDEPLOY_FILENAME "FastDeploy-Linux")
    set(FASTDEPLOY_CHECKSUM "b13891669242c6d219508ae99afba9fadde1ada4af9b3d5781298f58fef9bd34")
endif(WIN32)

set(FASTDEPLOY_URL ${FASTDEPLOY_URL_PREFIX}/${FASTDEPLOY_TAG}/${FASTDEPLOY_FILENAME}${COMPRESSED_SUFFIX})

if (FASTDEPLOY_DIRECTORY)
    set(FastDeploy_DIR ${FASTDEPLOY_DIRECTORY})
    find_package(FastDeploy REQUIRED PATHS ${FastDeploy_DIR})
    include_directories(${FastDeploy_INCLUDE_DIRS})
    list(APPEND DEPEND_LIBS ${FastDeploy_LIBS})
else ()
    download_and_decompress(${FASTDEPLOY_URL}
                        ${CMAKE_CURRENT_BINARY_DIR}/${FASTDEPLOY_FILENAME}${COMPRESSED_SUFFIX}
                        ${FASTDEPLOY_CHECKSUM}
                        ${THIRD_PARTY_PATH}/install/)
    set(FASTDEPLOY_FILENAME fastdeploy)
    set(FastDeploy_DIR ${THIRD_PARTY_PATH}/install/${FASTDEPLOY_FILENAME})

    if (APPLE)
        list(APPEND DEPEND_LIBS ${FastDeploy_DIR}/lib/libfastdeploy.dylib)
    else ()
        find_package(FastDeploy REQUIRED PATHS ${FastDeploy_DIR} NO_DEFAULT_PATH)
        include_directories(${FastDeploy_INCLUDE_DIRS})
        list(APPEND DEPEND_LIBS ${FastDeploy_LIBS})
    endif (APPLE)
endif (FASTDEPLOY_DIRECTORY)

if (INSTALL_THIRD_LIBS)
    install(DIRECTORY ${FastDeploy_DIR}/lib/ DESTINATION . USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${ORT_LIB_PATH}/ DESTINATION . USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${FastDeploy_DIR}/third_libs/install/paddle2onnx/lib/
            DESTINATION . USE_SOURCE_PERMISSIONS)
endif (INSTALL_THIRD_LIBS)
