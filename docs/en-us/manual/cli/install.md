---
order: 2
icon: material-symbols:download
---

# Install and Build

There are multiple ways to install maa-cli, including package managers, precompiled binaries, and building from source with `cargo`.

## Install via Package Manager

For macOS and supported Linux distributions, it is recommended to install maa-cli using a package manager.

### macOS

Homebrew users can install maa-cli via the unofficial [tap](https://github.com/MaaAssistantArknights/homebrew-tap/):

- Stable release:

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli
  ```

- Beta releases:

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli-beta
  ```

### Linux

- Arch Linux users can install the [AUR package](https://aur.archlinux.org/packages/maa-cli/):

  ```bash
  yay -S maa-cli
  ```

- ❄️ Nix users can run directly:

  ```bash
  # Stable release
  nix run nixpkgs#maa-cli
  ```

  ```bash
  # Nightly build
  nix run github:Cryolitia/nur-packages#maa-cli-nightly
  ```

  Stable is the maa-cli packaged in [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix), using the nixpkgs's Rust toolchain; Nightly is in [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix), uses the Beta Channel of Rust toolchain, automatically updates and builds for verification by Github Action daily.

- Users using Homebrew on Linux please refer to the macOS installation method above.

## Precompiled Binaries

If package managers are not available on your system or you prefer not to use them, you can download the precompiled binaries for your platform from the following links. After decompressing, place the executable file in your `PATH` to use.

- [macOS](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-universal-apple-darwin.zip)
- [Linux x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-unknown-linux-gnu.tar.gz)
- [Linux aarch64 (arm64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-aarch64-unknown-linux-gnu.tar.gz)
- [Windows x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-pc-windows-msvc.zip)

If your platform is not listed above, you can try to compile and install it yourself (see below).

## Build from Source

Rust developers can compile and install maa-cli themselves via `cargo`:

- Stable version:

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
  ```

- Development version:

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
  ```

### Features

When compiling from source, you can disable the default features with `--no-default-features` and then enable specific features with `--features`. The available features are:

- `cli_installer`: Provide `maa self update` command to update self, this feature is enabled by default;
- `core_installer`: Provide `maa install` and `maa update` commands to install and update MaaCore and resources, this feature is enabled by default;
- `git2`: Provide `libgit2` resource backend, this feature is enabled by default;
- `vendored-openssl`: Build OpenSSL library by self instead of using system library, this feature is disabled by default;

## Install MaaCore

maa-cli only provides an interface for MaaCore, it needs MaaCore and resources to run tasks, which can be installed by maa-cli once it is installed:

```bash
maa install
```

For users who installed via package managers, MaaCore can be installed via package managers:

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

**NOTE**: Only users who installed maa-cli via package managers can install MaaCore via package managers. Otherwise, please use the `maa install` command to install. In addition, the `maa install` downloads the official precompiled MaaCore, while the MaaCore installed by package managers has different compilation options and dependency versions from the official precompiled version. This does not affect the use of maa-cli but may cause differences in the functionality and performance of MaaCore. For example, the MaaCore installed by package managers uses a newer version of `fastdeploy`, while the official precompiled MaaCore uses an older version of `fastdeploy`. In the new version of `fastdeploy`, logs can be hidden, which can eliminate some unnecessary log output.
