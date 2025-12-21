if (BUILD_XCFRAMEWORK)
    set(XCFRAMEWORK_DIR "${CMAKE_BINARY_DIR}/xcframework")
    file(MAKE_DIRECTORY ${XCFRAMEWORK_DIR})

    # Macro to find a unique library file
    macro(find_unique_library lib_name glob_pattern output_var)
        file(GLOB _libs CONFIGURE_DEPENDS "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/${glob_pattern}")
        if(_libs)
            list(LENGTH _libs _cnt)
            if(_cnt EQUAL 1)
                list(GET _libs 0 ${output_var})
                message(STATUS "Found ${lib_name}: ${${output_var}}")
            else()
                message(FATAL_ERROR "Ambiguous ${lib_name} dylibs: ${_libs}")
            endif()
        else()
            message(FATAL_ERROR "${lib_name} library not found in ${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/")
        endif()
    endmacro()

    add_custom_command(OUTPUT ${XCFRAMEWORK_DIR}/MaaCore.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${XCFRAMEWORK_DIR}/MaaCore.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaCore> 
            -headers ${PROJECT_SOURCE_DIR}/include 
            -output MaaCore.xcframework
        DEPENDS MaaCore
        WORKING_DIRECTORY ${XCFRAMEWORK_DIR}
        COMMENT "Generating MaaCore.xcframework"
    )
    
    add_custom_command(OUTPUT ${XCFRAMEWORK_DIR}/MaaUtils.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${XCFRAMEWORK_DIR}/MaaUtils.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaUtils> 
            -output MaaUtils.xcframework
        DEPENDS MaaUtils
        WORKING_DIRECTORY ${XCFRAMEWORK_DIR}
        COMMENT "Generating MaaUtils.xcframework"
    )

    find_unique_library("OpenCV" "libopencv_world*.dylib" OPENCV_LIB)
    add_custom_command(OUTPUT ${XCFRAMEWORK_DIR}/OpenCV.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${XCFRAMEWORK_DIR}/OpenCV.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${OPENCV_LIB}" 
            -output OpenCV.xcframework
        WORKING_DIRECTORY ${XCFRAMEWORK_DIR}
        COMMENT "Generating OpenCV.xcframework"
    )

    find_unique_library("ONNXRuntime" "libonnxruntime*.dylib" ONNXRUNTIME_LIB)
    add_custom_command(OUTPUT ${XCFRAMEWORK_DIR}/ONNXRuntime.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${XCFRAMEWORK_DIR}/ONNXRuntime.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${ONNXRUNTIME_LIB}" 
            -output ONNXRuntime.xcframework
        WORKING_DIRECTORY ${XCFRAMEWORK_DIR}
        COMMENT "Generating ONNXRuntime.xcframework"
    )

    add_custom_command(OUTPUT ${XCFRAMEWORK_DIR}/fastdeploy_ppocr.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${XCFRAMEWORK_DIR}/fastdeploy_ppocr.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libfastdeploy_ppocr.dylib" 
            -output fastdeploy_ppocr.xcframework
        WORKING_DIRECTORY ${XCFRAMEWORK_DIR}
        COMMENT "Generating fastdeploy_ppocr.xcframework"
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS 
            MaaCore 
            ${XCFRAMEWORK_DIR}/MaaCore.xcframework 
            MaaUtils 
            ${XCFRAMEWORK_DIR}/MaaUtils.xcframework 
            ${XCFRAMEWORK_DIR}/OpenCV.xcframework 
            ${XCFRAMEWORK_DIR}/ONNXRuntime.xcframework 
            ${XCFRAMEWORK_DIR}/fastdeploy_ppocr.xcframework
    )
endif (BUILD_XCFRAMEWORK)
