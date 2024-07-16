---
order: 5
icon: ph:question-fill
---

# 常见问题

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

## 1. 如何在 macOS 上使用 `$HOME/.config/maa` 作为配置文件目录？

由于 Rust 库 [Directories](https://github.com/dirs-dev/directories-rs/) 在 macOS 上默认使用 Apple 风格目录，maa-cli 默认也使用 Apple 风格的配置目录。但是对于命令行程序来说，XDG 风格的目录更加合适。如果你想要使用 XDG 风格目录，你可以设置 `XDG_CONFIG_HOME` 环境变量，如 `export XDG_CONFIG_HOME="$HOME/.config"`，这会让 maa-cli 使用 XDG 风格配置目录。如果你想要使用 XDG 风格配置目录，但是不想设置环境变量，你可以使用下面的命令创建一个符号链接：

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```

## 2. 运行过程中出现奇怪日志，如何关闭？

MaaCore 的依赖 `fastdeploy` 会输出一些日志，因此你在运行 maa-cli 时可能会看到类似于下面的日志：

```plaintext
[INFO] ... /fastdeploy/runtime.cc(544)::Init Runtime initialized with Backend::ORT in Device::CPU.`
```

这个日志是由 `fastdeploy` 输出的，对于官方预编译的 MaaCore，这个日志无法关闭。但是如果你是使用包管理器安装的 maa-cli，你可以尝试安装包管理器提供的 MaaCore。包管理器提供的 MaaCore 使用了较新版本的 `fastdeploy`，其日志默认是关闭的。

## 3. 在 macOS 下使用 maa-cli 启动的 PlayCover 游戏客户端，无法代理作战且肉鸽无法结算

由于未知原因，通过终端启动的 PlayCover 游戏客户端无法代理作战，且肉鸽无法结算。但是该状况仅限于通过终端运行的 maa-cli。因此如果你是在终端交互使用，那么你可能需要手动启动游戏。如果你是通过 cron 等方式运行 maa-cli，那么该问题不会影响你的使用。
