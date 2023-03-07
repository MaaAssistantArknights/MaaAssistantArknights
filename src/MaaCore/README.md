# MaaCore

MAA 底层 C++ 模块

## 开发相关

### Windows

1. 下载预构建的第三方库

    ```cmd
    python maadeps-download.py
    ```

2. 使用 Visual Studio 2022 打开 `MAA.sln`，右键 `MaaWpfGui`，设为启动项目
3. 右键 `MaaWpfGui` - 属性 - 调试 - 启用本地调试（这样就能把断点挂到 C++ Core 那边了）
4. （可选）若准备提交 PR，建议启用 [clang-format 支持](https://maa.plus/docs/2.2-开发相关.html#在-visual-studio-中启用-clang-format)

### Linux | macOS

请参考 [Linux 编译教程](https://maa.plus/docs/2.1-Linux编译教程.html)
