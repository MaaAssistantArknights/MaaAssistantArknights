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
option(WITH_MAC_SCK "build with macOS ScreenCaptureKit" ${APPLE})
option(WITH_HASH_VERSION "generate version from git hash" OFF)
option(BUILD_RESOURCE_UPDATER "build resource updater tool" OFF)

if(INSTALL_FLATTEN)
    set(RPATH_LIBRARY_INSTALL_DIR .)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

include(src/MaaUtils/MaaUtils.cmake)

# MAA_VERSION 由 MaaUtils.cmake 通过 add_compile_definitions 设置在 directory scope；
# MAA 这一侧用 include() 引入 MaaUtils.cmake（文本内联，不创建新 scope），该宏会
# 随之进入 MaaCore 等下游 TUs 的命令行——MAA_HASH_VERSION 每次变化时所有下游 TUs 命令行
# 随之变化，git bisect 跨 commit 时 ccache / ninja 增量构建全 miss。
# 这里直接过滤 directory 的 COMPILE_DEFINITIONS 把该宏从根 scope 移除，再单独以 PRIVATE
# 形式应用于 MaaUtils target——MaaUtils 内部 TUs 的 per-DLL 版本签名 by-design 行为
# 保持不变。
#
# 注 1：remove_definitions 是为老 API add_definitions 设计的，无法移除
# add_compile_definitions 加的条目，所以此处直接操作 directory property。
# 注 2：本 hook 仅处理 directory 级传播。若 MaaUtils 上游将 MAA_VERSION 改为
# target_compile_definitions PUBLIC/INTERFACE，传播会改经 link 继承通道、绕过本 hook；
# 届时需追加对 MaaUtils target INTERFACE_COMPILE_DEFINITIONS 的对称过滤。
get_property(_compile_defs DIRECTORY PROPERTY COMPILE_DEFINITIONS)
list(FILTER _compile_defs EXCLUDE REGEX "^MAA_VERSION=")
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS "${_compile_defs}")
target_compile_definitions(MaaUtils PRIVATE MAA_VERSION="${MAA_HASH_VERSION}")

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
    add_subdirectory(src/MaaUpdater)

    include_external_msproject(MaaWpfGui ${PROJECT_SOURCE_DIR}/src/MaaWpfGui/MaaWpfGui.csproj)

    add_dependencies(MaaWpfGui MaaCore)
    add_dependencies(MaaWpfGui MAA.Updater)
    if(DEFINED ENV{VSCODE_PID})
        add_custom_target(run-MaaWpfGui
            COMMAND "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/MAA.exe"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        )
        add_dependencies(run-MaaWpfGui MaaCore MaaWpfGui)
    endif()
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
