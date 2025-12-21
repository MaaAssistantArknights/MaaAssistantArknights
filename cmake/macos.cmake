if (BUILD_XCFRAMEWORK)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework)

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

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/MaaCore.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/xcframework/MaaCore.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaCore> 
            -headers ${PROJECT_SOURCE_DIR}/include 
            -output MaaCore.xcframework
        DEPENDS MaaCore
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating MaaCore.xcframework"
    )
    
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/MaaUtils.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/xcframework/MaaUtils.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaUtils> 
            -output MaaUtils.xcframework
        DEPENDS MaaUtils
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating MaaUtils.xcframework"
    )

    find_unique_library("OpenCV" "libopencv_world*.dylib" OPENCV_LIB)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/OpenCV.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/xcframework/OpenCV.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${OPENCV_LIB}" 
            -output OpenCV.xcframework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating OpenCV.xcframework"
    )

    find_unique_library("ONNXRuntime" "libonnxruntime*.dylib" ONNXRUNTIME_LIB)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/ONNXRuntime.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/xcframework/ONNXRuntime.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${ONNXRUNTIME_LIB}" 
            -output ONNXRuntime.xcframework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating ONNXRuntime.xcframework"
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/fastdeploy_ppocr.xcframework
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/xcframework/fastdeploy_ppocr.xcframework"
        COMMAND xcodebuild -create-xcframework 
            -library "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libfastdeploy_ppocr.dylib" 
            -output fastdeploy_ppocr.xcframework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating fastdeploy_ppocr.xcframework"
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS 
            MaaCore 
            ${CMAKE_BINARY_DIR}/xcframework/MaaCore.xcframework 
            MaaUtils 
            ${CMAKE_BINARY_DIR}/xcframework/MaaUtils.xcframework 
            ${CMAKE_BINARY_DIR}/xcframework/OpenCV.xcframework 
            ${CMAKE_BINARY_DIR}/xcframework/ONNXRuntime.xcframework 
            ${CMAKE_BINARY_DIR}/xcframework/fastdeploy_ppocr.xcframework
    )
endif (BUILD_XCFRAMEWORK)
