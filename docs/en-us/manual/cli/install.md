---
order: 1
icon: material-symbols:download
---

# Installation and Compilation

There are multiple ways to install maa-cli, including pre-compiled binaries, package managers, and building from source with `cargo`.

## Pre-compiled Binaries

The easiest way to install maa-cli is by using the installation script:

::: tabs#pre-compile

@tab:active Linux and macOS

```bash
curl -fsSL https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.sh | bash
```

@tab Windows (PowerShell)

```powershell
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.ps1" -OutFile install.ps1; .\install.ps1
```

:::

You can later update maa-cli by running `maa self update`.

If your platform is not listed above, you can try to [build from source](#build-from-source).

## Install via Package Manager

For macOS and supported Linux distributions, you can install maa-cli using a package manager.

### macOS

Homebrew users can install maa-cli via the unofficial [tap](https://github.com/MaaAssistantArknights/homebrew-tap/):

::: code-tabs

@tab:active Stable

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab Unstable/Pre-release

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

Arch, Nix, and Linux Homebrew users can install maa-cli via their package managers.

#### Arch Linux

You can install the [AUR package](https://aur.archlinux.org/packages/maa-cli/):

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

Users can run directly:

::: code-tabs

@tab:active Stable

```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab Nightly Build

```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

The stable version is packaged in [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix), using the Rust toolchain from `nixpkgs`; the nightly build is in [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix), using the Beta channel Rust toolchain, and is automatically updated and verified daily by a GitHub Actions.

#### Homebrew

For users who use Homebrew on Linux, please refer to the macOS installation method.

#### Other Distributions

Please use the [pre-compiled binaries](#pre-compiled-binaries) or [build from source](#build-from-source).

We welcome interested developers to submit maa-cli to the official or user repositories of more distributions!

## Build from Source

Rust developers can compile and install maa-cli using `cargo`:

::: code-tabs

@tab:active Stable Version

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
```

@tab Development Version

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

:::

::: warning
The Minimum Supported Rust Version (MSRV) for maa-cli is currently 1.88. Please note that the MSRV may change at any time, and we recommend always using the latest Rust toolchain for the best experience.
:::

### Build Options

When compiling from source, you can disable default features with `--no-default-features` and then enable specific features with `--features`. The available features are:

- `cli_installer`: Enables the `maa self update` command for self-updating. Enabled by default.
- `core_installer`: Enables the `maa install` and `maa update` commands for installing and updating MaaCore and its resources. Enabled by default.
- `git2`: Provides the `libgit2` backend for resource updates. Enabled by default.

## Install MaaCore and Resources

maa-cli only provides a command-line interface; it requires MaaCore and resources to run tasks.

Your steps will differ depending on the installation method and platform:

::: tabs#maacore

@tab:active Pre-compiled
For users of pre-compiled binaries or those who built from source, maa-cli can help you install and update:

```bash :no-line-numbers
maa install
```

@tab Windows
For Windows users, before running `maa install`, please run the following command as an administrator in Command Prompt or PowerShell to install the necessary VC++ runtime:

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

Then run `maa install`.

@tab Arch
You can use maa-cli to install the pre-compiled MaaCore:

```bash :no-line-numbers
maa install
```

You can also install maa-core via the [AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/):

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

or

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

@tab Nix
maa-cli on Nix has a hard dependency on MaaCore, so Nix users do not need to, and should not, install MaaCore manually.

:::

::: warning
`maa install` downloads the officially pre-compiled MaaCore. MaaCore installed via package managers may use different compile options and dependency versions, which might lead to slight differences in performance and functionality.
:::
