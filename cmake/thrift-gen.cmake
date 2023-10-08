macro(GENERATE_THRIFT_LIB LIB_NAME FILENAME OUTPUTDIR SOURCES)
    file(MAKE_DIRECTORY ${OUTPUTDIR})
    detect_host_triplet(HOST_TRIPLET)
    if (CMAKE_CROSSCOMPILING)
      if (CMAKE_HOST_WIN32)
        set(_host_executable_suffix ".exe")
      else()
        set(_host_executable_suffix "")
      endif()
    else()
      set(_host_executable_suffix ${CMAKE_EXECUTABLE_SUFFIX})
    endif()
    if(EXISTS ${PROJECT_SOURCE_DIR}/MaaDeps/vcpkg/installed/maa-${HOST_TRIPLET}/tools/thrift/thrift${_host_executable_suffix})
        set(THRIFT_COMPILER ${PROJECT_SOURCE_DIR}/MaaDeps/vcpkg/installed/maa-${HOST_TRIPLET}/tools/thrift/thrift${_host_executable_suffix})
    else()
        find_program(THRIFT_COMPILER thrift)
    endif()
    if(NOT THRIFT_COMPILER)
        message(FATAL_ERROR "Thrift compiler not found")
    endif()
    get_filename_component(THRIFT_IDL_NAME ${FILENAME} NAME_WE)
    set(THRIFT_IDL_TARGET "${LIB_NAME}_${THRIFT_IDL_NAME}_idl")
    set("${THRIFT_IDL_NAME}-gen-cpp"
        ${OUTPUTDIR}/${THRIFT_IDL_NAME}.cpp
        ${OUTPUTDIR}/${THRIFT_IDL_NAME}.h
        ${OUTPUTDIR}/${THRIFT_IDL_NAME}_types.cpp
        ${OUTPUTDIR}/${THRIFT_IDL_NAME}_types.h)

    add_custom_command(OUTPUT ${${THRIFT_IDL_NAME}-gen-cpp}
                       DEPENDS ${FILENAME}
                       COMMAND ${THRIFT_COMPILER} --gen cpp:no_skeleton -out ${OUTPUTDIR} ${FILENAME}
                       VERBATIM)
    add_custom_target(${THRIFT_IDL_TARGET} DEPENDS ${${THRIFT_IDL_NAME}-gen-cpp})
    add_library(${LIB_NAME} STATIC ${${THRIFT_IDL_NAME}-gen-cpp})
    add_dependencies(${LIB_NAME} ${THRIFT_IDL_TARGET})
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${LIB_NAME} PUBLIC thrift::thrift)
    target_include_directories(${LIB_NAME} PUBLIC ${OUTPUTDIR})
    set(${SOURCES} ${${SOURCES}} ${GENERATED_SOURCES} PARENT_SCOPE)
endmacro(GENERATE_THRIFT_LIB)

if (WITH_THRIFT)
    add_compile_definitions(WITH_THRIFT)
endif (WITH_THRIFT)
