---
order: 5
icon: ph:question-fill
---

# FAQs

## 1. How to use `$HOME/.config/maa` as the configuration directory on macOS?

Due to the limitation of [Directories](https://github.com/dirs-dev/directories-rs/), maa-cli uses the Apple-style configuration directory on macOS by default. However, the XDG-style configuration directory is more suitable for command-line programs. If you want to use the XDG style configuration directory, you can set the `XDG_CONFIG_HOME` environment variable, such as `export XDG_CONFIG_HOME="$HOME/.config"`, this will make maa-cli use the XDG style configuration directory. Or you can use the below command to create a symbolic link:

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```

## 2. Strange logs appear during running, how to disable them?

When running the maa-cli task, you may see some logs that look like this:

```plaintext
[INFO] ... /fastdeploy/runtime.cc(544)::Init Runtime initialized with Backend::ORT in Device::CPU.
```

This log is output by `fastdeploy`, which is a dependency of MaaCore. For the officially compiled MaaCore, this log cannot be closed. However, if you are using a package manager to install maa-cli, you can try installing the package manager's version of MaaCore which uses a newer version of `fastdeploy` without logs enabled by default.
