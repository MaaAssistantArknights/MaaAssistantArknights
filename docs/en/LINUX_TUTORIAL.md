# Linux Compilation Tutorial

**The tutorial requires some basic knowledge about Linux OS!**

Though it is unclear why it needs to run in Linux... Anyway in case somebody needs it_(:з」∠)_

If you meet any other problems, please feel free to submit issue to us.

## Download and Compile 3rd-party Libraries

### Opencv

Please search the tutorial and install. Nothing to mention. The version `4.5.3` is confirmed to be able to use. Other versions should be OK as well, but `4.5.3` is recommended to keep it the same as the project and avoid some unnecessary problems. Installing `Opencv` itself is enough. `opencv_contrib` is not required.

### PaddleOCR

1. Use a hacked version: https://github.com/MistEO/PaddleOCR.
2. Refer to [this tutorial](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.3/deploy/cpp_infer#readme).
3. [Download](https://paddleinference.paddlepaddle.org.cn/master/user_guides/download_lib.html) extra dependencies for `PaddlePaddle`. Both `avx` and `mkl` are recommended, which can improve the efficiency of calculation. However, if your CPU does not support them, you can choose compatible version of either `noavx` or `openblas` (new CPUs should support them, even AMD). `cuda` and `cudnn` are GPU libraries, but I haven't tried it on `PaddleOCR` yet. Feel free to try it out.
4. If `cmake` cannot find `Opencv`, you can try editing [the finding path of Opencv](https://github.com/MistEO/PaddleOCR/blob/release/2.3/deploy/cpp_infer/CMakeLists.txt#L49).
5. If you are using `MKL` version, you can change the [MKL Macro](https://github.com/MistEO/PaddleOCR/blob/release/2.3/deploy/cpp_infer/include/ocr_defines.h#L9) to `true` to improve performance.

Compilation flags for your reference

```bash
cmake ../ -DPADDLE_LIB=/your_path/paddle_inference/ -DOpenCV_DIR=/your_path_to_opencv/ -DWITH_STATIC_LIB=OFF -DBUILD_SHARED=ON
# If your device does not support MKL, you can add `-DWITH_MKL=OFF` flag, and download corresponding PaddlePaddle version.
```

### zlib

In Ubuntu:

```bash
sudo apt update && sudo apt install zlib1g-dev
sudo ldconfig
```

If zlib does not exist in other distribution, you can try compiling from [source](https://github.com/madler/zlib).

## MeoAssistant

1. Copy the compiled 3rd-party library to `3rdparty/lib` or change `CMakeLists.txt` to indicate 3rd-party library path manually.
2. The header files in `3rdparty/include/opencv` is version `4.5.3`. If you are using other versions, please notice there may be conflicts in header files (you can simply replace them with your `opencv` header files).
3. Install `adb`, and change `resource/config.json` - `Custom.adb.path` to `"adb"`
4. Copy resource files to the same folder of `libMeoAssitant.so`.

```sh
cd tools
sh ./update_resource.sh <YourBuildDir>
```

5. Call by [Python interface](../src/Python/interface.py) or [C interface](../include/AsstCaller.h), requiring you to write some code.
6. `cmake` can compile a demo for testing by adding `-DBUILD_TEST=ON` flag

## Integration Documentation

[~~Maybe not a doc~~](https://github.com/MistEO/MeoAssistantArknights/wiki)

### Python

Refer to the implementation of `__main__` in [Python demo](../src/Python/sample.py)

### C

Refer to the implementation of [TestCaller](../tools/TestCaller/main.cpp)

### C sharp

Refer to the implementation of [MeoAsstGui](../src/MeoAsstGui/Helper/AsstProxy.cs)
