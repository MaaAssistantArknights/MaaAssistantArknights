cmake_minimum_required(VERSION 3.28)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

option(BUILD_WPF_GUI "build MaaWpfGui" ${WIN32})
option(BUILD_DEBUG_DEMO "build debug demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_UNIVERSAL "build both arm64 and x86_64 on macOS" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_FLATTEN "do not use bin lib include directory" ON)
option(WITH_EMULATOR_EXTRAS "build with emulator extras" ${WIN32})
option(BUILD_SMOKE_TEST "build smoke_test" OFF)
option(BUILD_RES_UPDATER "build ResourceUpdater" OFF)

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
if(LINUX)
    target_link_libraries(MaaCore pthread)
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

# 创建资源目录链接的函数
function(create_resource_link TARGET_NAME OUTPUT_DIR)
    if(WIN32)
        # Windows 使用 mklink /J 创建目录链接（不需要管理员权限）
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Creating resource directory link for ${TARGET_NAME}..."
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT_DIR}/resource"
            COMMAND cmd /c "mklink /J \"${OUTPUT_DIR}/resource\" \"${PROJECT_SOURCE_DIR}/resource\""
            COMMAND ${CMAKE_COMMAND} -E echo "Resource directory link created successfully"
            COMMENT "Creating junction for resource directory for ${TARGET_NAME}"
        )
    else()
        # Unix/Linux/macOS 使用符号链接
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Creating resource directory link for ${TARGET_NAME}..."
            COMMAND ${CMAKE_COMMAND} -E remove "${OUTPUT_DIR}/resource"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/resource" "${OUTPUT_DIR}/resource"
            COMMAND ${CMAKE_COMMAND} -E echo "Resource directory link created successfully"
            COMMENT "Creating symlink for resource directory for ${TARGET_NAME}"
        )
    endif()
endfunction()

if(WIN32)
    add_custom_command(
        TARGET MaaCore
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${PROJECT_SOURCE_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
        $<TARGET_FILE_DIR:MaaCore>
        COMMAND_EXPAND_LISTS)
endif()

if (BUILD_WPF_GUI)
    include_external_msproject(MaaWpfGui ${PROJECT_SOURCE_DIR}/src/MaaWpfGui/MaaWpfGui.csproj)
    
    add_dependencies(MaaWpfGui MaaCore)

    # 为 MaaWpfGui 创建资源目录链接
    create_resource_link(MaaWpfGui "${PROJECT_SOURCE_DIR}/bin/$<CONFIG>")
endif()

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

if(INSTALL_FLATTEN)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

install(TARGETS MaaCore ${MaaCore_install_flatten_args})

maadeps_install(.)

if (INSTALL_PYTHON)
    install(DIRECTORY src/Python DESTINATION .)
endif (INSTALL_PYTHON)
if (INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
endif (INSTALL_RESOURCE)

if (BUILD_DEBUG_DEMO)
    add_executable(debug_demo src/Cpp/main.cpp)
    if(MSVC)
        target_compile_options(debug_demo PRIVATE "/WX-")
    else()
        target_compile_options(debug_demo PRIVATE "-Wno-error")
    endif()
    target_link_libraries(debug_demo MaaCore)

    add_dependencies(debug_demo MaaCore)

    # 为 debug_demo 创建资源目录链接
    create_resource_link(debug_demo "$<TARGET_FILE_DIR:debug_demo>")
    # install(TARGETS debug_demo ${MaaCore_install_flatten_args})
endif (BUILD_DEBUG_DEMO)

if (BUILD_SMOKE_TEST)
    add_executable(smoke_test src/Cpp/main.cpp)
    if(MSVC)
        target_compile_options(smoke_test PRIVATE "/WX-")
    else()
        target_compile_options(smoke_test PRIVATE "-Wno-error")
    endif()
    target_compile_definitions(smoke_test PRIVATE SMOKE_TESTING)
    target_link_libraries(smoke_test MaaCore)

    add_dependencies(smoke_test MaaCore)
    
    # 为 smoke_test 创建资源目录链接
    create_resource_link(smoke_test "$<TARGET_FILE_DIR:smoke_test>")
    install(TARGETS smoke_test ${MaaCore_install_flatten_args})
endif (BUILD_SMOKE_TEST)

if (BUILD_RES_UPDATER)
    add_executable(ResourceUpdater tools/ResourceUpdater/main.cpp)
    target_include_directories(ResourceUpdater PUBLIC include PRIVATE src/MaaCore) # For Utils/Time.hpp
    target_link_libraries(ResourceUpdater HeaderOnlyLibraries ${OpenCV_LIBS})
    if(LINUX)
        target_link_libraries(ResourceUpdater pthread)
    endif()
    install(TARGETS ResourceUpdater DESTINATION . COMPONENT ResourceUpdater)

    # Copy from maadeps_install, adding component
    if(MSVC)
        install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>" DESTINATION . USE_SOURCE_PERMISSIONS COMPONENT ResourceUpdater)
    else()
        install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/" DESTINATION . USE_SOURCE_PERMISSIONS COMPONENT ResourceUpdater)
    endif()
endif (BUILD_RES_UPDATER)

if (APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif (APPLE)
