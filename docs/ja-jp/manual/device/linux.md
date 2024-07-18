---
order: 3
icon: teenyicons:linux-alt-solid
---

# Linux でのエミューレータ

## 準備作業

### 1. MAA をインストールする

1. Linux ダイナミック ライブラリを [MAA ウェブサイト](https://maa.plus/) からダウンロードし、解凍します

2. `./MAA-v{バージョン}-linux-{アアーキテクチャ}/Python/` ディレクトリに移動します、 `sample.py` ファイルを開け

::: tip
プリコンパイル済みバージョンには、比較的新しいLinuxディストリビューション(Ubuntu 22.04)でコンパイルされた動的ライブラリが含まれており、システムに古いバージョンのlibstdc++がある場合、ABIの非互換性に遭遇する可能性があります
[Linuxコンパイル・チュートリアル](../../develop/linux-tutorial.md) を参照して再コンパイルまたはコンテナを使用して実行できます
:::

- Arch Linuxシリーズのリリース版は、aur パッケージ [maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)を選択する使用し、インストール後のプロンプトに従ってファイルを編集することができる

### 2. `adb` 構成

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

### 3. タスク構成

カストムタスク： 必要に応じて [統合ドキュメント](../../protocol/integration.md) を参照し、 `sample.py` の [`# タスクとパラメーターについては 統合ドキュメント`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54) 欄を変更する

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
