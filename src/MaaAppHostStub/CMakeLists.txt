# 打包时用它覆盖 .NET SDK 生成的 apphost（MAA.exe）：
# 安装不完整或处于临时目录（如用户把 MAA.exe 单独拖出文件夹、压缩包内直接双击）时弹提示，
# 否则经 hostfxr_main_startupinfo 转发启动 MAA.dll

if(NOT DEFINED MAA_HASH_VERSION)
    set(MAA_HASH_VERSION "0.0.1")
endif()

if(MAA_HASH_VERSION MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(maa_apphost_fileversion "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
    set(maa_apphost_version_string "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
    set(maa_apphost_fileversion "0,0,1,0")
    set(maa_apphost_version_string "0.0.1")
endif()

configure_file(Version.h.in Version.h @ONLY)

add_executable(MaaAppHostStub WIN32 MaaAppHostStub.cpp MaaAppHostStub.rc)
target_include_directories(MaaAppHostStub PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(MSVC)
    # 显式 /utf-8，不依赖源文件 BOM（与 src/MaaUpdater 对齐）
    target_compile_options(MaaAppHostStub PRIVATE /utf-8)
    # 清单已由 rc 以 RT_MANIFEST 嵌入（app.manifest），关闭链接器默认清单避免冲突
    target_link_options(MaaAppHostStub PRIVATE /MANIFEST:NO)
endif()

# 静态链接 CRT：用户机器可能未安装 VC 运行库，stub 必须能独立启动
set_target_properties(MaaAppHostStub PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
