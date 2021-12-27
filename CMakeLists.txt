cmake_minimum_required(VERSION 2.8)
project(MeoAssistantArknights)

option(BUILD_TEST "build a demo" OFF)

include_directories(include 3rdparty/include)
aux_source_directory(src/MeoAssistant SRC)
add_definitions(-DASST_DLL_EXPORTS)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/MP>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

set(CMAKE_CXX_STANDARD 17)

add_library(MeoAssistant SHARED ${SRC})

find_library(PaddleOCR_LIB NAMES ppocr PATHS 3rdparty/lib)
find_library(Penguin_LIB NAMES penguin-stats-recognize PATHS 3rdparty/lib)
find_library(MeoJson_LIB NAMES meojson PATHS 3rdparty/lib)

if (WIN32)
    find_library(OpenCV NAMES opencv_world453 PATHS 3rdparty/lib)
else ()
    find_package(OpenCV REQUIRED)
endif ()

target_link_libraries(MeoAssistant ${OpenCV} ${PaddleOCR_LIB} ${Penguin_LIB} ${MeoJson_LIB})

if (BUILD_TEST)
    add_executable(test tools/TestCaller/main.cpp)
    target_link_libraries(test MeoAssistant)
endif(BUILD_TEST)
