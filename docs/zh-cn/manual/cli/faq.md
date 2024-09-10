---
order: 4
icon: ph:question-fill
---

# 常见问题

## 1. 如何在 macOS 上使用 `$HOME/.config/maa` 作为配置文件目录？

由于 Rust 库 [Directories](https://github.com/dirs-dev/directories-rs/) 在 macOS 上默认使用 Apple 风格目录，maa-cli 默认也使用 Apple 风格的配置目录。但是对于命令行程序来说，XDG 风格的目录更加合适。如果你想要使用 XDG 风格目录，你可以设置 `XDG_CONFIG_HOME` 环境变量，如 `export XDG_CONFIG_HOME="$HOME/.config"`，这会让 maa-cli 使用 XDG 风格配置目录。如果你想要使用 XDG 风格配置目录，但是不想设置环境变量，你可以使用下面的命令创建一个符号链接：

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```
