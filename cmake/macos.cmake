if (BUILD_XCFRAMEWORK)
    add_custom_command(OUTPUT MaaCore.xcframework
        COMMAND rm -rf MaaCore.xcframework
        COMMAND xcodebuild -create-xcframework -library libMaaCore.dylib -headers ${PROJECT_SOURCE_DIR}/include -output MaaCore.xcframework
        DEPENDS MaaCore
    )

    add_custom_command(OUTPUT OpenCV.xcframework
        COMMAND rm -rf OpenCV.xcframework
        COMMAND rm -f libopencv_world.4.6.dylib
        COMMAND cp ${OpenCV_INSTALL_PATH}/lib/libopencv_world4.4.6.0.dylib libopencv_world.4.6.dylib
        COMMAND xcodebuild -create-xcframework -library libopencv_world.4.6.dylib -output OpenCV.xcframework
    )

    add_custom_command(OUTPUT ONNXRuntime.xcframework
        COMMAND rm -rf ONNXRuntime.xcframework
        COMMAND xcodebuild -create-xcframework -library "${PROJECT_SOURCE_DIR}/MaaDeps/vcpkg/installed/${MAADEPS_TRIPLET}/lib/libonnxruntime.1.12.1.dylib" -output ONNXRuntime.xcframework
    )

    add_custom_command(OUTPUT MaaDerpLearning.xcframework
        COMMAND rm -rf FastDeploy.xcframework
        COMMAND xcodebuild -create-xcframework -library "${PROJECT_SOURCE_DIR}/MaaDeps/vcpkg/installed/${MAADEPS_TRIPLET}/lib/libMaaDerpLearning.dylib" -output MaaDerpLearning.xcframework
    )

    add_custom_target(MaaXCFramework ALL
        DEPENDS MaaCore MaaCore.xcframework OpenCV.xcframework MaaDerpLearning.xcframework
    )
endif (BUILD_XCFRAMEWORK)

target_compile_options(MaaCore PRIVATE
    -Wno-deprecated-declarations
    -Wno-gnu-zero-variadic-macro-arguments)
