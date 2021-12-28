# Linux 编译教程

**本教程需要读者有一定的 Linux 环境配置能力及编程基础！**

虽然没想明白为什么 Linux 下需要用助手挂模拟器，嘛总之大家有这个需求还是弄一下_(:з」∠)_

作者是 Linux 小白，所以虽说是教程，也只是分享一下自己的踩坑经历，如果遇到其他问题欢迎提出 ISSUE 一起讨论下 orz

## 下载编译第三方库

### Opencv

请自行搜索教程安装，没什么特别的，作者当前成功验证过的版本为`4.5.3`版本，其他版本应该也没什么问题。仅`Opencv`本体即可，不需要额外安装`opencv_contrib`。

### PaddleOCR

1. 使用我魔改了接口的版本：https://github.com/MistEO/PaddleOCR
2. 参考 [这个教程](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.3/deploy/cpp_infer#readme)
3. 额外依赖的 `PaddlePaddle` 直接 [下载](https://paddleinference.paddlepaddle.org.cn/master/user_guides/download_lib.html) 即可。其中 `avx` 和 `mkl` 两者都可以提高计算效率，建议选择。但如果 CPU 不支持，则只能选择 `noavx` 和 `openblas` 的兼容性版本（一般稍微新一点的 CPU 都支持两者了，AMD 的也没关系）。 `cuda` 和 `cudnn` 是 GPU 加速库，在 `PaddleOCR` 上可能比较麻烦，我没折腾过，有兴趣可以尝试下
4. 若 `cmake` 找不到 `Opencv` ，可尝试修改 [Opencv 的查找路径](https://github.com/MistEO/PaddleOCR/blob/release/2.3/deploy/cpp_infer/CMakeLists.txt#L49) 
5. 若您使用 `MKL` 版本，可修改 [MKL 宏定义](https://github.com/MistEO/PaddleOCR/blob/release/2.3/deploy/cpp_infer/include/ocr_defines.h#L9) 为 `true` 以提高性能

编译选项参考

```bash
cmake ../ -DPADDLE_LIB=/your_path/paddle_inference/ -D OPENCV_DIR=/your_path_to_opencv/ -DWITH_STATIC_LIB=OFF -DBUILD_SHARED=ON
# 若设备不支持 MKL, 可再额外添加 -DWITH_MKL=OFF 选项，并下载对应的 PaddlePaddle 预测库版本
```

### meojson

https://github.com/MistEO/meojson

```bash
make shared
```

### penguin-stats-recognize-v3

使用我魔改了接口的版本：https://github.com/MistEO/penguin-stats-recognize-v3

## MeoAssistant

1. 直接拷贝上面编译的第三方库到 `3rdparty/lib` 或者 手动修改 `CMakeLists.txt` 指定第三方库路径
2. `3rdparty/include/opencv` 中的头文件是 `4.5.3` 版本的，若是使用其他版本，请注意头文件冲突问题（直接将你的 `opencv` 头文件覆盖过去就好）
3. 安装 `adb`, 并修改 `resource/config.json` 中 `Custom`.`adb`.`path` 的值为 `"adb"`
4. 复制资源文件到 `libMeoAssitant.so` 同一目录下，具体可参考 [这个脚本](../../tools/update_resource.sh)
5. 通过 [Python 接口](../Python/interface.py) 或 [C 接口](../../include/AsstCaller.h) 进行调用，需要自行编写少量的代码
6. `cmake` 可通过添加 `-DBUILD_TEST=ON` 选项来编译一个测试小 demo

## 集成文档

[~~或许算不上文档~~](https://github.com/MistEO/MeoAssistantArknights/wiki)

### Python

可参考 [Python 接口](../Python/interface.py) 中 `__main__` 的实现

### C

可参考 [TestCaller](../../tools/TestCaller/main.cpp) 中的实现

### C sharp

可参考 [MeoAsstGui](../MeoAsstGui/Helper/AsstProxy.cs) 中的实现
