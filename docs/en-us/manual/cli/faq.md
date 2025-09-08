---
order: 4
icon: ph:question-fill
---

# FAQs

## 1. How to use `$HOME/.config/maa` as the configuration directory on macOS?

Due to the limitation of [Directories](https://github.com/dirs-dev/directories-rs/), maa-cli uses the Apple-style configuration directory on macOS by default. However, the XDG-style configuration directory is more suitable for command-line programs. If you want to use the XDG style configuration directory, you can set the `XDG_CONFIG_HOME` environment variable, such as `export XDG_CONFIG_HOME="$HOME/.config"`, this will make maa-cli use the XDG style configuration directory. Or you can use the below command to create a symbolic link:

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```
