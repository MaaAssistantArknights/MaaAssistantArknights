cmake_minimum_required(VERSION 3.21)
project(ResourceUpdaterUnix)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define a variable for the source directory
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../)

# Include directories
include_directories(
    ${SOURCE_DIR}/3rdparty/include
    ${SOURCE_DIR}/MaaDeps/vcpkg/installed/${MAADEPS_TRIPLET}/include
    ${SOURCE_DIR}/src/MaaCore
)

include(${SOURCE_DIR}/cmake/utils.cmake)
include(${SOURCE_DIR}/MaaDeps/maadeps.cmake)

# Link directories
link_directories(
    ${SOURCE_DIR}/MaaDeps/vcpkg/installed/${MAADEPS_TRIPLET}/lib
)

find_package(OpenCV REQUIRED COMPONENTS core)
# Source files
set(SOURCES
    main.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    opencv_core
)