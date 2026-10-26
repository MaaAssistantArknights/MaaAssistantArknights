# Keep native PE metadata consistent with the WPF assembly and SignPath restrictions.
function(maa_add_windows_version target original_filename file_type)
    if(NOT WIN32)
        return()
    endif()

    set(maa_product_version "${MAA_HASH_VERSION}")
    if(maa_product_version MATCHES "([0-9]+)[.]([0-9]+)[.]([0-9]+)")
        set(maa_file_version "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
        set(maa_file_version_string "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.0")
    else()
        set(maa_product_version "0.0.1")
        set(maa_file_version "0,0,1,0")
        set(maa_file_version_string "0.0.1.0")
    endif()
    string(TIMESTAMP maa_copyright_year "%Y" UTC)
    get_filename_component(maa_file_description "${original_filename}" NAME_WLE)
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/windows-version.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${target}-version.rc"
        @ONLY)
    target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}-version.rc")
endfunction()
