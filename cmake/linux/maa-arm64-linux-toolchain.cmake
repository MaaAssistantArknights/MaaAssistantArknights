set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(XTOOLS_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../MaaDeps/x-tools/aarch64-linux-gnu)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_C_COMPILER clang-20)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_CXX_COMPILER clang++-20)
set(CMAKE_ASM_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_ASM_COMPILER clang-20)
set(CMAKE_SYSROOT ${XTOOLS_ROOT}/aarch64-linux-gnu/sysroot)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -stdlib=libc++ --sysroot=${CMAKE_SYSROOT}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -stdlib=libc++ --sysroot=${CMAKE_SYSROOT}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC -stdlib=libc++ --sysroot=${CMAKE_SYSROOT} -fuse-ld=lld --gcc-toolchain=${XTOOLS_ROOT}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fPIC -stdlib=libc++ --sysroot=${CMAKE_SYSROOT} -fuse-ld=lld --gcc-toolchain=${XTOOLS_ROOT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -stdlib=libc++ --sysroot=${CMAKE_SYSROOT} -fuse-ld=lld --gcc-toolchain=${XTOOLS_ROOT}")
set(CMAKE_LINKER_TYPE LLD)
