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
        # UNIX/Linux/macOS 使用符号链接
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
