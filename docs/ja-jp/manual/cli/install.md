---
order: 1
icon: material-symbols:download-2-rounded
---

# インストールとコンパイル

maa-cli は、プリコンパイル済みバイナリ、パッケージマネージャー、または `cargo` を使用した自己コンパイルなど、複数のインストール方法を提供しています。

## プリコンパイル済みバイナリ

maa-cli をインストールする最も簡単な方法は、インストールスクリプトを使用する方法です：

::: tabs#pre-compile

@tab:active Linux と macOS

```bash
curl -fsSL https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.sh | bash
```

@tab Windows (PowerShell)

```powershell
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.ps1" -OutFile install.ps1; .\install.ps1
```

:::

その後、`maa self update` コマンドで maa-cli を更新できます。

お使いのプラットフォームが上記のリストにない場合は、[コンパイルによるインストール](#コンパイルによるインストール)を試してみてください。

## パッケージマネージャーによるインストール

macOS および対応している Linux ディストリビューションユーザーは、パッケージマネージャーを使用して maa-cli をインストールできます。

### macOS

Homebrew ユーザーは、非公式の [tap](https://github.com/MaaAssistantArknights/homebrew-tap/) を使用して maa-cli をインストールできます：

::: code-tabs

@tab:active 安定版

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab 不安定版/プレリリース版

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

Arch、Nix、および Linux Homebrew ユーザーは、パッケージマネージャーを使用して maa-cli をインストールできます。

#### Arch Linux

[AURパッケージ](https://aur.archlinux.org/packages/maa-cli/) をインストールできます：

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

ユーザーは直接実行できます：

::: code-tabs

@tab:active 安定版

```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab ナイトリービルド

```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

安定版は [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix) にパッケージされており、`nixpkgs` の Rust ツールチェーンを使用します。ナイトリービルドは [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix) にあり、Rust ツールチェーンのベータチャンネルを使用し、GitHub Actions によって毎日自動更新およびビルド検証が行われます。

#### Homebrew

Linux で Homebrew を使用するユーザーは、上記の macOS のインストール方法を参照してください。

#### その他のディストリビューション

[プリコンパイル済みバイナリ](#プリコンパイル済みバイナリ)を使用するか、[コンパイルによるインストール](#コンパイルによるインストール)を行ってください。

また、maa-cli をより多くのディストリビューションの公式リポジトリやユーザーリポジトリに提出してくださる開発者の方々も歓迎します！

## コンパイルによるインストール

Rust 開発者は、`cargo` を使用して maa-cli を自己コンパイルできます：

::: code-tabs

@tab:active 安定バージョン

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
```

@tab 開発バージョン

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

:::

::: warning
maa-cli の現在の最小サポート Rust バージョン（MSRV）は 1.88 です。MSRVは予告なく変更される可能性があるため、最適な体験を得るには常に最新のRustツールチェーンを使用することをお勧めします。
:::

### コンパイルオプション

ソースからコンパイルする場合、`--no-default-features` でデフォルトのフィーチャーを無効にし、`--features` で特定のフィーチャーを有効にすることができます。現在利用可能なフィーチャーは以下の通りです：

- `cli_installer`: `maa self update` コマンドを有効にし、自身を更新するために使用します。このフィーチャーはデフォルトで有効です。
- `core_installer`: `maa install` および `maa update` コマンドを有効にし、MaaCore とリソースをインストールおよび更新するために使用します。このフィーチャーはデフォルトで有効です。
- `git2`: `libgit2` リソース更新バックエンドを提供します。このフィーチャーはデフォルトで有効です。

## MaaCoreとリソースのインストール

maa-cli はコマンドラインインターフェースのみを提供しており、タスクを実行するには MaaCore とリソースが必要です。

インストール方法とプラットフォームによって、操作は異なります：

::: tabs#maacore

@tab:active プリコンパイル
プリコンパイル済みバイナリを使用する場合、または自己コンパイルしたユーザーの場合、maa-cli がインストールと更新を支援します：

```bash :no-line-numbers
maa install
```

@tab Windows
Windows プラットフォームのユーザーは、`maa install` コマンドを実行する前に、管理者権限でコマンドプロンプトまたは PowerShell で以下のコマンドを実行し、必須コンポーネントである VC++ ランタイムをインストールしてください：

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

その後、`maa install` を実行してください。

@tab Arch
maa-cli を使用してプリコンパイル済みの MaaCore をインストールできます：

```bash :no-line-numbers
maa install
```

また、[AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/) を使用して maa-core をインストールすることもできます：

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

または

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

@tab Nix
Nix の maa-cli は MaaCore に強く依存しています。そのため、Nix ユーザーは MaaCore を手動でインストールする必要はなく、またすべきではありません。

:::

::: warning
`maa install` は MAA 公式がプリコンパイルした MaaCore をダウンロードします。一方、パッケージマネージャーでインストールされる MaaCore は、公式のプリコンパイルバージョンとは異なるコンパイルオプションや依存関係のバージョンを使用する可能性があり、これによりパフォーマンスや機能にわずかな違いが生じる可能性があります。
:::
