# Linux 编译教程

**本教程需要读者有一定的 Linux 环境配置能力及编程基础！**

虽然没想明白为什么 Linux 下需要用助手挂模拟器，嘛总之大家有这个需求还是弄一下_(:з」∠)_

作者是 Linux 小白，所以虽说是教程，也只是分享一下自己的踩坑经历，如果遇到其他问题欢迎提出 ISSUE 一起讨论下 orz

## 下载编译第三方库

### Opencv

请自行搜索教程安装，没什么特别的，作者当前成功验证过的版本为`4.5.3`版本，不需要`opencv_contrib`

### PaddleOCR

1. 使用我魔改了接口的版本：https://github.com/MistEO/PaddleOCR
2. 参考 [这个教程](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.3/deploy/cpp_infer#readme)
3. PaddlePaddle 直接[下载](https://www.paddlepaddle.org.cn/documentation/docs/zh/2.0/guides/05_inference_deployment/inference/build_and_install_lib_cn.html)即可

编译选项参考

```bash
cmake ../ -DPADDLE_LIB=/your_path/paddle_inference/ -D OPENCV_DIR=/your_path_to_opencv/ -DBUILD_SHARED=ON -DWITH_MKL=OFF -DWITH_STATIC_LIB=OFF
```

### meojson

https://github.com/MistEO/meojson

```bash
make shared
```

### penguin-stats-recognize-v3

使用我魔改了接口的版本：https://github.com/MistEO/penguin-stats-recognize-v3

## 部署

1. 直接拷贝上面编译的第三方库到`3rdparty/lib` or 手动修改`CMakeLists.txt`指定第三方库路径
2. 复制资源文件到`libMeoAssitant.so`同一目录下，具体可参考 [这个脚本](../../tools/update_resource.sh)
3. 通过 [Python 接口](../Python/interface.py) 或 [C 接口](../../include/AsstCaller.h) 进行调用，需要自行编写少量的代码

## 集成文档

[~~或许算不上文档~~](https://github.com/MistEO/MeoAssistantArknights/wiki)

### Python

可参考 [Python 接口](../Python/interface.py) 中 `__main__` 的实现

### C

可参考 [TestCaller](../../tools/TestCaller/main.cpp) 中的实现

### C sharp

可参考 [MeoAsstGui](../MeoAsstGui/Helper/AsstProxy.cs) 中的实现
