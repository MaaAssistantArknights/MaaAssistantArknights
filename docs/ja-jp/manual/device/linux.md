---
order: 3
icon: teenyicons:linux-alt-solid
---

# Linux でのエミューレータ

## 準備作業

以下のインストール方法から一つを選択してください：

### maa-cli を使用する

[maa-cli](https://github.com/MaaAssistantArknights/maa-cli) は Rust で書かれたシンプルな MAA コマンドラインツールです。関連するインストールと使用方法については、[CLI 使用ガイド](../cli/) をご覧ください。

### Wine を使用する

MAA WPF GUI は現在 Wine を通じて実行できます。MAAは.NETランタイムを内蔵しています（自己完結型デプロイ）。

#### インストール手順

1. Visual C++ Redistributable をインストールする：

   [Visual C++ 再頒布可能パッケージ](https://aka.ms/vc14/vc_redist.x64.exe) をダウンロードしてインストールします：
   
   ```shell
   wine vc_redist.x64.exe
   ```
   
   ::: tip
   `DependencySetup_依赖库安装.bat` は winget と Windows の権限昇格機構に依存しているため、Wine では通常正常に動作しません。そのため、ランタイムライブラリは手動でインストールする必要があります。
   :::

2. Windows 版 MAA をダウンロードし、解凍した後、`wine MAA.exe` を実行します。

::: info 注意
接続設定で ADB パスを [Windows 版 `adb.exe`](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) に設定する必要があります。

ADB 経由で USB デバイスに接続する必要がある場合は、まず Wine の外で `adb start-server` を実行し、Wine を通じてネイティブ ADB サーバーに接続してください。
:::

#### Linux ネイティブ MaaCore の使用（実験的機能）

[MAA Wine Bridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/MaaWineBridge) のソースコードをダウンロードしてビルドし、生成された `MaaCore.dll`（ELF ファイル）で Windows 版を置き換え、Linux ネイティブ動的ライブラリ（`libMaaCore.so` および依存関係）を同じディレクトリに配置します。

この状態で Wine を通じて `MAA.exe` を実行すると、Linux ネイティブ動的ライブラリが読み込まれます。

::: info 注意
Linux ネイティブ MaaCore を使用する場合は、接続設定で ADB パスを Linux ネイティブ ADB に設定する必要があります。
:::

#### Linux デスクトップ統合（実験的機能）

デスクトップ統合は、ネイティブデスクトップ通知サポートと fontconfig フォント設定を WPF にマッピングする機能を提供します。

MAA Wine Bridge で生成された `MaaDesktopIntegration.so` を `MAA.exe` と同じディレクトリに配置すると有効になります。

#### 既知の問題

- Wine DirectWrite は強制的にヒンティングを有効にし、DPI を FreeType に渡さないため、フォント表示が良くありません。
- ネイティブデスクトップ通知を使用しない場合、通知がポップアップするとシステム全体のマウスフォーカスを奪うため、他のウィンドウを操作できなくなります。`winecfg` で仮想デスクトップモードを有効にするか、デスクトップ通知を無効にすることで緩和できます。
- Wine-staging ユーザーは、MAA が Wine 環境を正しく検出できるように、`winecfg` の `Wine バージョンを隠す` オプションを無効にする必要があります。
- Wine の Light テーマは WPF で一部のテキストカラーに異常を引き起こすため、`winecfg` でテーマなし（Windows クラシックテーマ）に切り替えることをお勧めします。
- Wine は古い XEmbed トレイアイコンを使用しており、GNOME では正常に動作しない可能性があります。
- Linux ネイティブ MaaCore を使用している場合、自動更新はサポートされていません（~~更新プログラム：Windows 版をダウンロードすべきでしょうか~~）

### Python を使用する

#### MAA 動的ライブラリのインストール

1. Linux ダイナミック ライブラリを [MAA ウェブサイト](https://maa.plus/) からダウンロードし、解凍します、または以下のソフトウェアリポジトリからインストールします：
   - AUR：[maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)、インストール後のプロンプトに従ってファイルを編集します
   - Nixpkgs: [maa-assistant-arknights](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)

2. `./MAA-v{バージョン}-linux-{アアーキテクチャ}/Python/` ディレクトリに移動し、`sample.py` ファイルを開きます

::: tip
プリコンパイル済みバージョンには、比較的新しいLinuxディストリビューション(Ubuntu 22.04)でコンパイルされた動的ライブラリが含まれており、システムに古いバージョンのlibstdc++がある場合、ABIの非互換性に遭遇する可能性があります。
[Linuxコンパイル・チュートリアル](../../develop/linux-tutorial.md) を参照して再コンパイルまたはコンテナを使用して実行できます。
:::

#### ADB 構成

1. [`if asst.connect('adb.exe', '127.0.0.1:5554'):`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L48) セクションを見つける

2. `adb` ツール呼び出し
   - エミュレータが `Android Studio` に `avd` を使用している場合は、 `adb` が付属します。 `adb.exe` の欄に直接 `adb` パスを記入することができ、一般的には `$HOME/Android/Sdk/platform-tools/` で見つけることができます。例：

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "エミュレータの adb アドレス"):
   ```

   - 他のエミュレータを使用する場合は、最初に `adb` をダウンロードして： `$ sudo apt install adb` 次に、パスを入力するか、 `PATH` 環境変数を使用して `adb` を直接入力します

3. エミュレータの `adb` パス取得
   - adb ツールを直接使用できます： `$ adbパス devices` ，例：

   ```shell
   $ /home/foo/Android/Sdk/platform-tools/adb devices
   List of devices attached
   emulator-5554 device
   ```

   - 返される `emulator-5554` はエミュレータのadbアドレスで、 `127.0.0.1:5555` を上書きします、例：

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "emulator-5554"):
   ```

4. この時点で、 `$ python3 sample.py` をテストでき、 `接続成功` が返されれば、基本的に成功です

#### タスク構成

カスタムタスク：必要に応じて [統合ドキュメント](../../protocol/integration.md) を参照し、`sample.py` の [`# タスクとパラメーターについては docs/integration.md 参照`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54) 欄を変更します

## エミュレータのサポート

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

必須構成： 16:9 の画面解像度で、解像度は 720p より大きい必要がある

推奨構成： x86_64 のフレームワーク (R - 30 - x86_64 - Android 11.0) と MAA のLinux x64 ダイナミック ライブラリ

- Android 10 以降、SELinux が `Enforcing` モードの場合、Minitouch は使用できません、別のタッチモードに切り替えてください。または SELinux を **一時的に** `Permissive` モードに切り替え。

### ⚠️ [Genymotion](https://www.genymotion.com/)

高バージョンの Android は x86_64 フレームワークを搭載しており、軽量ですがアークナイツを実行するとフラッシュバックしやすい

厳格なテストは行われておらず、 adb 機能とパス取得に問題はありません

## コンテナー化された Android のサポート

::: tip
以下のソリューションには通常、カーネルモジュールに関する特定の要件がありますので、特定のスキームとディストリビューションに従って適切なカーネルモジュールをインストールしてください
:::

### ✅ [Waydroid](https://waydro.id/)

インストール後に解像度（または 720P より大きく 16:9 の解像度）をリセットしてから、再起動する必要があります：

```shell
waydroid prop set persist.waydroid.width 1280
waydroid prop set persist.waydroid.height 720
```

adb の IP アドレスを設定する： `設定` - `バージョン情報` - `IPアドレス` を開き、最初の `IP` を記録するし、`sample.py` の adb IP に `${記録したIP}:5555` を入力する。

amdgpu を使用すると、`screencap` コマンドが stderr に情報を出力し、画像のデコードに失敗することがあります。
`adb exec-out screencap | xxd | head` と入力し、出力に `/vendor/etc/hwdata/amdgpu.ids: No such file...` のようなテキストがあるかどうかを確認して、これを確認します。
`resource/config.json` のスクリーンショットコマンドを `adb exec-out screencap` から `adb exec-out 'screencap 2>/dev/null'` に変更してみてください。

### ✅ [redroid](https://github.com/remote-android/redroid-doc)

Android 11 バージョンのイメージは正常に動作し、5555 adb ポートを公開する必要があります。
