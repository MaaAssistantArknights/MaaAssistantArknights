

set(FASTDEPLOY_URL_PREFIX "https://github.com/MaaAssistantArknights/build-fastdeploy/releases/download")

set(FASTDEPLOY_TAG "gac255b8a")

set(COMPRESSED_SUFFIX ".tar.gz")

if(WIN32)
    set(FASTDEPLOY_FILENAME "FastDeploy-Windows")
    set(FASTDEPLOY_CHECKSUM "0a0700d4e8923bcd4c387a658e5e9a689ed9e0d114c260b11d082a2176c233d4")
elseif(APPLE)
    if (CURRENT_OSX_ARCH STREQUAL "arm64")
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-arm64")
        set(FASTDEPLOY_CHECKSUM "28a771814d197ba8056f1fcb7cc0671facec90a8733a0dac2cf0b68a8cc36d24")
    else()
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-x86_64")
        set(FASTDEPLOY_CHECKSUM "76e97c7e944a18eb160646fa32ac0a7e24c87751b2b127dc33454f993f6d4b97")
    endif()
else()
    set(FASTDEPLOY_FILENAME "FastDeploy-Linux")
    set(FASTDEPLOY_CHECKSUM "4d106e5499e9b5b29426667617215ac7e4e70d2353af10caccdfdf6c1f1534a7")
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

    find_package(FastDeploy REQUIRED PATHS ${FastDeploy_DIR} NO_DEFAULT_PATH)
    include_directories(${FastDeploy_INCLUDE_DIRS})
    list(APPEND DEPEND_LIBS ${FastDeploy_LIBS})
endif (FASTDEPLOY_DIRECTORY)

if (INSTALL_THIRD_LIBS)
    install(DIRECTORY ${FastDeploy_DIR}/lib/ DESTINATION . USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${ORT_LIB_PATH}/ DESTINATION . USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${FastDeploy_DIR}/third_libs/install/paddle2onnx/lib/
            DESTINATION . USE_SOURCE_PERMISSIONS)
endif (INSTALL_THIRD_LIBS)
