---
order: 1
icon: ri:windows-fill
---

# Windows でのエミューレータ

::: tip
問題がある場合は [よくある質問](../よくある質問.md)を、先にお読みください。
:::

以下のシミュレーターの順番は、ランダムに生成されたものであり、順不同である。

<script setup>
import MarkdownIt from 'markdown-it'
import MarkdownItAnchor from 'markdown-it-anchor'

const shuffleArray = (array) => {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
}

const fullySupport = shuffleArray([
`
### ✅ [Bluestacks-CN 5](https://www.bluestacks.cn/)
サポートされています。エミュレーターの \`設定\` - \`エンジン設定\` で \`ADB接続を許可する\`をチェックする必要があります。
`,
`
### ✅ [Bluestacks 5](https://www.bluestacks.com/ja/index.html)（おすすめ👍）

サポートされています。エミュレーターの \`設定\` - \`上位設定\` で \`Android Debug Bridge\`をONにする必要があります。

- ネットワーク環境が悪い場合は [オフラインパッケージ](https://support.bluestacks.com/hc/zh-tw/articles/4402611273485-BlueStacks-5-%E9%9B%A2%E7%B7%9A%E5%AE%89%E8%A3%9D%E7%A8%8B%E5%BC%8F)をダウンロードしてください；
- adbポート番号が不規則に変化し続け、起動するたびに同じでない場合は、お使いのコンピュータが [Hyper-V](https://support.bluestacks.com/hc/ja/articles/4415238471053-BlueStacks-5-Hyper-V%E3%81%8C%E6%9C%89%E5%8A%B9%E3%81%AB%E3%81%AA%E3%81%A3%E3%81%A6%E3%81%84%E3%82%8BWindows-10%E3%81%A8Windows-11%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E3%82%B7%E3%82%B9%E3%83%86%E3%83%A0%E8%A6%81%E4%BB%B6)を有効にしている可能性があります，
    このような状況に対して、MAAはポート番号を自動的に取得する小さなバックドアを提供しています。この機能がは動作しません/複数のエミュレータが必要/複数のエミュレータコアがインストールされているの場合は [よくある質問](../よくある質問.html#blue-stackエミュレータが起動するたびにポート番号が異なるhyper-v) を参照して変更してください。Hyper-V は管理者として実行されるため、エミュレーターの自動シャットダウンや接続の自動検出など、adb を伴わない操作でも MAA を管理者として実行する必要があります。
`,
`
### ✅ [NOX](https://www.yeshen.com/)

サポートされています。
`,
`
### ✅ [NOX Android 9](https://www.yeshen.com/)

サポートされています。
`,
`
### ✅ [Nemu](https://www.xyaz.cn/)

サポートされていますが、テストはあまり行われていません。
`,
`
### ✅ [MuMu Player 12](https://www.mumuplayer.com/)（おすすめ👍）

サポートされています。

- “完了後にエミュレータを終了する”機能に異常が発生する場合がありますので、その場合はMuMu公式までご連絡ください；
- バージョン3.5.4 ~ 3.5.7 の MuMu 12 では画面キャプチャの失敗の原因となる「キープアライブ」機能が導入されているため、バージョン 3.6.4 を推奨します；バージョン 3.5.4 ~ 3.5.7 の MuMu 12をお使いの方は、MuMu 12の設定-その他で、「バックグラウンドでハングアップしている間キープアライブ」機能が無効になっていることをご確認ください。（参照[公式アナウンス](https://mumu.163.com/help/20230802/35047_1102450.html)）；
- 複数のインスタンスを開くには、MuMu 12 Multiple OpenerのADBボタンから対応するインスタンスのポート情報を確認し、MAA設定-接続設定の接続アドレスのポート番号を対応するポートに変更する必要があります。
`,
`
### ✅ [LDPlayer](https://www.ldplayer.net/)

サポートされています。

- **LDPlayer 9 はバージョン 9.0.57 以降に推奨されます；LDPlayer 5 はバージョン 5.0.67 以降に推奨されます；**
- 上記のバージョンより下では、Minitouch、MaaTouch、およびその他の効率的なタッチモードを使用するには、MAA設定 - 接続設定 で \`ADBを強制置き換え\`する必要があります。
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [Win11 WSA](https://learn.microsoft.com/ja-jp/windows/android/wsa/)

なんとか使えるレベルです。

- 需要使用 [カスタム接続](../詳細説明.html#カスタム接続) を使用する必要があります；
- WSA 2204 以降（バージョン番号はサブシステム設定の \`バージョン\` の中にあります），接続設定は \`一般モード\`を利用します；
- WSA 2203 あるいは旧版（バージョン番号はサブシステム設定の上にあります），接続設定は \`古いバージョンの WSA\`を利用します；
- このソフトウェアは 720p 以上の解像度しかサポートしていないので \`16:9\` の比率に、できるだけウインドウサイズを近づけてください。（ディスプレイのサイズが 16:9 であれば， \`F11\` で直接フルスクリーンにできます）；
- アークナイツが前面ウインドウにあることを確認し、同時に他のAndroidアプリを前面で起動していないか確認してください。そうでない場合、ゲームが一時停止したり、正しく認識されない可能性があります。；
- WSAのスクリーンショットは白い画面で撮影されることが多く、認識に異常が生じるため、使用は推奨されません。
`,
`
### ⚠️ [AVD](https://developer.android.com/studio/run/managing-avds)

理論的なサポートされています。

- Android 10 以降、SELinux が\`Enforcing\`モードの場合、Minitouch は使用できません、別のタッチモードに切り替えてください。または SELinux を **一時的に** \`Permissive\`モードに切り替え。
- AVD はデバッグ用に構築されており、ゲーム用に設計された他のエミュレーターを使用することをおすすめします。
`,
])

const notSupport = shuffleArray([
`
### 🚫 MuMu Androidアシスタント（Nebula Engine）

adbポートが開いていないのでサポートしていません。
`,
`
### 🚫 テンセントエミュレーター（GameLoop）

adbポートが開いていないのでサポートしていません。
`,
`
### 🚫 [Google Play 游戏 Beta](https://play.google.com/googleplaygames)

サポートされていません。[消費者クライアント](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)は adb ポートを開きません。
`,
])

const md = (new MarkdownIt()).use(MarkdownItAnchor, { permalink: MarkdownItAnchor.permalink.linkInsideHeader()})

const fullySupportHtml = md.render(fullySupport.join(''))
const partiallySupportHtml = md.render(particallySupport.join(''))
const notSupportHtml = md.render(notSupport.join(''))

</script>

## ✅ 完全サポート

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ 一部分のみサポート

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 未サポート

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>
