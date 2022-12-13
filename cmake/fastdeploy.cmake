

set(FASTDEPLOY_URL_PREFIX "https://github.com/MaaAssistantArknights/build-fastdeploy/releases/download")

set(FASTDEPLOY_TAG "g22325d23")

set(COMPRESSED_SUFFIX ".tar.gz")

if(UNIX)
    set(FASTDEPLOY_FILENAME "FastDeploy-Linux")
    set(FASTDEPLOY_CHECKSUM "5e1dedc72714f6f11fb1dfbc720d675203525152b777bd2711ba26f2a95e5f99")
elseif(WIN32)
    set(FASTDEPLOY_FILENAME "FastDeploy-Windows")
    set(FASTDEPLOY_CHECKSUM "0ea6064921a0af50ba8a1593fe1652a11379acad53f15e100731f02c3c9edfd1")
elseif(APPLE)
    set(FASTDEPLOY_FILENAME "FastDeploy-macOS")
    set(FASTDEPLOY_CHECKSUM "80188fb70c5632ba5b93338d5c6d323619430dcad77043250399dce8fb480d8d")
endif(UNIX)

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
