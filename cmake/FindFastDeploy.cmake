# TODO: replace with cmake-generated interface file

find_path(FastDeploy_INCLUDE_DIR NAMES fastdeploy/fastdeploy_model.h)

find_library(FastDeploy_LIBRARY NAMES fastdeploy)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    FastDeploy
    REQUIRED_VARS FastDeploy_LIBRARY FastDeploy_INCLUDE_DIR
)

if(FastDeploy_FOUND)
    set(FastDeploy_INCLUDE_DIRS ${FastDeploy_INCLUDE_DIR})
    if(NOT TARGET FastDeploy::FastDeploy)
        add_library(FastDeploy::FastDeploy UNKNOWN IMPORTED)
        set_target_properties(FastDeploy::FastDeploy PROPERTIES
            IMPORTED_LOCATION "${FastDeploy_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FastDeploy_INCLUDE_DIR}"
        )
    endif()
endif()
