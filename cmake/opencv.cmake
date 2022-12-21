

set(OPENCV_URL_PREFIX "https://github.com/MaaAssistantArknights/build-opencv/releases/download")

set(OPENCV_TAG "4.5.3")

set(COMPRESSED_SUFFIX ".tar.gz")

if(WIN32)
    set(OPENCV_FILENAME "OpenCV-Windows")
    set(OPENCV_CHECKSUM "bf736b243bbdaa020f139e4dfa1e4f15633f4ce7a8ad885524645e660de47a8b")
elseif(APPLE)
    if (CURRENT_OSX_ARCH STREQUAL "arm64")
        set(OPENCV_FILENAME "OpenCV-macOS-arm64")
        set(OPENCV_CHECKSUM "31beb633c033dd4ee789ffa50911c29c9580860f9a91f334f03d8aa9c85e9700")
    else()
        set(OPENCV_FILENAME "OpenCV-macOS-x86_64")
        set(OPENCV_CHECKSUM "249c5c97cc52257b68d35acf499b1cf1037f5e6f3b40752f82ad5abe7884bea9")
    endif()
else(UNIX)
    set(OPENCV_FILENAME "OpenCV-Linux")
    set(OPENCV_CHECKSUM "edc4138456189c9e8bdf29114ad2be8ec152e8e31087d98e633f6cda59b141ea")
endif(WIN32)

set(OPENCV_URL ${OPENCV_URL_PREFIX}/${OPENCV_TAG}/${OPENCV_FILENAME}${COMPRESSED_SUFFIX})

if(OPENCV_DIRECTORY)
    set(OpenCV_DIR ${OPENCV_DIRECTORY})
    find_package(OpenCV REQUIRED PATHS ${OpenCV_DIR})
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND DEPEND_LIBS ${OpenCV_LIBS})
else()
    download_and_decompress(${OPENCV_URL}
                        ${CMAKE_CURRENT_BINARY_DIR}/${OPENCV_FILENAME}${COMPRESSED_SUFFIX}
                        ${OPENCV_CHECKSUM}
                        ${THIRD_PARTY_PATH}/install/)
    set(OPENCV_FILENAME opencv)
    set(OpenCV_DIR ${THIRD_PARTY_PATH}/install/${OPENCV_FILENAME})
    set(OPENCV_DIRECTORY ${OpenCV_DIR})
    if (WIN32)
        set(OpenCV_DIR ${OpenCV_DIR}/lib)
    endif()
    find_package(OpenCV REQUIRED PATHS ${OpenCV_DIR} NO_DEFAULT_PATH)
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND DEPEND_LIBS ${OpenCV_LIBS})
endif(OPENCV_DIRECTORY)

if (INSTALL_THIRD_LIBS)
    if (OpenCV_SHARED)
        install(DIRECTORY ${OpenCV_INSTALL_PATH}/lib/
                DESTINATION .
                USE_SOURCE_PERMISSIONS PATTERN "cmake" EXCLUDE)
    endif (OpenCV_SHARED)
endif (INSTALL_THIRD_LIBS)
