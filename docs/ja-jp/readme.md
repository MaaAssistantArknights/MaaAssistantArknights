<div align="center">

<img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />

# MaaAssistantArknights

<br>
<div>
    <img alt="C++" src="https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus">
</div>
<div>
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet">
</div>
<div>
    <img alt="license" src="https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4">
</div>
<div>
    <img alt="stars" src="https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social">
    <img alt="GitHub all releases" src="https://img.shields.io/github/downloads/MaaAssistantArknights/MaaAssistantArknights/total?style=social">
</div>
<br>

[简体中文](../readme.md) | [繁體中文](../zh-tw/readme.md) | [English](../en-us/readme.md) | [日本語](../ja-jp/readme.md) | [한국어](../ko-kr/readme.md)

MAAは、MAA Assistant Arknightsです。

アークナイツゲームアシスタント

画像認識技術に基づいて、ワンクリックですべてのデイリーリクエストを完了します！

絶賛開発中  ✿✿ヽ(°▽°)ノ✿<br>

</div>

## 機能一覧

- 自動作戦、ドロップ認識および [PenguinStats](https://penguin-stats.io/) と [Yituliu](https://ark.yituliu.cn/) へデータアップロード。
- 自動基地シフト、オペレーター効率計算、単一設備内に最適なソリューション；[カスタムシフト](./プロトコルドキュメント/基地シフトAPI.md)にも対応しています。
- 自動公開求人、緊急招集票を使う、使い切るのもサポート。[PenguinStats](https://penguin-stats.io/result/stage/recruit/recruit)と[Yituliu](https://ark.yituliu.cn/survey/maarecruitdata)へ公開求人データのアップロード。
- 高スターの公開求人を選択するのに便利な公開求人のパネルの手動認識をサポートします。~~（この上級エリートとCost回復はシージかシージか）~~
- オペレーターのリストを識別し、既存および未所有のオペレーターを記録するためのサポート、公開求人を手動で識別するための潜在データヒントを提供することもできます。（日本語版のクライアントではオペレーターの名刺のフォントサイズが異なるためサポートが貧弱です）
- 倉庫のアイテム認識機能（ベータ） [Arkplanner](https://penguin-stats.io/planner)/[ARK-NIGHTS.com](https://ark-nights.com/settings)と[アークナイツ ツールボックス](https://arkn.lolicon.app/#/material)へ出力可能！使用方法はツール内文章を参照してください。
- 戦友訪問、FP収集、買い物、デイリーリワード収集、ワンクリックして全自動操作！
- 統合戦略自動作戦、オペレーターとレベルの自動認識、源石錐とキャンドルの自動収集、電気ケトルを獲得する！
- 作業JSONファイルをインポートし、自動操作も可能！ [ビデオデモ](https://www.bilibili.com/video/BV1H841177Fk/)（現在JP未対応/中文）
- C、Python、Java、Rust、Golang、Java HTTP、Rust HTTPなどの多種多様なインターフェースに対応、統合や呼び出しが簡単で、自分好みにMAAをカスタマイズできます!

UIを見れば使い方もすぐ分かる！  

![jp1](https://user-images.githubusercontent.com/99072975/232383662-9000d3dd-f6be-4bb6-8ffd-526a8815aa24.png)
![jp2](https://user-images.githubusercontent.com/99072975/232383668-6a42a55c-8c83-4c3c-a203-51e64eb4f39a.png)

## ダウンロードリンク

- [安定版/パブリックベータ版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)
- [ベータ版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## マニュアル

### 基本設定

1. [エミュレータのサポート](./使用説明/エミュレータのサポート/Windowsでのエミューレータ.md)を参照して、対応するエミュレーターの操作に進んでください。
2. エミュレータの解像度を「1280 * 720」以上に変更します。最大は「2K」、アスペクト比は「16:9」です。  
   YostarENでは、ゲームインターフェイスの問題から、解像度を`1920 * 1080`に変更することをお勧めします。
3. ゲームスタート！

詳しいマニュアルは[MAA使用説明書](./使用説明/詳細説明.md)を参照してください。

## よくある質問

- 起動する際にソフトウェアがクラッシュする。
- 接続エラー、adbパスがわからない場合。
- 接続は成功しますが、応答がありません。
- カスタムポートを接続するにはどうすれば。
- ダウンロード速度が遅く、ミラーサイトはWebページを開くことができません。
- ダウンロードが中途半端になり "ログイン"/"認証 "のプロンプトが出る。
- 接続は成功しますが、オペレーション開始した後に反応がない。

[よくある質問](./使用説明/よくある質問.md)を参照してください。

## サーバーに応じてサポートされる機能

現在、国際クライアント（英語クライアント）、日本語クライアント、韓国語クライアント、繁体字中国語クライアントのほとんどの機能がサポートされています。 ただし、海外ユーザーの少なさとプロジェクト要員不足により、十分に検証できていない機能も多いので、ぜひ体験してみてください。  
バグに遭遇した場合、または特定の機能に対する強い要望がある場合は、[Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) and [Discussions](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) で質問してください。 MAA の構築にご参加ください。[海外クライアント対応](#グローバル版を含む海外クライアントの対応について) を参照してください

### CLI支持

MAA はコマンドラインインタフェース（CLI）操作をサポートし、LinuxとmacOSをサポートし、自動スクリプトやグラフィックインタフェースのないサーバで使用することができる。[CLI使用ガイド](./使用説明/CLIユーザーガイド.md)を参照してください。

## 私たちに参加して

### 主な関連プロジェクト

**現在、プロジェクトチームにはフロントエンドの専門家が非常に不足しています。経験があれば、私たちに参加してください！**

- 新しいFramework: [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)
- 新しいGUI：[MaaX](https://github.com/MaaAssistantArknights/MaaX)
- [作業シェアサイト](https://www.prts.plus)：[フロントエンド](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- バックエンド：[MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [公式ウェブサイト](https://www.maa.plus): [フロントエンド](https://github.com/MaaAssistantArknights/maa-website)
- Deep Learning: [MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

### 多言語 (i18n)

MAA は多言語をサポートし、Weblateを使用してローカライズ管理を行います。複数の言語に精通している場合は、[MAA Weblate](https://weblate.maa-org.net)で翻訳のお手伝いをしてください。

MAA は中国語（簡体字）を第一言語とし、翻訳見出しはすべて中国語（簡体字）を基準とする。

[![Weblate](https://weblate.maa-org.net/widgets/maa-assistant-arknights/zh_Hans/maa-wpf-gui/multi-auto.svg)](https://weblate.maa-org.net/engage/maa-assistant-arknights/zh_Hans/)

### Windows

1. ビルド済みのサードパーティ ライブラリをダウンロードします。

      ```cmd
      python maadeps-download.py
      ```

2. Visual Studio 2022 で `MAA.sln` を開き、`MaaWpfGui` を右クリックして、スタートアップ プロジェクトとして設定します。
3. VS 上記の設定で 'RelWithDebInfo' 'x64' を選択します （Release パッケージまたは ARM プラットフォームをコンパイルしている場合は、この手順を無視してください）
4. `MaaWpfGui` を右クリックし、[プロパティ] - [デバッグ] - [ローカル デバッグを有効にする] を選択します (これにより、C++ コアにブレークポイントを掛けることができます)。
5. (オプション) PR を送信する場合は、[clang-formatを有効にする](./開発関連/開発を開始.md#visual-studioでclang-formatを有効にする)を有効にすることをお勧めします。

### Linux | macOS

[Linuxチュートリアル](./開発関連/Linuxチュートリアル.md)を参照してください。

### API

- [Cインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/include/AsstCaller.h)：[統合例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)
- [Pythonインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/asst/asst.py)：[統合例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)
- [Golangインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Golang/)：[統合例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Golang/maa/maa.go)
- [Dartインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Dart)
- [Javaインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java)：[統合例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTPインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/Readme.md)
- [Rustインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust/src/maa_sys)：[HTTPインターフェース](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust)
- [TypeScriptインターフェース](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [Woolangインターフェース](src/Woolang/maa.wo)：[統合例](src/Woolang/main.wo)
- [統合ドキュメント](./プロトコルドキュメント/統合ドキュメント.md)
- [コールバックAPI](./プロトコルドキュメント/コールバックAPI.md)
- [タスクAPI](./プロトコルドキュメント/タスクAPI.md)
- [自動戦闘API](./プロトコルドキュメント/自動戦闘API.md)

### グローバル版を含む海外クライアントの対応について

[海外版クライアントの対応について](./開発関連/海外クライアントの対応.md)をご覧ください。大陸版で既にサポートされている機能を他地域クライアントへ移植するための必要作業の大半は、簡単なJSONの修正と（作業に必要な）スクリーンショットの提出で済みます。。

### 開発に参加したいがGitHubの使い方がよくわかりません

[Githubプルリクエストのプロセス](./開発関連/開発を開始.md)

### Issue bot

詳細については[Issue bot 使用方法](./開発関連/IssueBotの使う方.md)を参照してください。

## 謝辞

### オープンソースライブラリ

- 画像認識ライブラリ：[opencv](https://github.com/opencv/opencv.git)
- ~~テキスト認識ライブラリ：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- テキスト認識ライブラリ：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- ML Deployment: [FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- ML accelerator: [onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~ステージドロップ認識：[PenguinStats認識](https://github.com/penguin-statistics/recognizer)~~
- マップタイル認識：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSONライブラリ：[meojson](https://github.com/MistEO/meojson.git)
- C++ オペレーターパーサー：[calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64エンコードとデコード：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ 圧縮・解凍ライブラリ：[zlib](https://github.com/madler/zlib)
- C++ Gzipカプセル化ライブラリ：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- Android タッチ イベント: [minitouch](https://github.com/openstf/minitouch)
- Android タッチ イベント: [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVWフレームワーク：[Stylet](https://github.com/canton7/Stylet)
- WPFコントロールライブラリ：[HandyControl](https://github.com/HandyOrg/HandyControl) -> [HandyControls](https://github.com/ghost1372/HandyControls)
- C# JSONライブラリ：[Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json) & [System.Text.Json](https://github.com/dotnet/runtime)
- ~~ダウンローダー：[aria2](https://github.com/aria2/aria2)~~

### データソース

- ~~公開求人データ：[アークナイツツール](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~オペレーターおよび基地データ：[PRTSアークナイツ中国語WIKI](http://prts.wiki/)~~
- ステージデータ：[PenguinStatsデータ統計](https://penguin-stats.io/)
- ゲームのデータとリソース：[アークナイツのクライアント資料](https://github.com/yuanyan3060/ArknightsGameResource)
- ~~ゲームデータ：[アークナイツのゲームデータ](https://github.com/Kengxxiao/ArknightsGameData)~~

### 貢献/協力者

MAAをより良くするために開発・テストに貢献してくれたすべての方々に感謝します！ (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 免責事項

- 本ソフトウェアのロゴはAGPL 3.0ライセンスを使用しており、オープンソースではなく、[耗毛](https://weibo.com/u/3251357314)とVie、2人のアーティストと開発者がすべての権利を留保しています。「AGPL 3.0ライセンスに基づいて許可されている」という理由で、許可がない場合に本ソフトウェアのロゴを無断で使用することは禁止しております。また、許可なく商業目的で本ソフトウェアのロゴを無断で使用することも禁止しております。
- 本ソフトウェアはオープンソースで無料であり、学習と研究のみの目的としています。販売者が本ソフトウェアの料金を請求する場合は、デバイスや時間の料金である可能性があります。発生した問題と結果は、本ソフトウェアとは一切関係ありません。

## 広告

ユーザー研究グループQQグループ：[QQグループ](https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)  
ユーザー研究グループTGグループ：[Telegram](https://t.me/+Mgc2Zngr-hs3ZjU1)  
自動作戦JSON作業シェア：[prts.plus](https://prts.plus) 或いは [抄作业.com](http://抄作业.com)  
ビリビリ生放送：[ビリビリ生放送](https://live.bilibili.com/2808861) 毎晩ライブでコーディングします、最近はずっとこのソフトウェアのプログラミングをしていることが多いです。  

技術研究(アークナイツ無関係)：[インボリューション・ヘル！(QQグループ)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
開発者グループ：[QQグループ](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)  

ソフトウェアが役立つと思うなら、Star（ページの右上隅にある星）をクリックしてください。私たちにとって最高のサポートです！
