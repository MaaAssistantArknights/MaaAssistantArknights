---
order: 2
icon: material-symbols:download-2-rounded
---

<!-- markdownlint-disable MD024 -->

# Download & Installation

::: tip
You are viewing the MAA GUI download and installation guide. If you need the maa-cli download and installation instructions, see maa-cli's [Installation and Build](./cli/install.md). The Android variant (MAA Android) is currently in open testing — see [MAA-Meow](https://github.com/Aliothmoon/MAA-Meow) for details.
:::

## Downloading MAA

MAA offers multiple download channels, including the official website, package managers, and group file shares. Choose the method that fits your environment.

### Download the latest package from the [official website](https://maa.plus)

The website usually selects the correct architecture automatically; for most readers this will be Windows x64. macOS users should choose the macOS universal build.

### Download the latest package via [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install)

Confirm your system architecture and download the matching package. Mirror酱 does not provide a macOS universal build — macOS users should select the package matching their chip architecture (arm/x86).

::: tip
[Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install) is a third‑party, paid download acceleration service and is not operated by the MAA project. Its operating costs are covered by subscriptions and a portion of revenue is shared with project contributors. Consider subscribing to CDK for faster downloads and to support the project.
:::

### Install via Windows package manager (Winget)

::: tip
This method applies only to Windows users.
:::

Run the following command in a terminal:

```bash
winget install maa
```

When installed this way the default install path is `C:\Users\<username>\AppData\Local\Microsoft\WinGet\Packages`.

### Download from the official QQ group files

1. Join the [MAA official QQ group](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. Download the latest MAA package from the group's shared files.

### Download from [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

Make sure to choose the package that matches your system architecture. For most Windows users download `MAA-<version>-win-x64.zip`. For macOS users choose `MAA-<version>-macos-universal.dmg`.

## Linux and other operating systems

MAA GUI is currently not supported on Linux or other operating systems. You can use `maa-cli` to access MAA functionality on those systems — see the maa-cli [Installation and Build](./cli/install.md) document for details.

## Installing MAA

### Windows

After downloading you will have a `.zip` file. Extract it fully with your archive tool to obtain a folder containing all MAA files.

::: warning

1. Do not extract MAA to locations that require UAC elevation such as `C:\` or `C:\Program Files\`.
2. MAA includes a bundled .NET runtime (self-contained deployment), but it still requires the Visual C++ Redistributable x64 (VCRedist x64). After extraction, run `DependencySetup_依赖库安装.bat` in the MAA directory as administrator to install the required dependencies. After installation completes, run `MAA.exe`.

For more information see the top of the [FAQ](./faq.md).
:::

Double-click `MAA.exe` to start MAA.

::: tip
If you installed via Winget you can start MAA by running `maa` from the command line without extracting the archive or installing runtimes manually. If `maa-cli` is on your PATH you may need extra steps to disambiguate the two.
:::

### macOS

Open the downloaded `.dmg` and drag `MAA.app` to `/Applications` to complete installation.

## Next steps

After installation, return to the [New User Guide](./newbie.md) to continue setup, or visit [Introduction](./introduction/) to learn about MAA's features. If you encounter problems during installation, check the [FAQ](./faq.md) for troubleshooting tips.
