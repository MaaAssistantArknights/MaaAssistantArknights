if (BUILD_XCFRAMEWORK)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework)

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/MaaCore.xcframework
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/xcframework/MaaCore.xcframework
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaCore> 
            -headers ${PROJECT_SOURCE_DIR}/include 
            -output MaaCore.xcframework
        DEPENDS MaaCore
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating MaaCore.xcframework"
    )
    
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/MaaUtils.xcframework
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/xcframework/MaaUtils.xcframework
        COMMAND xcodebuild -create-xcframework 
            -library $<TARGET_FILE:MaaUtils> 
            -output MaaUtils.xcframework
        DEPENDS MaaUtils
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating MaaUtils.xcframework"
    )

    file(GLOB OPENCV_LIBS "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libopencv_world*.dylib")
    if(OPENCV_LIBS)
        list(LENGTH OPENCV_LIBS _cnt)
        if(_cnt EQUAL 1)
            list(GET OPENCV_LIBS 0 OPENCV_LIB)
            message(STATUS "Found OpenCV: ${OPENCV_LIB}")
        else()
            message(FATAL_ERROR "Ambiguous OpenCV dylibs: ${OPENCV_LIBS}")
        endif()
    else()
        message(FATAL_ERROR "OpenCV library not found in ${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/")
    endif()
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/OpenCV.xcframework
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/xcframework/OpenCV.xcframework
        COMMAND xcodebuild -create-xcframework 
            -library "${OPENCV_LIB}" 
            -output OpenCV.xcframework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating OpenCV.xcframework"
    )

    file(GLOB ONNXRUNTIME_LIBS "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libonnxruntime*.dylib")
    if(ONNXRUNTIME_LIBS)
        list(LENGTH ONNXRUNTIME_LIBS _cnt)
        if(_cnt EQUAL 1)
            list(GET ONNXRUNTIME_LIBS 0 ONNXRUNTIME_LIB)
            message(STATUS "Found ONNXRuntime: ${ONNXRUNTIME_LIB}")
        else()
            message(FATAL_ERROR "Ambiguous ONNXRuntime dylibs: ${ONNXRUNTIME_LIBS}")
        endif()
    else()
        message(FATAL_ERROR "ONNXRuntime library not found in ${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/")
    endif()
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/ONNXRuntime.xcframework
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/xcframework/ONNXRuntime.xcframework
        COMMAND xcodebuild -create-xcframework 
            -library "${ONNXRUNTIME_LIB}" 
            -output ONNXRuntime.xcframework
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/xcframework
        COMMENT "Generating ONNXRuntime.xcframework"
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/xcframework/fastdeploy_ppocr.xcframework
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/xcframework/fastdeploy_ppocr.xcframework
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
