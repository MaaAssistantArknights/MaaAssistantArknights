---
order: 2
icon: material-symbols:download
---

# 安装及编译 maa-cli

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

maa-cli 提供多种方式安装，包括包管理器、预编译二进制文件和通过 `cargo` 自行编译安装。

## 通过包管理器安装

对于 macOS 和受支持的 Linux 发行版用户，推荐使用包管理器安装 maa-cli。

### macOS

Homebrew 用户可以通过非官方的 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/) 安装 maa-cli：

- 稳定版本：

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli
  ```

- 不稳定预发行版本：

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli-beta
  ```

### Linux

- Arch Linux 用户可以安装 [AUR 包](https://aur.archlinux.org/packages/maa-cli/)：

  ```bash
  yay -S maa-cli
  ```

- ❄️ Nix 用户可以直接运行:

  ```bash
  # 稳定版
  nix run nixpkgs#maa-cli
  ```

  ```bash
  # 每夜构建
  nix run github:Cryolitia/nur-packages#maa-cli-nightly
  ```

  稳定版打包至 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix) 中，使用 `nixpkgs` 中的 Rust 工具链；每夜构建位于 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix) 中，使用 Beta channel 的 Rust 工具链，由 Github Action 每日自动更新和构建验证。

- 对于在 Linux 上使用 Homebrew 的用户，参见上述 macOS 的安装方式。

## 预编译二进制文件

如果你的系统不受支持或者不想使用包管理器，你可以点击以下链接下载对应平台的预编译二进制文件，解压后将可执行文件放入 `PATH` 中即可使用。

- [macOS](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-universal-apple-darwin.zip)
- [Linux x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-x86_64-unknown-linux-gnu.tar.gz)
- [Linux aarch64 (arm64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-aarch64-unknown-linux-gnu.tar.gz)
- [Windows x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-x86_64-pc-windows-msvc.zip)

如果你的平台不在上述列表中，可以尝试自行编译安装（参见下文）。

## 编译安装

Rust 开发者可以通过 `cargo` 自行编译安装 maa-cli：

- 稳定版本：

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
  ```

- 开发版本：

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
  ```

### 编译选项

从源码编译时，你可以通过 `--no-default-features` 禁用默认的特性，然后通过 `--features` 来启用特定的特性。目前可用的特性有：

- `cli_installer`: 启用 `maa self update` 命令，用于更新自身，这个特性默认启用；
- `core_installer`: 启用 `maa install` 和 `maa update` 命令，用于安装和更新 MaaCore 及资源，这个特性默认启用；
- `git2`: 提供 `libgit2` 资源更新后端，这个特性默认启用；
- `vendored-openssl`: 自行编译 `openssl` 库，而不是使用系统的 `openssl` 库，这个特性默认禁用，这个特性通常在你的系统没有安装 `openssl` 库或者 `openssl` 版本过低时启用。

## 安装 MaaCore 及资源

maa-cli 只提供了一个命令行界面，它需要 MaaCore 和资源来运行任务。一旦 maa-cli 安装完成，你可以通过它安装 MaaCore 及资源：

```bash
maa install
```

对于使用包管理器安装的用户，可以通过包管理器安装 MaaCore：

- Homebrew：

  ```bash
  brew install MaaAssistantArknights/tap/maa-core
  ```

- Arch Linux：

  ```bash
  yay -S maa-assistant-arknights
  ```

- Nix：

  ```bash
  nix-env -iA nixpkgs.maa-assistant-arknights
  ```

**注意**：只有使用包管理器安装 maa-cli 的用户才能使用包管理器安装 MaaCore，否则请使用 `maa install` 命令安装。此外，`maa install` 通过下载官方预编译的 MaaCore，而包管理器安装的 MaaCore 的编译选项和依赖版本与官方预编译的版本不同。这不会影响 maa-cli 的使用，但可能会导致 MaaCore 的功能和性能有所不同。比如，包管理器安装的 MaaCore 使用较新版本的 `fastdeploy`，而官方预编译的 MaaCore 使用较旧版本的 `fastdeploy`。而在新版本的 `fastdeploy` 中，日志可以被隐藏，这可以消除了一些不必要的日志输出。

<!-- markdownlint-disable-file MD013 -->
