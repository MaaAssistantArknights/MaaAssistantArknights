---
order: 1
icon: ri:guide-fill
---

# 始めに

## 準備

1. システムバージョンの確認

   MAAはWindows 10 / 11 のみサポートしています。それ以前のバージョンのWIndowsについては[よくある質問](./faq.md#システムの問題)を確認してください。

   Windows 以外のユーザーは[エミュレーターおよびデバイスのサポート](./device/)をご覧ください。

2. 環境にあったバージョンのダウンロード

   [MAA公式ウェブサイト](https://maa.plus/) は自動的に環境を感知しバージョンを選択します。この文章を読んでいるほとんどのユーザーはWindows x64だと思われます。

3. Zipファイルを解凍する

   解凍が完了したことを確認し、MAAを別のフォルダに解凍してください。`C:\`や`C:\Program Files\`などのUACパーミッションが必要なパスにMAAを展開しないでください。

4. ランタイムライブラリをインストールする

   MAAはVCRedist x64と.NET 8が必要です。MAAディレクトリ内の `DependencySetup_依赖库安装.bat` を実行してインストールしてください。

5. エミュレータのサポートを確認

   [エミュレータのサポート](./device)をクリックして、使用しているエミュレータのサポートを確認してください。

6. エミュレータの解像度を正しく設定する

   エミュレーターの解像度は横画面の `1280x720` または `1920x1080` にするべきです；YosterEN のプレイヤーには、`1920x1080` が必須です。

## 初回起動

1. 設定ガイドに従って設定してください。複数のポートがあるなど、特別な要件がない場合は、接続設定を変更する必要はありません。

2. MAAを初めて実行すると、ホットアップデートが実行されます。MAAを終了し、右のログでプロンプトが表示された後に再起動してください。

3. 左側のタスクリストをドラッグしてタスクを並べ替え、チェックボックスをオンまたはオフにして実行するタスクを選択します。

4. Link Start!

## 高度な設定

ドキュメントを参照してください。

## その他

- **問題が発生したらドキュメントをお読みください。**

1. ログファイルはMAAフォルダの下にある`debug`という名前のフォルダにあります。ログは重要です。他の人に助けを求めるときは、必ず`asst.log`と`gui.log`を準備してください。

2. MAAでは，ユーザーの皆様の多様なニーズに可能な限りお応えするため，数多くのカスタマイズオプションを用意しています。例えば，「ステージ名の手入力」や「オペレーターを宿舎に入れない」などです。

3. いくつかのオプションの上にマウスポインターを置くと、「仕事中のオペレーターを宿舎に入れない」など、詳細な指示が表示されます。

4. チェックボックスの中には、右クリックで半選択状態にできるものもあります。このチェックボックスは、次回MAAを起動したときに自動的にクリアになります。

5. 使用用途がはっきりしない場合は、先行版リリースを選択しないでください。
