cmake_minimum_required(VERSION 3.28)
project(MAA)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

option(BUILD_WPF_GUI "build MaaWpfGui" ${WIN32})
option(BUILD_DEBUG_DEMO "build debug demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_SMOKE_TEST "build smoke_test" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_FLATTEN "do not use bin lib include directory" ON)
option(WITH_EMULATOR_EXTRAS "build with emulator extras" ${WIN32})
option(WITH_HASH_VERSION "generate version from git hash" OFF)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)

if(APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif()

if(INSTALL_FLATTEN)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE 3rdparty/include)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(OpenCV QUIET COMPONENTS xfeatures2d)
find_package(Boost REQUIRED CONFIG)
if(Boost_VERSION VERSION_LESS 1.89.0)
  find_package(Boost REQUIRED CONFIG COMPONENTS system)
endif()
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime REQUIRED)

add_subdirectory(src/MaaCore)

if(BUILD_WPF_GUI)
    include_external_msproject(MaaWpfGui ${PROJECT_SOURCE_DIR}/src/MaaWpfGui/MaaWpfGui.csproj)

    add_dependencies(MaaWpfGui MaaCore)
endif()

if(INSTALL_PYTHON)
    install(DIRECTORY src/Python DESTINATION .)
endif()

if(INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
endif()

if(BUILD_DEBUG_DEMO OR BUILD_SMOKE_TEST)
    add_subdirectory(src/Cpp)
endif()
