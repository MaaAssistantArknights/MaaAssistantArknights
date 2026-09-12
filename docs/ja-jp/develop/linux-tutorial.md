---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux コンパイル チュートリアル

::: warning
**このチュートリアルでは、読者にLinux環境の構成能力とプログラミングの基礎が必要です！**MAAを自分でコンパイルするのではなく実行するだけの場合は、[ユーザーマニュアル - Linux エミュレータとコンテナ](../manual/device/linux.md)をお読みください。
:::

::: info 注意
MAAの構築方法はまだ議論されていますが、このチュートリアルの内容は古くなる可能性があります。[GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/.github/workflows/ci.yml#L264#:~:text=ubuntu%3A) のスクリプトに準拠してください。  
また、[AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) や [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix) も参照可能です。
:::

::: info
Mac は `tools/build_macos_universal.zsh` スクリプトを使用してコンパイルできます。  
MaaAssistantArknights/MaaMacGui プロジェクトの [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md) を参照することをお勧めします。
:::

## MaaCore のコンパイル

:::: steps

1. コンパイルに必要な依存関係をダウンロードする
   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash :no-line-numbers
   sudo apt install cmake
   ```

   @tab Arch

   ```bash :no-line-numbers
   sudo pacman -S --needed cmake
   ```

   :::

2. サードパーティ製ライブラリのビルド

   以下のいずれかの方法を選択してください：

   - 事前構築されたサードパーティ製ライブラリのダウンロード（推奨）

     ```bash
     python tools/maadeps-download.py
     ```

     ::: info
     事前構築されたサードパーティ製ライブラリは [MaaLinuxToolchain](https://github.com/MaaXYZ/MaaLinuxToolchain) ツールチェーンによってクロスコンパイルされており、glibc 2.31 (Ubuntu 20.04) のみに依存します。依然として ABI の非互換性問題が発生する場合は、コンテナを使用するか、最初から自分でビルドしてみてください。
     :::

   - サードパーティ製ライブラリを自分でビルドする（時間がかかります）

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # システム環境が古すぎて事前構築の llvm 20 を使用できない場合は、クロスコンパイルを使わずローカルのビルド環境を使用することを検討してください。
     # src/MaaUtils/MaaDeps/cmake 内の toolchain 設定を調整する必要があります。
     python linux-toolchain-download.py
     python build.py
     ```

3. MAAのコンパイル

   ```bash
   cmake --preset linux-x64 -DINSTALL_RESOURCE=ON -DINSTALL_PYTHON=ON
   cmake --build build
   cmake --install build --prefix <target_directory>
   ```

   最初の2行で `build/bin/Debug/libMaaCore.so`（および `libMaaUtils.so`）が生成され、3行目でビルド成果物がインストール（つまり、コピー）されます。

   ::: info CMake オプションの説明
   `-DINSTALL_RESOURCE=ON` は `resource` ディレクトリをインストール先にコピーします。MaaCore の実行には、`resource` ディレクトリが必要です。

   `-DINSTALL_PYTHON=ON` は Python インテグレーション（`src/Python`）をインストール先にコピーします。使用しない場合やリポジトリから直接利用する場合は省略可能です。
   :::

   ::: tip
   動的ライブラリのパスまたは `LD_LIBRARY_PATH` を指定して MAA を実行することをお勧めします。root 権限で MAA を `/usr` にインストールしないでください。
   :::

4. MaaFramework 関連コンポーネントのコンパイル

   MaaFwAdbController（MaaFwAdb タッチモード）関連の機能をデバッグする場合は、[MaaFramework の Debug バージョンを自分でコンパイル](https://maafw.com/docs/4.1-BuildGuide)し、`libMaaAdbControlUnit.so` をインストールディレクトリに配置する必要があります。

5. 実行

   [各プログラミング言語 API](../readme.md#api) や [Python の使用](../manual/device/linux.md#python-を使用する) などの資料を参照して MAA 動的ライブラリを呼び出します。

::::

## MaaWpfGui のコンパイル

::: info 注意
MaaWpfGui は Windows 向けに作られており、Linux 上でビルド成果物を実行するには Wine を介する必要があります。詳細は [Wine を使用する](../manual/device/linux.md#wine-を使用する) を参照してください。
:::

:::: steps
1. `MaaCore.dll` の準備

   MaaWpfGui は `MaaCore.dll`（および依存する他の DLL）に依存します。現在、Linux から Windows 版の `MaaCore.dll` をクロスコンパイルすることはできません。MAA の Windows 版からコピーすることも可能ですが、バージョンの不一致が生じやすくなります。

   そのため、推奨される手順は以下のとおりです。まず前のセクションの説明に従って MaaCore をビルド（インストールは不要）し、その後 [MaaWineBridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev-v2/src/MaaWineBridge) をビルドして、生成された `MaaCore.dll` を `build/bin/Debug` ディレクトリに配置します。

2. .NET SDK のインストール

   ::: code-tabs
   @tab:active Ubuntu

   ```bash
   sudo apt install dotnet-sdk-10.0
   ```

   @tab Arch

   ```bash
   sudo pacman -S --needed dotnet-sdk
   ```

   :::

   [Linux に .NET をインストールする](https://learn.microsoft.com/en-us/dotnet/core/install/linux) も参照してください。

3. MaaWpfGui のコンパイル

   リポジトリのルートディレクトリで

   ```bash
   dotnet build src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64
   ```

   を実行します。初回のコンパイル時に依存関係が自動的にダウンロードされます。`build/bin/Debug/MAA.exe` が生成されます。

4. 実行

   ```bash
   wine build/bin/Debug/MAA.exe
   ```
::::
