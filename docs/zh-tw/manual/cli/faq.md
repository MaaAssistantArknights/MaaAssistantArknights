---
order: 2
icon: ph:question-fill
---

# 常見問題

## 1. 如何在 macOS 上使用 `$HOME/.config/maa` 作為配置目錄？

由於 Rust 函式庫 [Directories](https://github.com/dirs-dev/directories-rs/) 在 macOS 上預設使用 Apple 風格目錄，maa-cli 預設也使用 Apple 風格的配置目錄。但對於命令列程式來說，XDG 風格的目錄更加合適。如果您想要使用 XDG 風格目錄，您可以設定 `XDG_CONFIG_HOME` 環境變數，例如 `export XDG_CONFIG_HOME="$HOME/.config"`，這會讓 maa-cli 使用 XDG 風格的配置目錄。如果您想要使用 XDG 風格的配置目錄，但不想設定環境變數，您可以使用下方的指令建立一個符號連結：

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```