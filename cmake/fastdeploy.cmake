

set(FASTDEPLOY_URL_PREFIX "https://github.com/MaaAssistantArknights/build-fastdeploy/releases/download")

set(FASTDEPLOY_TAG "gac255b8a")

set(COMPRESSED_SUFFIX ".tar.gz")

if(WIN32)
    set(FASTDEPLOY_FILENAME "FastDeploy-Windows")
    set(FASTDEPLOY_CHECKSUM "fa1e6457f0787f578fca877f11bc1dc41b6eb196c542946757224e80f24b8008")
elseif(APPLE)
    if (CURRENT_OSX_ARCH STREQUAL "arm64")
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-arm64")
        set(FASTDEPLOY_CHECKSUM "5286563d17593f2124aff305181e22bedee264ef82730f54e6b538cacd9343ee")
    else()
        set(FASTDEPLOY_FILENAME "FastDeploy-macOS-x86_64")
        set(FASTDEPLOY_CHECKSUM "d0c8e4947301da1cbf295f79783c6016e1af139d67cc8ea3b532098ec6f1ae4d")
    endif()
else()
    set(FASTDEPLOY_FILENAME "FastDeploy-Linux")
    set(FASTDEPLOY_CHECKSUM "fc79d95251a82c584eac61c144f962de347469312c4be1d25fb2b8a624136998")
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
