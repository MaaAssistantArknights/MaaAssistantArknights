---
order: 2
icon: basil:apple-solid
---

# Mac でのエミューレータ

::: tip
問題が発生した場合は、まず [よくある質問](../faq.md) を参照してください
:::

## Apple Silicon チップ

### ✅ [PlayCover](https://playcover.io)（ネイティブに動作、最も流暢である 🚀）

実験的なサポート、問題が発生した場合は問題を開き、タイトルに iOS について言及してください。

注： `macOS` の仕組みにより、ゲームウィンドウを最小化したり、ステージマネージャーで別のウィンドウに切り替えたり、ウィンドウを別のデスクトップ/画面に移動したりすると、スクリーンショットに問題が発生し、正しく実行されなくなります。 👉🏻️ [issue](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512) を参照

0. 要件: MAA バージョン v4.13.0-rc.1 以降

1. [PlayCover のフォークバージョン](https://github.com/hguandl/PlayCover/releases) をダウンロードしてインストールします。

2. [殻を剥がしたアークナイツ クライアント インストーラー](https://decrypt.day/app/id1454663939)をダウンロードし、 PlayCover にインストールします。

3. PlayCover で アークナイツを右クリックし、 `設定` - `バイパス` を選択し、 `PlayChainを有効にする` 、 `脱獄検出バイパスを有効にする` 、 `イントロスペクションの挿入` 、 `MaaTools` にチェックを入れ、 `OK` をクリックします。

4. この時、アークナイツを再び起動すれば、正常に動作できるようになります。タイトルバーの最後に `[localhost:port number]` と表示され、正常に有効になったことを示します。

5. MAAで、`設定` - `接続設定` 、 `タッチモード` の順にクリックし、 `MacPlayTools` を選択します。 `接続アドレス` は、上のタイトルバーの `[]` セクションにいたコンテンツを入力します。

6. セットアップが完了すると、MAA は接続する準備が整います。 画像認識でエラーが発生した場合は、PlayCover で解像度を 1080p に設定してみてください。

7. ステップ 3-5 は一度だけ行う必要があり、その後はアークナイツをアクティブにするだけで済みます。アークナイツのクライアントを更新するたびに、手順2を再度実行する必要があります。

### ✅ [MuMu エミューレータ Pro](https://mumu.163.com/mac/)

サポートされています、ただしテストは少なく、 `MacPlayTools` 以外のタッチモードが必要です。 関連する問題 [#8098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8098)

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

サポートされています。Android 10 以降、SELinux が\`Enforcing\`モードの場合、Minitouch は使用できません、別のタッチモードに切り替えてください。または SELinux を **一時的に** \`Permissive\`モードに切り替え。

## Intel チップ

::: tip
Mac版MAAの開発者が少なく、更新速度が比較的遅いです。そのため、Macに標準搭載されているマルチシステムでWindowsをインストールし、Windows版MAAを使用することをお勧めします。
:::

### ✅ [Bluestacks-CN](https://www.bluestacks.cn/)

サポートされています。エミュレータ `設定` - `エンジン設定` で `ADB接続を許可する` をオンにする必要があります。

### ✅ [Bluestacks](https://www.bluestacks.com/tw/index.html)

サポートされています。エミュレータ `設定` - `詳細` で `Androidデバッグブリッジ` をオンにする必要があります。

### ✅ [NOX](https://www.yeshen.com/)

サポートされています。現在、 Ma cの MAAX バージョンはエミュレータの自動適応をサポートしていないため、MAAの `設定` - `接続設定` で、 `adb` に `127.0.0.1：62001`を接続する必要があり、ポートはデフォルトの` 5555 `ではないことに注意し、エミュレータポートに関する詳細な説明は[一般的なエミュレータadbポート](../faq.md#一般的なAndroidエミュレータのadbポート)を参照してください。

補足：mac の NOX の adb バイナリファイルの場所は `/Applications/NoxAppPlayer.app/Contents/MacOS/adb` 、親ディレクトリ `MacOS` の下で `adb devices` コマンドを使用してadbポートを表示できます。

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

サポートされています。
