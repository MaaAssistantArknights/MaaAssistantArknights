# 打包时用它覆盖 .NET SDK 生成的 apphost（MAA.exe）：
# MAA.dll 不在旁边（如用户把 MAA.exe 单独拖出文件夹）时弹提示，否则转发 hostfxr_main

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

# 静态链接 CRT：用户机器可能未安装 VC 运行库，stub 必须能独立启动
set_target_properties(MaaAppHostStub PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
