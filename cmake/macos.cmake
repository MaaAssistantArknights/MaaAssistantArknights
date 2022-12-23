if (BUILD_XCFRAMEWORK)
    add_custom_command(OUTPUT MaaCore.xcframework
        COMMAND rm -rf MaaCore.xcframework
        COMMAND xcodebuild -create-xcframework -library libMaaCore.dylib -headers ${PROJECT_SOURCE_DIR}/include -output MaaCore.xcframework
        DEPENDS MaaCore
    )

    add_custom_command(OUTPUT OpenCV.xcframework
        COMMAND rm -rf OpenCV.xcframework
        COMMAND rm -f libopencv_world.4.5.dylib
        COMMAND cp ${OPENCV_DIRECTORY}/lib/libopencv_world.4.5.3.dylib libopencv_world.4.5.dylib
        COMMAND xcodebuild -create-xcframework -library libopencv_world.4.5.dylib -output OpenCV.xcframework
    )

    add_custom_command(OUTPUT ONNXRuntime.xcframework
        COMMAND rm -rf ONNXRuntime.xcframework
        COMMAND xcodebuild -create-xcframework -library ${FastDeploy_DIR}/third_libs/install/onnxruntime/lib/libonnxruntime.1.12.0.dylib -output ONNXRuntime.xcframework
    )

    add_custom_command(OUTPUT Paddle2ONNX.xcframework
        COMMAND rm -rf Paddle2ONNX.xcframework
        COMMAND xcodebuild -create-xcframework -library ${FastDeploy_DIR}/third_libs/install/paddle2onnx/lib/libpaddle2onnx.1.0.5.dylib -output Paddle2ONNX.xcframework
    )

    add_custom_command(OUTPUT FastDeploy.xcframework
        COMMAND rm -rf FastDeploy.xcframework
        COMMAND xcodebuild -create-xcframework -library ${FastDeploy_DIR}/lib/libfastdeploy.0.0.0.dylib -output FastDeploy.xcframework
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS MaaCore Paddle2ONNX.xcframework MaaCore.xcframework OpenCV.xcframework ONNXRuntime.xcframework FastDeploy.xcframework
    )
endif (BUILD_XCFRAMEWORK)

target_compile_options(MaaCore PRIVATE
    -Wno-deprecated-declarations
    -Wno-gnu-zero-variadic-macro-arguments)
