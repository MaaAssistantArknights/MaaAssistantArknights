cmake_minimum_required(VERSION 3.24)

project(MAAUnitTests LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # 源文件是 UTF-8 无 BOM，MSVC 默认按系统代码页读，中文注释和字符串会被读坏
    # （936/GBK 等双字节代码页下会直接报语法错），与 src/MaaUtils/cmake/config.cmake 保持一致
    add_compile_options("/utf-8")
endif()

set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS ON CACHE BOOL "" FORCE)
set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Catch2)

include(Catch)

add_executable(maa-algorithm-test
    MaaCore/AlgorithmTest.cpp
)

target_compile_features(maa-algorithm-test PRIVATE cxx_std_20)
target_link_libraries(maa-algorithm-test PRIVATE Catch2::Catch2WithMain)
target_include_directories(maa-algorithm-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/MaaCore)

catch_discover_tests(maa-algorithm-test TEST_PREFIX "algorithm::")

add_executable(maa-adb-failure-test
    MaaCore/AdbFailureTest.cpp
)

target_compile_features(maa-adb-failure-test PRIVATE cxx_std_20)
target_link_libraries(maa-adb-failure-test PRIVATE Catch2::Catch2WithMain)
target_include_directories(maa-adb-failure-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/MaaCore)

catch_discover_tests(maa-adb-failure-test TEST_PREFIX "adb_failure::")