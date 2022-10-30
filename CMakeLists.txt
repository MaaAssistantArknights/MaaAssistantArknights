cmake_minimum_required(VERSION 3.1)
project(MaaAssistantArknights)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

option(BUILD_TEST "build a demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_UNIVERSAL "build both arm64 and x86_64 on macOS" OFF)

if (BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

    add_compile_options("/W4;/WX")
else ()
    add_compile_options("-Wall;-Wextra;-Wpedantic")
endif ()

add_definitions(-DASST_DLL_EXPORTS)
if (MSVC)
    #注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    add_definitions(-D_UNICODE -DUNICODE)
endif ()

include_directories(include)
include_directories(src/MeoAssistant)

file(GLOB_RECURSE maa_src src/MeoAssistant/*.cpp)

add_library(MeoAssistant SHARED ${maa_src})

if (MSVC)
    find_library(PaddleOCR_LIB NAMES ppocr PATHS 3rdparty/lib)
    find_library(OpenCV NAMES opencv_world453 PATHS 3rdparty/lib)
    find_library(ZLIB NAMES zlibstatic PATHS 3rdparty/lib)
    target_link_libraries(MeoAssistant ws2_32 ${OpenCV} ${PaddleOCR_LIB} ${ZLIB})
    target_include_directories(MeoAssistant PRIVATE 3rdparty/include)
else ()
    find_package(ZLIB REQUIRED)
    target_include_directories(MeoAssistant PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(MeoAssistant ${ZLIB_LIBRARY})

    if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        find_package(range-v3 REQUIRED)
        target_link_libraries(MeoAssistant range-v3::range-v3)
    endif ()

    if (APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
        add_subdirectory(3rdparty/VisionOCR)
        target_link_libraries(MeoAssistant
            vnocr
            "-framework Accelerate"
        )
    else ()
        find_package(OpenCV REQUIRED)
        target_include_directories(MeoAssistant PRIVATE ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(MeoAssistant ${OpenCV_LIBS})

        target_link_libraries(MeoAssistant ppocr paddle_inference protobuf cryptopp gflags glog utf8proc xxhash iomp5 mkldnn mklml_intel)
    endif (APPLE)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -idirafter ${PROJECT_SOURCE_DIR}/3rdparty/include")
endif ()

if (BUILD_TEST)
    add_executable(test tools/TestCaller/main.cpp)
    target_link_libraries(test MeoAssistant)
endif (BUILD_TEST)

if (BUILD_XCFRAMEWORK)
    add_custom_command(OUTPUT MeoAssistant.xcframework
        COMMAND rm -rf MeoAssistant.xcframework
        COMMAND xcodebuild -create-xcframework -library libMeoAssistant.dylib -headers ${PROJECT_SOURCE_DIR}/include -output MeoAssistant.xcframework
        DEPENDS MeoAssistant
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS MeoAssistant MeoAssistant.xcframework
    )
endif (BUILD_XCFRAMEWORK)

# define MAA_VERSION from git
if (NOT MAA_VERSION)
    find_package(Git)
endif ()
if (GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty --broken --abbrev=40
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result EQUAL 0)
        set(MAA_VERSION "${output}")
    else ()
        message(WARNING "git describe returning ${result}, output:\n${err}")
    endif()
endif ()
if (NOT MAA_VERSION AND GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result EQUAL 0)
        set(MAA_VERSION "${output}")
    else ()
        message(WARNING "git rev-parse returning ${result}, output:\n${err}")
    endif ()
endif ()
if (NOT MAA_VERSION)
    set(MAA_VERSION "DEBUG VERSION")
endif ()
message(STATUS "MAA_VERSION=${MAA_VERSION}")
add_compile_definitions(MAA_VERSION="${MAA_VERSION}")
