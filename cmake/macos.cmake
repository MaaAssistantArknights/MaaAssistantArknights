if (BUILD_XCFRAMEWORK)
    add_custom_command(OUTPUT MaaCore.xcframework
        COMMAND rm -rf MaaCore.xcframework
        COMMAND xcodebuild -create-xcframework -library libMaaCore.dylib -headers ${PROJECT_SOURCE_DIR}/include -output MaaCore.xcframework
        DEPENDS MaaCore
    )

    add_custom_command(OUTPUT OpenCV.xcframework
        COMMAND rm -rf OpenCV.xcframework
        COMMAND xcodebuild -create-xcframework -library "${PROJECT_SOURCE_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/libopencv_world4.407.dylib" -output OpenCV.xcframework
    )

    add_custom_command(OUTPUT ONNXRuntime.xcframework
        COMMAND rm -rf ONNXRuntime.xcframework
        COMMAND xcodebuild -create-xcframework -library "${PROJECT_SOURCE_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/libonnxruntime.1.14.1.dylib" -output ONNXRuntime.xcframework
    )

    add_custom_command(OUTPUT fastdeploy_ppocr.xcframework
        COMMAND rm -rf fastdeploy_ppocr.xcframework
        COMMAND xcodebuild -create-xcframework -library "${PROJECT_SOURCE_DIR}/MaaDeps/runtime/${MAADEPS_TRIPLET}/libfastdeploy_ppocr.dylib" -output fastdeploy_ppocr.xcframework
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS MaaCore MaaCore.xcframework OpenCV.xcframework ONNXRuntime.xcframework fastdeploy_ppocr.xcframework
    )
endif (BUILD_XCFRAMEWORK)

target_compile_options(MaaCore PRIVATE
    -Wno-deprecated-declarations
    -Wno-gnu-zero-variadic-macro-arguments)
