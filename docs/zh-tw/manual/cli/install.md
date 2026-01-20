---
order: 1
icon: material-symbols:download
---

# 安裝及編譯

maa-cli 提供多種方式安裝，包括預編譯執行檔、套件管理員以及透過 `cargo` 自行編譯安裝。

## 預編譯執行檔

安裝 maa-cli 最簡單的方式是使用安裝指令碼一鍵安裝：

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

後續您可以透過 `maa self update` 來更新 maa-cli。

如果您的平台不在上述列表中，可以嘗試自行[編譯安裝](#編譯安裝)。

## 透過套件管理員安裝

對於 macOS 和受支援的 Linux 發行版使用者，可以使用套件管理員安裝 maa-cli。

### macOS

Homebrew 使用者可以透過非官方的 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/) 安裝 maa-cli：

::: code-tabs

@tab:active 穩定版本

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab 不穩定版本 / 預發行版本

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

Arch、Nix 以及 Linux Homebrew 使用者可以透過套件管理員安裝 maa-cli。

#### Arch Linux

可以安裝 [AUR 套件](https://aur.archlinux.org/packages/maa-cli/)：

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

使用者可以直接執行：

::: code-tabs

@tab:active 穩定版本

```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab Nightly 版本

```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

穩定版本已收錄至 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix) 中，並使用 `nixpkgs` 中的 Rust 工具鏈；Nightly 版本則位於 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix)，使用 Beta 分支的 Rust 工具鏈，且每日經由 GitHub Actions 自動更新與驗證建置結果。

#### Homebrew

對於在 Linux 上使用 Homebrew 的使用者，請參考上述 macOS 的安裝方式。

#### 其他发行版

請使用 [預編譯執行檔](#預編譯執行檔) 或是 [自行編譯安裝](#編譯安裝)。

我們也歡迎各位有興趣的開發者將 maa-cli 提交到更多發行版的官方倉庫或是使用者倉庫！

## 編譯安裝

Rust 開發者可以透過 `cargo` 自行編譯安裝 maa-cli：

::: code-tabs

@tab:active 穩定版本

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
```

@tab 開發版本

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

:::

::: warning
maa-cli 目前的最低支援 Rust 版本 (MSRV) 為 1.88。請注意，MSRV 可能隨時發生變化，我們建議始終使用最新的 Rust 工具鏈以獲得最佳體驗。
:::

### 編譯選項

從原始碼編譯時，您可以透過 `--no-default-features` 停用預設的特性，然後透過 `--features` 來啟用特定的特性。目前可用的特性有：

- `cli_installer`: 啟用 `maa self update` 指令，用於更新自身，這個特性預設啟用。
- `core_installer`: 啟用 `maa install` 和 `maa update` 指令，用於安裝和更新 MaaCore 及資源，這個特性預設啟用。
- `git2`: 提供 `libgit2` 資源更新後端，這個特性預設啟用。

## 安裝 MaaCore 及資源

maa-cli 只提供了一個命令列介面，它需要 MaaCore 和資源來執行任務。

根據安裝方式和平台的不同，您的操作也會有所區別：

::: tabs#maacore

@tab:active 預編譯
對於使用預編譯執行檔或者自行編譯的使用者，maa-cli 可以協助您安裝和更新：

```bash :no-line-numbers
maa install
```

@tab Windows
對於 Windows 使用者，在執行 `maa install` 指令前，請以管理員身份在命令提示字元或 PowerShell 中執行以下指令，以安裝必要組件 VC++ 執行環境 (Redistributable)：

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

然後再執行 `maa install`。

@tab Arch
您可以使用 maa-cli 安裝預編譯的 MaaCore：

```bash :no-line-numbers
maa install
```

您也可以透過 [AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/) 安裝 maa-core

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

或者

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

@tab Nix
Nix 上的 maa-cli 強制依賴 MaaCore，因此 Nix 使用者無需、也不應該手動安裝 MaaCore。

:::

::: warning
`maa install` 下載的是 MAA 官方預編譯的 MaaCore，而套件管理員安裝的 MaaCore 可能使用與官方預編譯版本不同的編譯選項和依賴版本，這也許會導致效能和功能上的些微差異。
:::
