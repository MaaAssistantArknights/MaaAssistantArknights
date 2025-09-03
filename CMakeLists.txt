cmake_minimum_required(VERSION 3.28)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

option(BUILD_TEST "build a demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_UNIVERSAL "build both arm64 and x86_64 on macOS" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_DEVEL "install development files" OFF)
option(INSTALL_FLATTEN "do not use bin lib include directory" ON)
option(WITH_EMULATOR_EXTRAS "build with emulator extras" ${WIN32})
option(WITH_ASST_DEBUG "build with ASST_DEBUG defined" OFF)

include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)

add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE 3rdparty/include 3rdparty/EmulatorExtras)

file(GLOB_RECURSE maa_src src/MaaCore/*.h src/MaaCore/*.hpp src/MaaCore/*.cpp)

add_library(MaaCore SHARED ${maa_src})

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED CONFIG COMPONENTS system)
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime REQUIRED)

target_link_libraries(MaaCore HeaderOnlyLibraries ${OpenCV_LIBS} fastdeploy_ppocr ONNXRuntime::ONNXRuntime ZLIB::ZLIB Boost::system)
if(WIN32)
    target_link_libraries(MaaCore ws2_32)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_src})

if (WIN32)
    #注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    target_compile_definitions(MaaCore PRIVATE ASST_DLL_EXPORTS _UNICODE UNICODE)
endif ()

target_include_directories(MaaCore PUBLIC include PRIVATE src/MaaCore)
set(MaaCore_PUBLIC_HEADERS include/AsstCaller.h include/AsstPort.h)
target_sources(MaaCore PUBLIC ${MaaCore_PUBLIC_HEADERS})
set_target_properties(MaaCore PROPERTIES PUBLIC_HEADER "${MaaCore_PUBLIC_HEADERS}")

# if (WIN32)
#     include_external_msproject(SyncRes ${PROJECT_SOURCE_DIR}/src/SyncRes/SyncRes.csproj)
#     include_external_msproject(MaaWpfGui ${PROJECT_SOURCE_DIR}/src/MaaWpfGui/MaaWpfGui.csproj)
    
#     add_dependencies(MaaCore SyncRes)
#     add_dependencies(MaaWpfGui MaaCore SyncRes)
# endif()

if(APPLE)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "@loader_path/")
elseif(UNIX)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif()

if(WITH_EMULATOR_EXTRAS AND NOT EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/EmulatorExtras/Mumu)
    message(WARNING "EmulatorExtras not found, please run `git submodule update --init 3rdparty/EmulatorExtras`")
    set(WITH_EMULATOR_EXTRAS OFF)
endif()
target_compile_definitions(MaaCore PRIVATE ASST_WITH_EMULATOR_EXTRAS=$<BOOL:${WITH_EMULATOR_EXTRAS}>)

if (WITH_ASST_DEBUG)
    target_compile_definitions(MaaCore PRIVATE ASST_DEBUG)
endif ()


if(INSTALL_DEVEL)
    set(MaaCore_install_extra_args PUBLIC_HEADER DESTINATION devel/include ARCHIVE DESTINATION devel/lib)
endif()

if(INSTALL_FLATTEN)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

install(TARGETS MaaCore ${MaaCore_install_flatten_args} ${MaaCore_install_extra_args})

maadeps_install(.)

if (INSTALL_PYTHON)
    install(DIRECTORY src/Python DESTINATION .)
endif (INSTALL_PYTHON)
if (INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
endif (INSTALL_RESOURCE)

if (BUILD_TEST)
    add_executable(test src/Cpp/main.cpp)
    set_target_properties(test PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    target_link_libraries(test MaaCore)
endif (BUILD_TEST)

if (APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif (APPLE)
