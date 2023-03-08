find_path(ONNXRuntime_INCLUDE_DIR NAMES onnxruntime/core/session/onnxruntime_c_api.h)

find_library(ONNXRuntime_LIBRARY NAMES onnxruntime)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ONNXRuntime
    REQUIRED_VARS ONNXRuntime_LIBRARY ONNXRuntime_INCLUDE_DIR
)

if(ONNXRuntime_FOUND)
    set(ONNXRuntime_INCLUDE_DIRS ${ONNXRuntime_INCLUDE_DIR})
    if(NOT TARGET ONNXRuntime::ONNXRuntime)
        add_library(ONNXRuntime::ONNXRuntime UNKNOWN IMPORTED)
        set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
            IMPORTED_LOCATION "${ONNXRuntime_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ONNXRuntime_INCLUDE_DIR}"
        )
    endif()
endif()