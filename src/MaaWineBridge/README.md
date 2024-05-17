# MAA Wine Bridge

## Getting Started

```console
$ cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=winegcc.cmake
$ cmake --build build
$ cmake --install build --prefix build
```

Place `build/MaaCore.dll` next to `MAA.exe`. When is gets loaded by Wine, it will load `libMaaCore.so` from the same directory and forward calls to it.
