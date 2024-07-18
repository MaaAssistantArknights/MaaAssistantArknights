---
order: 1
icon: iconoir:developer
---

# 開発を開始

## Github Pull Request プロセス概要

### プログラミングの仕方がわからないので、JSONファイルやドキュメントなどを少し変更したいのですが、どうすればいいですか？

[「パラスちゃん」も理解できるGitHubのPull Requestの使用ガイド](./pr-tutorial.md)へようこそ（純WebサイトのPRチュートリアル）

### プログラミングの仕方を知っていますが、GitHub/C++/...... に触れたことがありません、どうすればいいですか？

1. かなり前にフォークした場合は、まず自分のリポジトリの `Settings` の一番下に移動してそのリポジトリを削除します。
2. [MAAリポジトリ](https://github.com/MaaAssistantArknights/MaaAssistantArknights)を開いて、 `Fork` をクリックし、引き続き `Create fork` をクリックします。
3. （自分のアカウントの）リポジトリの `dev` のブランチをローカルに clone をします。

    ```bash
    git clone --recurse-submodules <リポジトリの git のリンク> -b dev
    ```

    Visual Studioなどの `--recurse-submodules` パラメータを含まないGit GUIを使用している場合は、クローン後に `git submodule update --init` を実行して、サブモジュールを取得する必要があります。

4. 構築済みのサードパーティライブラリをダウンロードする

    **Python環境が必要ですので、Pythonインストールチュートリアルを自分で検索してください**  
    _（maadeps-download.py ファイルはプロジェクトのルートにあります）_

    ```cmd
    python maadeps-download.py
    ```

5. プログラミング環境を構成

    - `Visual Studio 2022 community` をダウンロードしてインストールします。インストール時に `C++ベースのデスクトップ開発` と `.NETデスクトップ開発` を選択する必要があります。

6. ダブルクリックして `MAA.sln` ファイルを開きます。 Visual Studioは、プロジェクト全体を自動的に読み込みます。
7. VS を設定する

    - VS 上で構成を `RelWithDebInfo` `x64` に選択します （リリースパッケージまたは ARM プラットフォームをコンパイルする場合は、この手順をスキップしてください）
    - `MaaWpfGui` を右クリックして、 - `プロパティー` - `デバッグ` - `ローカル デバッグを有効にする` （これにより、ブレークポイントを C++ コアにフックできます）

8. ここまで楽しくコードを変更できます～
9. 開発中は、一定量修正するたびに commit を忘れず、コミットメッセージを書くことを忘れないでください。  
    git を初めて使用する場合は、 `dev` に直接コミットする代わりに、新しいブランチを作成して変更を加えることをお勧めします

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    このようにして、コミットは `dev` の更新に邪魔されることなく、新しいブランチで成長できます

10. 開発が完了したら、ローカルブランチを（自分の）リモートリポジトリにプッシュします。

    ```bash
    git push origin dev
    ```

11. [MAAリポジトリ](https://github.com/MaaAssistantArknights/MaaAssistantArknights)を開いて、プルリクエストを送信し、管理者が承認するのを待ちます。 dev ブランチで変更することを忘れないで、 master ブランチにコミットしないでください。
12. 元のMAAリポジトリに変更がある場合（他のユーザーが行った）、これらの変更を自分のフォークリポジトリに同期する必要があります。

    1. MAAオリジナル リポジトリーの関連付け

        ```bash
        git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
        ```

    2. MAAのオリジナル リポジトリーから更新をプルします

        ```bash
        git fetch upstream
        ```

    3. リベース (推奨) またはマージでブランチに統合する

        ```bash
        git rebase upstream/dev  # リベース
        ```

        または

        ```bash
        git merge  # マージ
        ```

    4. 上記の手順7、8、9、10を繰り返します

::: tip
VS2022を開いた後、gitに関する操作はコマンドラインツールなしで実行でき、VSに付属の「Git変更」を直接使用できます。
:::

## Visual Studioでclang-formatを有効にする

1. clang-format バージョン17以上をインストールします。

    ```bash
    python -m pip install clang-format
    ```

2. 'Everything'などのツールを使用して、clang-format.exeのインストール場所を見つけます。参考までに、Anacondaを使用している場合、clang-format.exeはYourAnacondaPath/Scripts/clang-format.exeにインストールされます。
3. Visual Studioで、 Tools-Optionsで 'clang-format'を検索します。
4. `clang-formatサポートを有効にする` をクリックし、下の `カスタムのclang-format.exeファイルを使用する` を選択し、最初取得した `clang-format.exe` を選択します。

![Visual Studioでclang-formatを有効にする](https://github.com/MaaAssistantArknights/MaaAssistantArknights/assets/18511905/23ab94dd-09da-4b88-8c62-6b5f9dfad1a2)

そうすれば、 Visual Studio は c++20 構文をサポートする clang-format を問題なく使用できます！

`tools\ClangFormatter\clang-formatter.py` を使用して、clang-formatを直接呼び出して書式設定することもできます。プロジェクトのルートディレクトリで

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`

を実行するだけです。

## オンライン開発に GitHub codespace を使用する

GitHub codespace を作成して C++ 開発環境を自動的に構成する

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

次に、 vscode または [Linuxチュートリアル](./linux-tutorial.md) のプロンプトに従って、 GCC 12 および CMake プロジェクトを構成します。
