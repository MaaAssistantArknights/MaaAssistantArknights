file(GLOB_RECURSE maa_src *.h *.hpp *.cpp)

add_library(MaaCore SHARED ${maa_src})

if(WITH_EMULATOR_EXTRAS AND NOT EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/EmulatorExtras/Mumu)
    target_include_directories(EmulatorExtras INTERFACE ${PROJECT_SOURCE_DIR}/3rdparty/EmulatorExtras)
    target_link_libraries(MaaCore PRIVATE EmulatorExtras)
    message(WARNING "EmulatorExtras not found, please run `git submodule update --init 3rdparty/EmulatorExtras`")
    set(WITH_EMULATOR_EXTRAS OFF)
endif()
target_compile_definitions(MaaCore PRIVATE ASST_WITH_EMULATOR_EXTRAS=$<BOOL:${WITH_EMULATOR_EXTRAS}>)

target_include_directories(MaaCore PUBLIC ${PROJECT_SOURCE_DIR}/include PRIVATE .)
target_link_libraries(MaaCore HeaderOnlyLibraries ${OpenCV_LIBS} fastdeploy_ppocr ONNXRuntime::ONNXRuntime ZLIB::ZLIB Boost::system)
if(WIN32)
    target_link_libraries(MaaCore ws2_32)
endif()
if(LINUX)
    target_link_libraries(MaaCore pthread)
endif()

file(GLOB_RECURSE MaaCore_PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/*.h)
target_sources(MaaCore PUBLIC ${MaaCore_PUBLIC_HEADERS})
set_target_properties(MaaCore PROPERTIES PUBLIC_HEADER "${MaaCore_PUBLIC_HEADERS}")

if(WIN32)
    add_custom_command(
        TARGET MaaCore
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${PROJECT_SOURCE_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
        $<TARGET_FILE_DIR:MaaCore>
        COMMAND_EXPAND_LISTS)
endif()


source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_src})

if (WIN32)
    #注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    target_compile_definitions(MaaCore PRIVATE ASST_DLL_EXPORTS _UNICODE UNICODE)
endif ()

if(APPLE)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "@loader_path/")
elseif(UNIX)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif()

if(INSTALL_FLATTEN)
    set(MaaCore_install_flatten_args RUNTIME DESTINATION . LIBRARY DESTINATION . PUBLIC_HEADER DESTINATION .)
endif()

install(TARGETS MaaCore ${MaaCore_install_flatten_args})
maadeps_install(.)
