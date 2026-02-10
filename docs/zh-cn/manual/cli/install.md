---
order: 1
icon: material-symbols:download
---

# 安装及编译

maa-cli 提供多种方式安装，包括预编译二进制文件、包管理器和通过 `cargo` 自行编译安装。

## 预编译二进制文件

安装 maa-cli 最简单的方式是使用安装脚本一键安装：

::: tabs#pre-compile

@tab:active Linux 和 macOS

```bash
curl -fsSL https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.sh | bash
```

@tab Windows (PowerShell)

```powershell
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.ps1" -OutFile install.ps1; .\install.ps1
```

:::

后续你可以通过 `maa self update` 来更新 maa-cli。

如果你的平台不在上述列表中，可以尝试自行[编译安装](#编译安装)。

## 通过包管理器安装

对于 macOS 和受支持的 Linux 发行版用户，可以使用包管理器安装 maa-cli。

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

Arch、Nix 以及 Linux Homebrew 用户可以通过包管理器安装 maa-cli。

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

稳定版打包至 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix) 中，使用 `nixpkgs` 中的 Rust 工具链；每夜构建位于 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix) 中，使用 Beta channel 的 Rust 工具链，由 GitHub Actions 每日自动更新和构建验证。

#### Homebrew

对于在 Linux 上使用 Homebrew 的用户，请参见上述 macOS 的安装方式。

#### 其他发行版

请使用 [预编译二进制文件](#预编译二进制文件) 或是 [自行编译安装](#编译安装)。

我们也欢迎各位有兴趣的开发者将 maa-cli 提交到更多发行版的官方仓库或是用户仓库！

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

::: warning
maa-cli 目前的最低支持 Rust 版本 (MSRV) 为 1.88。请注意，MSRV 可能随时发生变化，我们建议始终使用最新的 Rust 工具链以获得最佳体验。
:::

### 编译选项

从源码编译时，你可以通过 `--no-default-features` 禁用默认的特性，然后通过 `--features` 来启用特定的特性。目前可用的特性有：

- `cli_installer`: 启用 `maa self update` 命令，用于更新自身，这个特性默认启用；
- `core_installer`: 启用 `maa install` 和 `maa update` 命令，用于安装和更新 MaaCore 及资源，这个特性默认启用；
- `git2`: 提供 `libgit2` 资源更新后端，这个特性默认启用。

## 安装 MaaCore 及资源

maa-cli 只提供了一个命令行界面，它需要 MaaCore 和资源来运行任务。

根据安装方式和平台的不同，你的操作也有所区别：

::: tabs#maacore

@tab:active 预编译
对于使用预编译二进制或者自行编译的用户，maa-cli 可以帮助你安装和更新：

```bash :no-line-numbers
maa install
```

@tab Windows
对于 Windows 平台的用户，在运行 `maa install` 命令前，请以管理员身份在命令提示符或 PowerShell 中运行以下命令，以安装必要组件 VC++ 运行库：

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

然后再运行 `maa install`。

@tab Arch
你可以使用 maa-cli 安装预编译的 MaaCore:

```bash :no-line-numbers
maa install
```

你也可以通过 [AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/) 安装 maa-core

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

或者

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

@tab Nix
Nix 上的 maa-cli 强制依赖 MaaCore，因此 Nix 用户无需，也不应该手动安装 MaaCore。

:::

::: warning
`maa install` 下载的是 MAA 官方预编译的 MaaCore，而包管理器安装的 MaaCore 可能使用与官方预编译版本不同的编译选项和依赖版本，这也许会导致性能和功能上的略微差异。
:::
