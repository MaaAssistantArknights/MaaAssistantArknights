include(FetchContent)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(
  adbLite
  GIT_REPOSITORY https://github.com/hguandl/adb-lite.git
  GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(adbLite)
