cmake_minimum_required(VERSION 3.28)
project(ResourceUpdater)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

set(MAA_PROJECT_DIR "${PROJECT_SOURCE_DIR}/../../")

list(APPEND CMAKE_MODULE_PATH "${MAA_PROJECT_DIR}/cmake/modules")

include(${MAA_PROJECT_DIR}/MaaDeps/maadeps.cmake)

include(${MAA_PROJECT_DIR}/cmake/config.cmake)
include(${MAA_PROJECT_DIR}/cmake/utils.cmake)
include(${MAA_PROJECT_DIR}/cmake/version.cmake)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
# find_package(Boost REQUIRED CONFIG COMPONENTS system)

add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE ${MAA_PROJECT_DIR}/3rdparty/include)

add_executable(res_updater main.cpp)

target_include_directories(res_updater PUBLIC ${MAA_PROJECT_DIR}/include PRIVATE ${MAA_PROJECT_DIR}/src/MaaCore) # For Utils/Time.hpp
target_link_libraries(res_updater HeaderOnlyLibraries ${OpenCV_LIBS}) # Boost::system
if(LINUX)
    target_link_libraries(res_updater pthread)
endif()

install(TARGETS res_updater DESTINATION .)
maadeps_install(.)

if(WIN32)
    add_custom_command(
        TARGET res_updater
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${MAA_PROJECT_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
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