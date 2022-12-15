

set(FASTDEPLOY_URL_PREFIX "https://github.com/MaaAssistantArknights/build-fastdeploy/releases/download")

set(FASTDEPLOY_TAG "g22325d23")

set(COMPRESSED_SUFFIX ".tar.gz")

if(UNIX)
    set(FASTDEPLOY_FILENAME "FastDeploy-Linux")
    set(FASTDEPLOY_CHECKSUM "b13891669242c6d219508ae99afba9fadde1ada4af9b3d5781298f58fef9bd34")
elseif(WIN32)
    set(FASTDEPLOY_FILENAME "FastDeploy-Windows")
    set(FASTDEPLOY_CHECKSUM "83e23fd701c828b8e15e38806db65cde71c4fbba3482ca1e24ba23c44fb19028")
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
