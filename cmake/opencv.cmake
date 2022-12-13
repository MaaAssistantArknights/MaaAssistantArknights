

set(OPENCV_URL_PREFIX "https://github.com/MaaAssistantArknights/build-opencv/releases/download")

set(OPENCV_TAG "4.5.3")

set(COMPRESSED_SUFFIX ".tar.gz")

if(UNIX)
    set(OPENCV_FILENAME "OpenCV-Linux")
    set(OPENCV_CHECKSUM "ab41d96f64d8aada3c06584f88bcc5c94eab9f8a4c1a6fbe2e20d83d5509f1c3")
elseif(WIN32)
    set(OPENCV_FILENAME "OpenCV-Windows")
    set(OPENCV_CHECKSUM "92213f8615d5ca74bf6a8a3681abaac1f5981f9da6b5a1808759909329bb5ea2")
elseif(APPLE)
    set(OPENCV_FILENAME "OpenCV-macOS")
    set(OPENCV_CHECKSUM "44d6308696ee0b38d1ba40b8b16ee0cb1da4a0f9f72b6916afc5b18b99a9089e")
endif(UNIX)

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
