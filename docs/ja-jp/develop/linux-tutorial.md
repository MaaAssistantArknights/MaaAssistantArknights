---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux コンパイル チュートリアル

**このチュートリアルでは、読者にLinux環境の構成能力とプログラミングの基礎が必要です！**

::: info 注意
MAAの構築方法はまだ議論されていますが、このチュートリアルの内容は古くなる可能性があります。 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134) のスクリプトに準拠してください
:::

## コンパイルプロセス

1. コンパイルに必要な依存をダウンロードして

   - Ubuntu/Debian

   ```bash
   sudo apt install gcc-12 g++-12 cmake zlib1g-dev
   ```

2. サードパーティ製ライブラリのコンパイル

   構築済みの依存ライブラリをダウンロードするか、最初からコンパイルするかを選択できます

   - 事前構築されたサードパーティ製ライブラリのダウンロード（推奨）

     > **Note**
     > 比較的新しいLinuxリリース（Ubuntu 22.04）でコンパイルされたダイナミックライブラリに含まれています。システム内のlibstdc++バージョンが古い場合、ABI互換性のない問題が発生する可能性があります。

     ```bash
     python maadeps-download.py
     ```

   上記の方法でダウンロードしたライブラリがABIバージョンなどの理由でシステム上で実行できず、コンテナなどのスキームを使用したくないことがわかった場合は、最初からコンパイルしてみることもできます

   - サードパーティ製ライブラリを直接にコンパイルする（時間がかかる）

     ```bash
     git submodule update --init --recursive
     cd MaaDeps
     python build.py
     ```

3. MAAのコンパイル

   ```bash
   CC=gcc-12 CXX=g++-12 cmake -B build \
       -DINSTALL_THIRD_LIBS=ON \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON
   cmake --build build
   ```

   MAA をターゲットロケーションにインストールします。管理者権限を使用して MAA を `/usr` にロードしないで、MAAは `LD_LIBRARY_PATH` を指定して実行することを推奨します。

   ```bash
   cmake --install build --prefix <target_directory>
   ```

## その他のインストール方法

- AUR: [maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)
- NUR: [nur.repos.cryolitia.MaaAssistantArknights](https://github.com/nix-community/nur-combined/tree/master/repos/cryolitia/pkgs/MaaAssistantArknights/default.nix#L138)

## 統合ドキュメント

[~~ドキュメントとは言えないかもしれません~~](../プロトコルドキュメント/統合ドキュメント.md)

### Python

[Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py) の中の `__main__` での実装を参照可能

### C++

[CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp) での実装を参照可能

### C\#

[MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs) での実装を参照可能
