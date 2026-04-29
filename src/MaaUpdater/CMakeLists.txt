add_executable(MAA.Updater main.cpp)
set_target_properties(MAA.Updater PROPERTIES OUTPUT_NAME "MAA.Updater")

# Static CRT - no runtime DLL dependency, updater must survive MAA's own runtime being replaced
set_property(TARGET MAA.Updater PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options(MAA.Updater PRIVATE /WX- /W3 /utf-8)
target_compile_definitions(MAA.Updater PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# kernel32: process/file/directory APIs
# user32:   MessageBoxW and Win32 windowing APIs
# comctl32: progress bar common control
# shell32:  recycle-bin delete via SHFileOperationW
# advapi32: system theme detection via registry
# dwmapi:   immersive dark title bar attribute
target_link_libraries(MAA.Updater PRIVATE kernel32 user32 comctl32 shell32 advapi32 dwmapi)

install(TARGETS MAA.Updater RUNTIME DESTINATION .)
