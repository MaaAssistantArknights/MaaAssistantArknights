function(download_and_decompress url filename sha256_checksum decompress_dir)
    if(EXISTS ${filename})
        file(SHA256 ${filename} CHECKSUM_VARIABLE)
    endif()

    if(NOT EXISTS ${filename} OR NOT CHECKSUM_VARIABLE STREQUAL sha256_checksum)
        message("Downloading file from ${url} to ${filename} ...")
        file(
            DOWNLOAD ${url} "${filename}.tmp"
            SHOW_PROGRESS
            EXPECTED_HASH SHA256=${sha256_checksum})
        file(RENAME "${filename}.tmp" ${filename})
    endif()

    if(NOT EXISTS ${decompress_dir})
        file(MAKE_DIRECTORY ${decompress_dir})
    endif()

    message("Decompress file ${filename} ...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${filename} WORKING_DIRECTORY ${decompress_dir})
endfunction()

function(get_osx_architecture)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(CURRENT_OSX_ARCH
            "arm64"
            PARENT_SCOPE)
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(CURRENT_OSX_ARCH
            "x86_64"
            PARENT_SCOPE)
    else()
        set(CURRENT_OSX_ARCH
            ${CMAKE_HOST_SYSTEM_PROCESSOR}
            PARENT_SCOPE)
    endif()
endfunction()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 13.3) # for to_chars
    get_osx_architecture()
endif(APPLE)

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
