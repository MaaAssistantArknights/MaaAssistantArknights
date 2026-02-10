---
order: 1
icon: iconoir:developer
---

# 開発ガイド

::: tip
本ページは主に PR プロセスおよび MAA のファイルフォーマット要件について説明します。MAA の実行ロジックを変更する具体的な方法については、[プロトコル文書](../protocol/)を参照してください。
:::

::: tip
[DeepWiki に質問して](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights)、MAA プロジェクトの全体的なアーキテクチャを概要的に理解することができます。
:::

## プログラミングの仕方がわからないので、JSONファイルやドキュメントなどを少し変更したいのですが、どうすればいいですか？

[「パラスちゃん」も理解できるGitHubのPull Requestの使用ガイド](./pr-tutorial.md)へようこそ（純WebサイトのPRチュートリアル）

## 数行のコードを少しだけ変更したいが、環境設定が面倒。純粋なWeb編集も使いにくい。どうすればよいですか？

[GitHub Codespaces](https://github.com/codespaces)オンライン開発環境をご利用ください！

次のような、異なる開発環境を事前に準備しています：

- 空白環境（裸のLinuxコンテナ）（デフォルト）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- 軽量環境：ドキュメントサイトのフロントエンド開発に適している

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- 完全環境：MAA Core関連の開発に適している（使用は推奨しません。ローカル開発を推奨します。関連環境を完全に設定。次のセクションを参照）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## 完全な環境セットアップ（Windows）

1. かなり前にフォークした場合は、まず自分のリポジトリの `Settings` の一番下で削除します
2. [MAA メインリポジトリ](https://github.com/MaaAssistantArknights/MaaAssistantArknights)を開き、`Fork` → `Create fork` をクリック
3. 自身のリポジトリの dev ブランチをクローン（サブモジュール含む）

   ```bash
   git clone --recurse-submodules <リポジトリの git リンク> -b dev
   ```

   ::: warning
   Visual Studio など `--recurse-submodules` パラメータに対応していない Git GUI を使用する場合、クローン後に以下を実行：

   ```bash
   git submodule update --init
   ```

   :::

4. 事前ビルド済みサードパーティライブラリのダウンロード

   **Python環境が必要（インストール方法は各自検索）**  
   _（tools/maadeps-download.py はプロジェクトルートに配置）_

   ```cmd
   python tools/maadeps-download.py
   ```

5. 開発環境の設定
   - `CMake` をダウンロードしてインストール
   - `Visual Studio 2026 Community` をインストール時、`C++ によるデスクトップ開発` と `.NET デスクトップ開発` を選択必須

6. cmake プロジェクト設定を実行

   ```cmd
   cmake --preset windows-x64
   ```

7. `build/MAA.slnx` をダブルクリックで開き、Visual Studio にプロジェクトを自動ロード
8. VS の設定
   - 上部設定バーで `Debug` `x64` を選択
   - `MaaWpfGui` 右クリック → スタートアップ プロジェクトに設定
   - F5 キーを押して実行

   ::: tip
   Win32Controller（Windows ウィンドウ制御）関連機能をデバッグする場合は、[MaaFramework Releases](https://github.com/MaaXYZ/MaaFramework/releases) から対応プラットフォームのアーカイブをダウンロードし、`bin` ディレクトリ内の `MaaWin32ControlUnit.dll` を MAA の DLL と同じディレクトリ（例：`build/bin/Debug`）に配置してください。自動ダウンロードスクリプトの PR 歓迎！
   :::

9. これで自由に ~~改造~~ 開発を始められます
10. 一定量の変更ごとにコミット（メッセージ記入必須）  
    Git 未経験者は dev ブランチ直接変更ではなく新規ブランチ作成推奨：

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    これで dev の更新影響を受けずに開発可能

11. 開発完了後、変更をリモートリポジトリへプッシュ：

    ```bash
    git push origin dev
    ```

12. [MAA メインリポジトリ](https://github.com/MaaAssistantArknights/MaaAssistantArknights) で Pull Request を提出（master ではなく dev ブランチを指定）
13. 上流リポジトリの更新を同期する場合：
    1. 上流リポジトリを追加：

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. 更新を取得：

       ```bash
       git fetch upstream
       ```

    3. リベース（推奨）またはマージ：

       ```bash
       git rebase upstream/dev
       ```

       または

       ```bash
       git merge
       ```

    4. ステップ8、9、10、11 を繰り返し

::: tip
Visual Studio 起動後、Git 操作は「Git 変更」画面からコマンドライン不要で可能
:::

## VSCodeでの開発（オプション）

::: warning
**Visual Studio での開発を推奨します。** MAA プロジェクトは主に Visual Studio をベースに構築されており、上記の完全な環境セットアップですべての開発ニーズをカバーし、すぐに使える最高の体験を提供します。VSCode ワークフローは、VSCode + CMake + clangd に精通した開発者向けの代替手段としてのみ提供されており、設定のハードルが比較的高くなります。
:::

VSCodeを好む場合、CMake、clangdなどの拡張機能でコード補完、ナビゲーション、デバッグが可能です。前述の手順1～6（クローン、依存関係、CMake設定）を完了した後、以下の手順で設定できます。

### 推奨拡張機能

VS Code Marketplace からインストール：

| 拡張機能                                                                                            | 用途                                                              |
| --------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)            | CMake の設定、ビルド、デバッグ統合                                |
| [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) | C++ インテリセンス、コードナビゲーション、診断（LSPベース）       |
| [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)                     | C++ プログラムのデバッグ（CMake Tools または launch.json と連携） |

::: tip
clangd を使用する場合、C/C++ 拡張機能の IntelliSense を無効化（`C_Cpp.intelliSenseEngine` を `disabled` に設定）することを推奨します。競合を避けるためです。
:::

### 設定手順

1. VS Code でプロジェクトルートを開く
2. **CMake Tools**：ステータスバーで Configure Preset（例：`windows-x64`、`linux-x64`）を選択し、Build Preset でビルドを実行
3. **clangd**：Windows で MSVC を使用する場合、`compile_commands.json` がなくても clangd で開発可能。Linux/macOS ではプリセットで `CMAKE_EXPORT_COMPILE_COMMANDS` が有効となり、clangd は `build/compile_commands.json` を自動使用
4. **デバッグ**：プロジェクトには `.vscode/launch.json` が含まれており、MaaWpfGui や Debug Demo の起動が可能

### ビルドとデバッグのショートカット

- **ビルド**：`Ctrl+Shift+B` または CMake Tools ステータスバー
- **デバッグ**：F5 または Run and Debug パネルで設定を選択

## MAAのファイルフォーマット要件

MAAは、リポジトリ内のコードとリソースファイルが美しく統一されるよう、一連のフォーマットツールを使用してメンテナンスと読み取りを容易にしています。

提出する前にフォーマットするか、または[Pre-commit Hooksを使用してコードを自動フォーマット](#pre-commit-hooksを使用してコードを自動フォーマット)してください。

現在有効になっているフォーマットツールは次のとおりです：

| ファイルタイプ | フォーマットツール                                              |
| -------------- | --------------------------------------------------------------- |
| C++            | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)    |
| JSON/YAML      | [Prettier](https://prettier.io/)                                |
| Markdown       | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### Pre-commit Hooksを使用してコードを自動フォーマット

1. コンピュータにPython環境とNode環境があることを確認してください。

2. プロジェクトのルートディレクトリで次のコマンドを実行します：

   ```bash
   pip install pre-commit
   pre-commit install
   ```

pipインストール後もPre-commitを実行できない場合は、PIPインストールパスがPATHに追加されていることを確認してください。

次に、提出するたびにフォーマットツールが自動的に実行され、コード形式がスタイルガイドに準拠していることを確認します。

### Visual Studioでclang-formatを有効にする

1. clang-format バージョン20.1.0以上をインストールします。

   ```bash
   python -m pip install clang-format
   ```

2. 'Everything'などのツールを使用して、clang-format.exeのインストール場所を見つけます。参考までに、Anacondaを使用している場合、clang-format.exeはYourAnacondaPath/Scripts/clang-format.exeにインストールされます。
3. Visual Studioで、 Tools-Optionsで 'clang-format'を検索します。
4. `clang-formatサポートを有効にする` をクリックし、下の `カスタムのclang-format.exeファイルを使用する` を選択し、最初取得した `clang-format.exe` を選択します。

![Visual Studioでclang-formatを有効にする](/images/zh-cn/development-enable-vs-clang-format.png)

そうすれば、 Visual Studio は c++20 構文をサポートする clang-format を問題なく使用できます！

`tools\ClangFormatter\clang-formatter.py` を使用して、clang-formatを直接呼び出して書式設定することもできます。プロジェクトのルートディレクトリで

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`

を実行するだけです。
