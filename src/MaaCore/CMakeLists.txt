file(GLOB_RECURSE maa_src *.h *.hpp *.cpp)

add_library(MaaCore SHARED ${maa_src})

if(WITH_EMULATOR_EXTRAS)
    if(EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/EmulatorExtras/Mumu)
        target_include_directories(MaaCore PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/EmulatorExtras)
    else()
        message(WARNING "EmulatorExtras not found, please run `git submodule update --init 3rdparty/EmulatorExtras`")
        set(WITH_EMULATOR_EXTRAS OFF)
    endif()
endif()

target_compile_definitions(MaaCore PRIVATE ASST_WITH_EMULATOR_EXTRAS=$<BOOL:${WITH_EMULATOR_EXTRAS}>)

target_include_directories(MaaCore PUBLIC ${PROJECT_SOURCE_DIR}/include PRIVATE .)
add_dependencies(MaaCore MaaUtils)
target_link_libraries(MaaCore HeaderOnlyLibraries MaaUtils ${OpenCV_LIBS} fastdeploy_ppocr ONNXRuntime::ONNXRuntime ZLIB::ZLIB Boost::system)

if(WIN32)
    target_link_libraries(MaaCore ws2_32)
endif()

if(LINUX)
    target_link_libraries(MaaCore pthread dl)
endif()

file(GLOB_RECURSE MaaCore_PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/*.h)
target_sources(MaaCore PUBLIC ${MaaCore_PUBLIC_HEADERS})
set_target_properties(MaaCore PROPERTIES PUBLIC_HEADER "${MaaCore_PUBLIC_HEADERS}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_src})

# 收集resource/tasks目录下的所有文件作为资源
file(GLOB_RECURSE TASKS_RESOURCE_FILES 
    "${PROJECT_SOURCE_DIR}/resource/tasks/*"
)
file(GLOB_RECURSE GLOBAL_TASKS_RESOURCE_FILES 
    "${PROJECT_SOURCE_DIR}/resource/global/*/resource/tasks/*"
)

# 将资源文件添加到MaaCore目标，但不参与编译
target_sources(MaaCore PRIVATE ${TASKS_RESOURCE_FILES})
target_sources(MaaCore PRIVATE ${GLOBAL_TASKS_RESOURCE_FILES})

# 使用source_group创建正确的文件夹结构
source_group(TREE "${PROJECT_SOURCE_DIR}/resource/tasks" 
    PREFIX "Resource Files/tasks/CN" 
    FILES ${TASKS_RESOURCE_FILES}
)

# 为GLOBAL_TASKS_RESOURCE_FILES创建特殊的文件夹结构
foreach(GLOBAL_FILE ${GLOBAL_TASKS_RESOURCE_FILES})
    # 从完整路径中提取A部分：${PROJECT_SOURCE_DIR}/resource/global/A/resource/tasks/xxx
    string(REGEX REPLACE "${PROJECT_SOURCE_DIR}/resource/global/([^/]+)/resource/tasks/.*" "\\1" FOLDER_NAME "${GLOBAL_FILE}")
    
    # 获取相对于resource/tasks的文件路径部分
    string(REGEX REPLACE "${PROJECT_SOURCE_DIR}/resource/global/[^/]+/resource/tasks/(.*)" "\\1" REL_FILE_PATH "${GLOBAL_FILE}")
    
    # 获取文件的目录部分
    get_filename_component(FILE_DIR "${REL_FILE_PATH}" DIRECTORY)
    
    # 构建VS中的显示路径
    if(FILE_DIR)
        set(VS_GROUP "Resource Files/tasks/${FOLDER_NAME}/${FILE_DIR}")
    else()
        set(VS_GROUP "Resource Files/tasks/${FOLDER_NAME}")
    endif()
    
    # 设置单个文件的source_group
    source_group("${VS_GROUP}" FILES "${GLOBAL_FILE}")
endforeach()

if(WIN32)
    # 注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    target_compile_definitions(MaaCore PRIVATE ASST_DLL_EXPORTS _UNICODE UNICODE)
endif()

if(APPLE)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "@loader_path/")
elseif(UNIX)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif()

maadeps_install(.)
install(TARGETS MaaUtils ${MaaCore_install_flatten_args})
install(TARGETS MaaCore ${MaaCore_install_flatten_args})

if(WIN32)
    add_custom_command(
        TARGET MaaCore
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
        $<TARGET_FILE_DIR:MaaCore>
        COMMAND_EXPAND_LISTS)
endif()

if(ANDROID)
    if(CMAKE_STRIP)
        add_custom_command(
            TARGET MaaCore
            POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:MaaCore>
            COMMENT "Stripping MaaCore for Android")
    else()
        message(WARNING "CMAKE_STRIP not found, Android library will not be stripped")
    endif()
endif()
