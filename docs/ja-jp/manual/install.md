---
order: 2
icon: material-symbols:download-2-rounded
---

<!-- markdownlint-disable MD024 -->

# ダウンロードとインストール

::: tip

これは MAA GUI のダウンロードとインストールに関するドキュメントです。maa-cli のダウンロードとインストールについては、[インストールとコンパイル](./cli/install.md)を参照してください。現在、Android版「MAA-Meow（MAA Android版）」はテスト公開中です。詳細は [MAA-Meow](https://github.com/Aliothmoon/MAA-Meow)をご覧ください。（現在、このソフトウェアのUIは中国語のみサポートしています）

:::

## MAA のダウンロード

MAA では、公式サイトからのダウンロード、パッケージマネージャーからのインストール、グループファイルからのダウンロードなど、複数の方法を提供しています。ご都合に合わせた方法でダウンロードしてください。

### [公式サイト](https://maa.plus)から最新の MAA パッケージをダウンロード

通常、公式サイトでは正しいバージョンとアーキテクチャが自動選択されます。ほとんどの読者にとっては Windows x64 です。macOS ユーザーの方は、macOS ユニバーサル版をダウンロードしてください。

### [Mirror ちゃん](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install)から最新の MAA パッケージをダウンロード

システムアーキテクチャを確認し、対応するパッケージをダウンロードしてください。ほとんどの Windows ユーザーにとっては Windows x64 です。Mac ユーザーの場合、Mirrorちゃんはユニバーサルパッケージを提供していないため、お使いのチップアーキテクチャ（arm/x86）を確認して対応するパッケージをダウンロードしてください。

::: tip

[Mirrorちゃん](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install)は独立したサードパーティのダウンロードアクセラレーションサービスであり、MAA の有料サービスではありません。その運営コストはサブスクリプション収入で賄われ、収益の一部はプロジェクト開発者に還元されます。高速ダウンロードを楽しみながらプロジェクトの継続的な開発を支援するために、CDK の購読をご検討ください。

:::

### Windows パッケージマネージャー（Winget）を使用したインストール

::: tip

この方法は Windows ユーザーのみ利用可能です。

:::

ターミナルで以下のコマンドを実行してください：

```bash
winget install maa
```

この方法でインストールした場合、デフォルトのインストールパスは `C:\Users\ユーザー名\AppData\Local\Microsoft\WinGet\Packages` です。

### QQ グループファイルから最新のMAAパッケージをダウンロード

1. [MAA 公式 QQ グループ](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)に参加してください。
2. グループファイル内で最新の MAA パッケージをダウンロードしてください。

### [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)から最新の MAA パッケージをダウンロード

システムアーキテクチャを確認し、対応するパッケージをダウンロードしてください。ほとんどの Windows ユーザーにとっては `MAA-<バージョン番号>-win-x64.zip` です。macOS ユーザーの方は `MAA-<バージョン番号>-macos-universal.dmg` を選択してください。

## Linux およびその他のオペレーティングシステム

MAA GUI は**現時点では** Linux やその他のオペレーティングシステムをサポートしていません。これらのシステムで MAA の機能を使用するには、**maa-cli** を利用できます。詳細は、maa-cli の [インストールとコンパイル](./cli/install.md)を参照してください。

## MAA のインストール

### Windows

ダウンロードが完了すると、`.zip`ファイルが取得できます。解凍ソフトで完全に解凍すると、MAAのすべてのファイルを含むフォルダが作成されます。

::: warning

1. `C:\` や `C:\Program Files\` など UAC 権限が必要なパスに MAA を解凍しないでください。

2. MAA には .NET ランタイムが内蔵されています（自己完結型デプロイ）。ただし、Visual C++ Redistributable x64（VCRedist x64）が必要です。解凍したMAAディレクトリ内で、管理者権限で `DependencySetup_依赖库安装.bat` を実行し、この依存関係をインストールしてください。インストール完了後、`MAA.exe` を実行してください。

詳細は[よくある質問](./faq.md)のトップを参照してください。

:::

`MAA.exe`をダブルクリックすると、MAAが起動します。

::: tip

Windows パッケージマネージャー（Winget）を使用してインストールした場合、解凍やランタイムのインストールなどの追加操作なしで、コマンドラインに `maa` と入力するだけで MAA を起動できます。ただし、PATHに `maa-cli` が存在する場合、それらを区別するために追加の手順が必要になることがあります。

:::

### macOS

ダウンロードが完了すると、`.dmg`ファイルが取得できます。その `.dmg` をダブルクリックして開き、`MAA.app` を `/Applications` にドラッグしてインストールを完了します。

## 次のステップ

インストールが完了したら、[初心者向けガイド](./newbie.md)に戻って設定を続行するか、[機能紹介](./introduction/)で MAA がサポートするさまざまな機能を確認してください。インストール中に問題が発生した場合は、[よくある質問](./faq.md)を参照して解決を試みてください。
