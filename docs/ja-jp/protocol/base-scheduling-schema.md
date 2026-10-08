---
order: 6
icon: material-symbols:view-quilt-rounded
---

# インフラスケジュール設定

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

`resource/custom_infrast/*.json` 各フィールドの設定方法と説明

::: tip
JSONファイルはコメントをサポートしていません。テキスト内のコメントはプレゼンテーション用にのみ使用されます。直接コピーして使用しないでください。
:::

[ビジュアルスケジュール表生成ツール](https://ark.yituliu.cn/tools/scheduleV2)

[ビジュアルスケジュール表自動生成ツール](https://ark.yituliu.cn/tools/scheduleV3)

[基地スケジュール自動生成ツール](https://riic.autos/)

## 完全なフィールドの一覧

```json
{
    "title": "サブ垢スケジュール",       // タイトル，オプション
    "description": "イロハニホヘト",      // 説明，オプション
    "plans": [
        {
            "name": "早朝組",        // プランタイトル，オプション
            "description": "lol",   // 説明，オプション
            "period": [             // シフト時間の間隔，オプション
                                    // 現在時刻がこの間隔内にある場合、プランが自動的に選択されます (jsonファイル全体が複数のプランを含む場合もあります)
                                    // このフィールドが存在しない場合は、実行のたびに自動的に次のプランに切り替わります。
                                    // core ではこのフィールドを扱わないので、maa を統合するためにインターフェイスを使用する場合は、このロジックを自分で実装してください。
                [
                    "22:00",        // 必須　記述のフォーマットは hh:mm， 日をまたぐ場合はこの例のように記述してください。
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360,        // 作業時間(分) 予約項目、未実装 将来的には、シフトを変更時間を告知させるポップアップ時間になるかも？ 自動的に変更される機能になる可能性もあり
            "Fiammetta": {          // “フィアメッタ” がどのオペレーターを配属するか、オプション、記入しない場合は配属しない。
                "enable": true,     // “フィアメッタ” を使うかな、オプション、デフォルト true
                "target": "シャマレ", // ターゲットオペレーター。ここではOCRを使用して行い、対応するクライアント言語のオペレーター名を入力する必要があります
                "order": "pre",     // 全シフト前またはシフト後に使用、オプション、引数 "pre" / "post", デフォルト "pre"
            },
            "drones": {             // ドローン使用、任意、未記入の場合はドローンを使用しない
                "enable": true,     // ドローンを使用するかどうか、オプション、デフォルト true
                "room": "trading",  // 使用する施設、引数 "trading" / "manufacturing"
                "index": 1,         // 施設番号、tab の番号に対応、引数 [1 - 5]
                "rule": "all",      // ルール、予約フィールド、未実装。後で使用する可能性がある。
                "order": "pre"      // オペレーターの交換前後の使用設定、オプション、引数 "pre" / "post", デフォルト "pre" (テキーラなど)
            },
            "groups":[              // "control" / "manufacture" / "trading" の場合、オペレーターグループを設定できます
                {
                    "name":"A",
                    "operators":[
                        "グム",
                        "シルバーアッシュ",
                        "メイ"
                    ]
                },
                {
                    "name":"B",
                    "operators":[
                        "セイリュウ",
                        "ユーネクテス",
                        "ウィーディ"
                    ]
                }
            ],
            "rooms": {              // 部屋情報，必須
                                    // 引数 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing" / "training" / "recycling"
                                    // training / recycling の A/B グループは草案であり、現在の MAA カスタムスケジュールは未対応
                                    // 1つもないということは、その施設ではシフト変更にデフォルトのアルゴリズムが使用されていることを意味します。
                                    // 部屋のシフトを変更しない場合は、skip を使用するか、タスク設定 - 基地仕事 - 基地設定 で該当施設のチェックを外すだけです。
                "control": [
                    {
                        "operators": [
                            "シー",   // OCRを使用してこれを行うには、対応するクライアント言語のオペレーター名を渡す必要があります。
                            "リィン",   // JP版なら日本語で入力しろってことですね。
                            "ケルシー",
                            "アーミヤ",
                            "ムリナール"
                        ]
                    }
                ],
                "manufacture": [
                    {
                        "operators": [
                            "フェン",
                            "シーン",
                            "クルース"
                        ],
                        "sort": false,  // ソートするかどうか（上の オペレーター の順番で）、オプション、デフォルトはfalse
                        // 例：シーン、パラス、シャマレ、などのオペレーター、そして "sort": false を使用すると、オペレーターの順序が乱れ、事前有効の効果が失われる可能性があります。
                        // "sort": true を使う、この問題を避けることができる
                    },
                    {
                        "skip": true    // 現在の施設をスキップするかどうか（配列番号に対応），オプション，デフォルトは false
                                        // true の場合、他のすべてのフィールドを空にすることができる。 オペレーター交代の操作だけが省略され、ドローンの使用や手がかりの交換など、他のことは今まで通り行われます。
                    },
                    {
                        "operators": [
                            "Castle-3"
                        ],
                        "autofill": true,   // 残った位置を自動的に埋めるためにオリジナルのアルゴリズムを使用、オプション、デフォルトは false
                                        // operators が空の場合、部屋はオリジナルのアルゴリズムで全体的にスケジュールされる
                                        // operators が空でない場合、施設全体の効率ではなく、単一のオペレータの効率のみで考慮されます。
                                        // 後でカスタムシステムと競合する可能性があることに注意してください。たとえば、後で必要な関数がここで使用される場合、注意するか、autofill を最後に置いてください
                        "product": "Battle Record"  // 現在の製造資材、オプション
                                                    // 現在の設備がジョブで設定された製品と一致しないことが確認された場合、インターフェースは赤いメッセージでポップアップ表示され、後でより便利になる可能性があります
                                                    // 使用可能引数： "Battle Record" | "Pure Gold" |  "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                    },
                    {
                        "operators": [
                            "ドロシー"
                        ],
                        "candidates": [ // サブオペレーター，オプション。埋まるまで、誰でも利用可能です
                                        // autofill=true との互換性はありません。空白でない場合、autofill は false である必要があります。
                            "アステジーニ",
                            "フィリオプシス",
                            "サイレンス"
                        ]
                    },
                    {
                        "use_operator_groups":true,  // グループ内の演算子のグループ化を使用するには true に設定します。デフォルトは false です。
                        "operators":[                // 有効にすると、演算子の名前がグループ名として解釈されます。
                            "A",                     // グループ化は気分の閾値と設定順序に従って選択されます
                            "B"                      // グループ A に気分がしきい値を下回るオペレーターがいる場合、グループ B が使用されます
                        ]
                    }
                ],
                "meeting": [
                    {
                        "autofill": true // 応接室は autofill
                    }
                ],
                "training": [ // 訓練室
                    {
                        // 人数上限、省略・空配列の意味、既存の部屋フィールドとオペレーターグループとの関係は未定
                        "operatorsA": ["アーミヤ"], // 訓練を補助するオペレーター名配列（A スロット）、任意。名前の要件は operators と同じ
                        "operatorsB": ["ドーベルマン"] // 訓練を受けるオペレーター名配列（B スロット）、任意。名前の要件は operators と同じ
                    }
                ],
                "recycling": [ // 回收站（A/B スロットを持つ予定の施設）、キー名は暫定
                    {
                        "operatorsA": ["フェン"], // 最初に解放される位置のオペレーター名配列（A スロット）、任意。名前の要件は operators と同じ
                        "operatorsB": ["クルース"] // 2 番目に解放される位置のオペレーター名配列（B スロット）、任意。名前の要件は operators と同じ
                    }
                ]
            }
        },
        {
            "name": "夜勤組"
            // ...
        }
    ]
}
```

## サンプル

[243 有効率が最も高い 一日三回](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 有効率が最も高い 一日三回](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/153_layout_3_times_a_day.json)

## スケジュール表の拡張プロトコル

MAA の基本スケジュールプロトコルには、オペレーターのシフト情報のみが含まれます。スケジュール表の拡張プロトコルは MAA のスケジュール設定を基に、任意の基地配置 `layout`、オペレーター情報 `operators`、生成元とバージョン情報 `metadata` を追加します。これにより、MAA、[**明日方舟一图流-スケジュール自動生成ツール**](https://ark.yituliu.cn/tools/scheduleV3)、[**明日方舟一图流-スケジュール収益計算ツール**](https://ark.yituliu.cn/tools/maa-schedule-calculator)**などのサードパーティーアプリケーションで**同じファイルを共有し、双方向にデータを交換できます。拡張フィールドはすべて任意で、サードパーティーアプリケーションの実際の要件に応じて調整できます。

| トップレベルのフィールド | 用途                                                                                                                         |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------------- |
| `layout`                 | 施設の種類、レベル、位置を含む基地配置を記録                                                                                 |
| `operators`              | オペレーターの昇進段階とレベルの情報を記録                                                                                   |
| `metadata`               | プロトコルのバージョン、生成ツール、モジュールのバージョンなどのデータと、サードパーティーツールが必要とする追加データを記録 |

### 拡張フィールド一覧

この例は基本プロトコルの `plans` などと組み合わせて使用してください。育成状況とバージョン番号は説明用の値です。実際の JSON からはコメントを削除してください。

```jsonc
{
    // 3 つともトップレベルの任意フィールドで、個別に指定でき、MAA の基本スケジュール実行には影響しない
    // ツールはインポート、編集、エクスポート時に拡張情報を保持する
    "layout": [ // 建造済みの基地配置全体、任意。省略は配置情報なしを意味する
        // 記載のない施設は未建造。plans[].rooms で省略した施設には基本プロトコルの既定の規則を適用
        // type：施設の種類、level：施設レベル、position：実際の位置
        { "type": "control", "level": 5 },
        // 同じ施設種類の部屋順序は plans[].rooms とドローンの index に対応
        { "type": "trading", "level": 3, "position": "B101" },
        { "type": "trading", "level": 3, "position": "B102" },
        { "type": "trading", "level": 1, "position": "B103" },
        { "type": "manufacture", "level": 3, "position": "B201" },
        { "type": "manufacture", "level": 3, "position": "B202" },
        { "type": "manufacture", "level": 2, "position": "B203" },
        { "type": "manufacture", "level": 2, "position": "B301" },
        { "type": "power", "level": 3, "position": "B302" },
        { "type": "power", "level": 3, "position": "B303" },
        { "type": "meeting", "level": 3 },
        { "type": "processing", "level": 3, "position": "B105" },
        { "type": "hire", "level": 3, "position": "B205" },
        { "type": "training", "level": 3, "position": "B305" },
        { "type": "dormitory", "level": 1, "position": "B104" },
        { "type": "dormitory", "level": 1, "position": "B204" },
        { "type": "dormitory", "level": 1, "position": "B304" },
        { "type": "dormitory", "level": 1, "position": "B404" }
    ],
    "operators": { // 育成状況、任意。ゲーム内 ID をキーとして使用
        // 部屋のオペレーター配置設定を置き換えない
        "char_002_amiya": {
            "name": "アーミヤ", // オペレーター名、任意
            "elite": 2, // 昇進段階：0、1、2
            "level": 80 // 現在の昇進段階でのレベル
        },
        "char_124_kroos": {
            "name": "クルース",
            "elite": 1,
            "level": 55
        }
    },
    "metadata": { // 生成元とバージョンなどの追加情報、任意
        "extensionVersion": "1.0", // 拡張プロトコル全体のバージョン文字列、任意。省略時はバージョン情報なし
        "generator": { // 元の生成ツールの情報
            "id": "yituliu-riic-schedule-generator",
            "name": "明日方舟一图流-排班表自动生成工具",
            "url": "https://ark.yituliu.cn/tools/scheduleV3"
        },
        "moduleVersions": { // 生成時のモジュールバージョン。識別子は生成ツールが定義し、generator.id と合わせて解釈
            "layout": "v20260924.2314", // 基地配置モジュール
            "data": "v20260924.2314", // データモジュール
            "team": "v20260928.1118", // チームモジュール
            "assembler": "v20260822.2233", // スケジュール組み立て
            "yield": "v20261002.1334", // 収益計算
            "recommendation": "v20260928.1102" // デバッグ情報
        }
    }
}
```
