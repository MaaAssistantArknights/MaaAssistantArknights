cmake_minimum_required(VERSION 3.28)

project(ResourceUpdater)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

# When built as a subdirectory of the main project, MaaUtils is already loaded.
# When built standalone, load it ourselves.
set(MAA_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../")
if(NOT TARGET HeaderOnlyLibraries)
    set(BUILD_MAA_UTILS OFF)
    include(${MAA_PROJECT_DIR}/src/MaaUtils/MaaUtils.cmake)
endif()

add_executable(res_updater main.cpp)

target_include_directories(res_updater PRIVATE ${MAA_PROJECT_DIR}/src/MaaCore) # For Utils
target_link_libraries(res_updater ${OpenCV_LIBS}) # Boost::system
if(LINUX)
    target_link_libraries(res_updater pthread)
endif()

maadeps_install(.)
install(TARGETS res_updater DESTINATION .)

if(WIN32)
    add_custom_command(
        TARGET res_updater
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
        $<TARGET_FILE_DIR:res_updater>
        COMMAND_EXPAND_LISTS)
endif()

if(APPLE)
    set_target_properties(res_updater PROPERTIES INSTALL_RPATH "@loader_path/")
elseif(UNIX)
    set_target_properties(res_updater PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif()

if(MSVC)
    target_compile_options(res_updater PRIVATE "/WX-")
else()
    target_compile_options(res_updater PRIVATE "-Wno-error")
endif()
