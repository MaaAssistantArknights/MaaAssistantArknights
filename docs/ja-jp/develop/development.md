---
order: 1
icon: iconoir:developer
---

# 開発を開始

## Github Pull Request プロセス概要

### プログラミングの仕方がわからないので、JSONファイルやドキュメントなどを少し変更したいのですが、どうすればいいですか？

[「パラスちゃん」も理解できるGitHubのPull Requestの使用ガイド](./pr-tutorial.md)へようこそ（純WebサイトのPRチュートリアル）

### プログラミングの仕方を知っていますが、GitHub/C++/...... に触れたことがありません、どうすればいいですか？

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
    _（maadeps-download.py はプロジェクトルートに配置）_

    ```cmd
    python maadeps-download.py
    ```

5. 開発環境の設定

    - `Visual Studio 2022 Community` をインストール時、`C++ によるデスクトップ開発` と `.NET デスクトップ開発` を選択必須

6. `MAA.sln` をダブルクリックで開き、Visual Studio にプロジェクトを自動ロード
7. VS の設定

    - 上部設定バーで `RelWithDebInfo` `x64` を選択（Release ビルド/ARM プラットフォームの場合は不要）
    - `MaaWpfGui` 右クリック → プロパティ → デバッグ → ネイティブデバッグを有効化（C++ Core へのブレークポイント設定可能）

8. これで自由に ~~改造~~ 開発を始められます
9. 一定量の変更ごにコミット（メッセージ記入必須）  
   Git 未経験者は dev ブランチ直接変更ではなく新規ブランチ作成推奨：

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    これで dev の更新影響を受けずに開発可能

10. 開発完了後、変更をリモートリポジトリへプッシュ：

    ```bash
    git push origin dev
    ```

11. [MAA メインリポジトリ](https://github.com/MaaAssistantArknights/MaaAssistantArknights) で Pull Request を提出（master ではなく dev ブランチを指定）
12. 上流リポジトリの更新を同期する場合：

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

    4. ステップ7、8、9、10 を繰り返し

::: tip
Visual Studio 起動後、Git 操作は「Git 変更」画面からコマンドライン不要で可能
:::

## Visual Studioでclang-formatを有効にする

1. clang-format バージョン17以上をインストールします。

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

## オンライン開発に GitHub codespace を使用する

GitHub codespace を作成して C++ 開発環境を自動的に構成する

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

次に、 vscode または [Linuxチュートリアル](./linux-tutorial.md) のプロンプトに従って、 GCC 12 および CMake プロジェクトを構成します。
