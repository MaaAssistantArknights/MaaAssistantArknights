cmake_minimum_required(VERSION 3.14)
project(MaaCoreForwarder C)

add_library(MaaCore_INTERFACE INTERFACE)
set_target_properties(MaaCore_INTERFACE PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../../../include")

# fool cmake to add MaaCore.spec to linker command line
add_library(MaaCore_SPEC IMPORTED STATIC)
set_target_properties(MaaCore_SPEC PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/MaaCore.spec")

add_library(MaaCoreForwarder SHARED 
    maacore.c
    dl_maacore.c
    entry.c
    task_queue.c
    task_queue_wine.c
    MaaCore.spec
)

set_target_properties(MaaCoreForwarder PROPERTIES
    NO_SONAME TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    BUILD_RPATH_USE_ORIGIN TRUE  # FIXME: not working
    INSTALL_RPATH "$ORIGIN"
    PREFIX ""
    OUTPUT_NAME "MaaCore"
    SUFFIX ".dll"
)
target_link_options(MaaCoreForwarder PRIVATE -Wl,--wine-builtin)
target_link_libraries(MaaCoreForwarder MaaCore_INTERFACE MaaCore_SPEC)

# winegcc will force add .so suffix
add_custom_command(TARGET MaaCoreForwarder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "$<TARGET_FILE:MaaCoreForwarder>.so" "$<TARGET_FILE:MaaCoreForwarder>"
    COMMENT "[Winelib hack] Rename $<TARGET_FILE_NAME:MaaCoreForwarder>.so to $<TARGET_FILE_NAME:MaaCoreForwarder>"
)

install(TARGETS MaaCoreForwarder LIBRARY DESTINATION ".")

# or a good old fake module? (FIXME: not working)
# add_library(MaaCoreForwarder_FAKE SHARED dummy.c)
# set_target_properties(MaaCoreForwarder_FAKE PROPERTIES PREFIX "" OUTPUT_NAME "MaaCore" SUFFIX ".dll" NO_SONAME TRUE)
# target_link_options(MaaCoreForwarder_FAKE PRIVATE -Wb,--fake-module)
