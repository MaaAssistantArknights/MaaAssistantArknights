cmake_minimum_required(VERSION 3.28)

project(MAA)

option(BUILD_WPF_GUI "build MaaWpfGui" ${WIN32})
option(BUILD_DEBUG_DEMO "build debug demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_SMOKE_TEST "build smoke_test" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_FLATTEN "do not use bin lib include directory" ON)
option(WITH_EMULATOR_EXTRAS "build with emulator extras" ${WIN32})
option(WITH_HASH_VERSION "generate version from git hash" OFF)
option(BUILD_RESOURCE_UPDATER "build resource updater tool" OFF)

if(INSTALL_FLATTEN)
    set(RPATH_LIBRARY_INSTALL_DIR .)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

include(src/MaaUtils/MaaUtils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

if(APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif()

add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE 3rdparty/include)

add_subdirectory(src/MaaCore)

if(BUILD_WPF_GUI)
    include_external_msproject(MaaWpfGui ${PROJECT_SOURCE_DIR}/src/MaaWpfGui/MaaWpfGui.csproj)

    add_dependencies(MaaWpfGui MaaCore)
    if(DEFINED ENV{VSCODE_PID})
        add_custom_target(run-MaaWpfGui
            COMMAND "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/MAA.exe"
            DEPENDS MaaWpfGui
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        )
    endif()
endif()

if (ANDROID)
    add_library(stdc++fs INTERFACE)
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
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

if(BUILD_RESOURCE_UPDATER)
    add_subdirectory(tools/ResourceUpdater)
endif()
