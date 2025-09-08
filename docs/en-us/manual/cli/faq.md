---
order: 4
icon: ph:question-fill
---

# FAQs

## 1. How to use `$HOME/.config/maa` as the configuration directory on macOS?

Due to the Rust library [Directories](https://github.com/dirs-dev/directories-rs/) using Apple-style directories on macOS by default, maa-cli also uses Apple-style configuration directories by default. However, XDG-style directories are more suitable for command-line programs. If you want to use XDG-style directories, you can set the `XDG_CONFIG_HOME` environment variable, such as `export XDG_CONFIG_HOME="$HOME/.config"`, which will make maa-cli use XDG-style configuration directories. If you want to use XDG-style configuration directories but don't want to set environment variables, you can use the following command to create a symbolic link:

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```
