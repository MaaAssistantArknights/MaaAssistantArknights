---
order: 1
icon: material-symbols:download
---

# 安装及编译

maa-cli 提供多种方式安装，包括包管理器、预编译二进制文件和通过 `cargo` 自行编译安装。

## 通过包管理器安装

对于 macOS 和受支持的 Linux 发行版用户，推荐使用包管理器安装 maa-cli。

### macOS

Homebrew 用户可以通过非官方的 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/) 安装 maa-cli：

::: code-tabs

@tab:active 稳定版
```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab 不稳定版/预发行版
```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

我们目前为 Arch 和 Nix 用户提供预编译基于包管理器的分发支持

#### Arch Linux 

可以安装 [AUR 包](https://aur.archlinux.org/packages/maa-cli/)：

::: code-tabs

@tab:active paru
```bash :no-line-numbers
paru -S maa-cli
```

@tab yay
```bash :no-line-numbers
yay -S maa-cli
```

:::

#### ❄️ Nix 

用户可以直接运行:

::: code-tabs

@tab:active 稳定版
```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab 每夜构建
```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

稳定版打包至 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix) 中，使用 `nixpkgs` 中的 Rust 工具链；每夜构建位于 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix) 中，使用 Beta channel 的 Rust 工具链，由 Github Action 每日动更新和构建验证

#### 其他发行版

对于在 Linux 上使用 Homebrew 的用户，请参见上述 macOS 的安装方式。

否则，请 [使用预编译二进制文件](#预编译二进制文件) 或是 [自行编译安装](#编译安装)

我们也欢迎各位有兴趣的开发者将 maa-cli 提交到更多发行版的官方仓库或是用户仓库！

## 预编译二进制文件

如果你的系统不受支持或者不想使用包管理器，你可以点击以下链接下载对应平台的预编译二进制文件，解压后将可执行文件放入 `PATH` 中即可使用。

::: tabs#pre-compile

@tab:active macOS
[前往下载](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-universal-apple-darwin.zip)

@tab Linux
请选择你的架构
+ [x64/x86_64/amd64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-unknown-linux-gnu.tar.gz)
+ [aarch64/arm64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-aarch64-unknown-linux-gnu.tar.gz)

@tab Windows
请选择你的架构
+ [x64/x86_64/amd64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-pc-windows-msvc.zip)
+ [aarch64/arm64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-aarch64-pc-windows-msvc.zip)

:::

如果你的平台不在上述列表中，可以尝试自行编译安装（参见下文）

## 编译安装

Rust 开发者可以通过 `cargo` 自行编译安装 maa-cli：

::: code-tabs

@tab:active 稳定版本
```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
```

@tab 开发版本
```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

:::

### 编译选项

从源码编译时，你可以通过 `--no-default-features` 禁用默认的特性，然后通过 `--features` 来启用特定的特性。目前可用的特性有：

- `cli_installer`: 启用 `maa self update` 命令，用于更新自身，这个特性默认启用；
- `core_installer`: 启用 `maa install` 和 `maa update` 命令，用于安装和更新 MaaCore 及资源，这个特性默认启用；
- `git2`: 提供 `libgit2` 资源更新后端，这个特性默认启用；
- `vendored-openssl`: 自行编译 `openssl` 库，而不是使用系统的 `openssl` 库，这个特性默认禁用，这个特性通常在你的系统没有安装 `openssl` 库或者 `openssl` 版本过低时启用。

## 安装 MaaCore 及资源

::: warning
只有使用包管理器安装 maa-cli 的用户才能使用包管理器安装 MaaCore，否则请使用 `maa install` 命令安装  
此外，`maa install` 下载的是官方预编译的 MaaCore，而包管理器安装的 MaaCore 可能使用与官方预编译版本不同的编译选项和依赖版本，这也许会导致性能和功能上的略微差异
:::

maa-cli 只提供了一个命令行界面，它需要 MaaCore 和资源来运行任务。一旦 maa-cli 安装完成，你可以通过它安装 MaaCore 及资源：

```bash :no-line-numbers
maa install
```

根据不同的平台，你的操作也有所区别

::: tabs#maacore

@tab Windows
对于 Windows 平台用户，在运行 `maa install` 命令前，请以管理员身份在命令提示符或PowerShell中运行以下命令，以安装必要工具组 VC++

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

然后再运行 `maa install`

@tab Homebrew
你可以直接通过 Homebrew 安装 maa-core
```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-core
```

@tab Arch
  你可以通过 [AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/) 安装 maa-core
  ::: code-tabs

  @tab:active paru
  ```bash :no-line-numbers
  paru -S maa-assistant-arknights
  ```

  @tab yay
  ```bash :no-line-numbers
  yay -S maa-assistant-arknights
  ```
  :::

@tab Nix
Nix 上的 maa-cli 强制依赖 MaaCore，因此 Nix 用户无需，也不应该手动安装 MaaCore

:::
