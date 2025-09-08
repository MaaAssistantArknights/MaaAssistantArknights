set(debug_comp_defs "_DEBUG;MAA_DEBUG")
add_compile_definitions("$<$<CONFIG:Debug>:${debug_comp_defs}>")

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path;@executable_path")
    set(CMAKE_BUILD_RPATH "@loader_path;@executable_path")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_RPATH "$ORIGIN")
endif()

if(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")
    add_compile_options("/W4;/WX;/Gy;/permissive-;/sdl")
    add_compile_options("/wd4127") # conditional expression is constant
    add_compile_options("/wd4251") # export dll with templates

    add_compile_options("/DWINVER=0x0A00")
    add_compile_options("/D_WIN32_WINNT=0x0A00")

    # https://github.com/actions/runner-images/issues/10004 https://github.com/microsoft/STL/releases/tag/vs-2022-17.10
    add_compile_definitions("_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

    set(release_link_options "/OPT:REF;/OPT:ICF")
    add_link_options("$<$<CONFIG:Release>:${release_link_options}>")
    SET(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RelWithDebInfo;Release;")
    SET(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL "MinSizeRel;Release;")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    add_compile_options("-Wall;-Werror;-Wextra;-Wpedantic;-Wno-missing-field-initializers")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        add_compile_options("-Wno-restrict")
    endif()
endif()

if(LINUX)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so.6
        OUTPUT_VARIABLE LIBSTDCPP_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    file(READ_SYMLINK "${LIBSTDCPP_PATH}" LINK_TARGET)
    if(NOT IS_ABSOLUTE "${LINK_TARGET}")
        get_filename_component(LIBSTDCPP_PATH_DIR "${LIBSTDCPP_PATH}" DIRECTORY)
        file(REAL_PATH "${LIBSTDCPP_PATH_DIR}/${LINK_TARGET}" LIBSTDCPP_PATH_REAL)
    else()
        set(LIBSTDCPP_PATH_REAL "${LINK_TARGET}")
    endif()
    message(STATUS "libstdc++.so.6 path: ${LIBSTDCPP_PATH_REAL}")
    if(NOT EXISTS "${LIBSTDCPP_PATH_REAL}")
        message(FATAL_ERROR "File not found: ${LIBSTDCPP_PATH_REAL}")
    endif()
    install(FILES "${LIBSTDCPP_PATH_REAL}" DESTINATION bin RENAME libstdc++.so.6)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
