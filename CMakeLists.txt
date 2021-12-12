cmake_minimum_required(VERSION 2.8)
project(MeoAsst)

include_directories(include 3rdparty/include)
aux_source_directory(src/MeoAssistance SRC)
add_definitions(-DMEO_DLL_EXPORTS)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/MP>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_STANDARD 17)

add_library(MeoAsst_LIB SHARED ${SRC})

find_library(Opencv_LIB NAMES opencv_world453 PATHS 3rdparty/lib)
find_library(PaddleOCR_LIB NAMES ppocr PATHS 3rdparty/lib)
find_library(Penguin_LIB NAMES penguin-stats-recognize PATHS 3rdparty/lib)
find_library(MeoJson_LIB NAMES libmeojson PATHS 3rdparty/lib)

target_link_libraries(MeoAsst_LIB ${Opencv_LIB} ${PaddleOCR_LIB} ${Penguin_LIB} ${MeoJson_LIB})
