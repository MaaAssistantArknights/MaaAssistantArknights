cmake_minimum_required(VERSION 2.8)
project(callback)

add_library(Callback SHARED callback.c dart_api_dl.c)

target_include_directories(Callback PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/internal"
)
