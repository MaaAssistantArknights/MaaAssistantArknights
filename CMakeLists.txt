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
endif()

if(INSTALL_PYTHON)
    install(DIRECTORY src/Python DESTINATION .)
endif()

if(INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        install(CODE "
            set(minify_script \"${PROJECT_SOURCE_DIR}/tools/minify_json_folder.py\")
            set(resource_dir \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/resource\")
            execute_process(
                COMMAND \"${Python3_EXECUTABLE}\" \"\${minify_script}\" \"\${resource_dir}\"
                RESULT_VARIABLE minify_result
                ERROR_VARIABLE minify_error
                OUTPUT_QUIET
            )
            if(minify_result)
                message(WARNING \"minify_json_folder.py failed (exit \${minify_result}), resource JSON not minified\")
            else()
                message(STATUS \"minify_json_folder.py succeeded, resource JSON minified\")
            endif()
        ")
    else()
        message(STATUS "Python3 not found, skipping JSON minify during install")
    endif()
endif()

if(BUILD_DEBUG_DEMO OR BUILD_SMOKE_TEST)
    add_subdirectory(src/Cpp)
endif()
