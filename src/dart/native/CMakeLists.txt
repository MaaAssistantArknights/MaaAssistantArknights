cmake_minimum_required(VERSION 2.8)
project(MeoAssistantArknightsDart)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/MP>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/W4>")
add_compile_options("$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>")
add_library(MeoAssistantDart SHARED src/AsstDartApi.cpp) 
set(CMAKE_CXX_STANDARD 17)
set(MAA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")


include_directories("include" "${MAA_ROOT}/include" "dart" "dart/internal")
find_library(MeoAssistant PATHS "${CMAKE_CURRENT_SOURCE}/lib")


target_link_libraries(MeoAssistantDart PUBLIC ${MeoAssitant})
install(TARGETS MeoAssistantDart DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../linux")
install(FILES lib/libMeoAssistant.so DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../linux")
