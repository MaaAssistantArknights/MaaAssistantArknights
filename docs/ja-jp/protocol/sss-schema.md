---
order: 7
icon: game-icons:prisoner
---

# 保全駐在プロトコル

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

::: tip
JSONファイルはコメントをサポートしていません。テキスト内のコメントはプレゼンテーション用にのみ使用されます。直接コピーして使用しないでください。
:::

```json
{
    "type": "SSS",                         // プロトコルタイプ、SSSは保全駐在を表し、必須、変更不可
    "stage_name": "ドッソレス建設現場",     // 保全駐在ステージの名前、必須
    "minimum_required": "v4.9.0",          // 最低限必要なMAAのバージョン、必須
    "doc": {                               // 説明、オプション
        "title": "低スキル要件で成功率の高い操作",
        "title_color": "dark",
        "details": "低いスキル要件なんて...", // ここにあなたの名前(著者の名前)、ビデオガイドへのリンクなどを書くことをお勧めします
        "details_color": "dark"
    },
    "buff": "自适应补给元件",                // 開始の拡張式戦術ユニットの選択、オプション
                                           // テキスト認識に使用されるこのタイプのコンテンツは、中国語の名前を使用することが推奨されており、その効果は他のクライアントにも適用されます
    "equipment": [                         // 水平にカウントされる戦術装備の選択、オプション
                                           // 現在のバージョンでは実装されておらず、インタフェース上の表示のみです
        "A", "A", "A", "A",
        "B", "B", "B", "B"
    ],
    "strategy": "优选策略",                 // または「自由な戦略」、オプションです
                                           // 現在のバージョンでは実装されておらず、インタフェース上の表示のみです
    "opers": [                             // 指定されたオペレータ、オプション
        {
            "name": "Thorns",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {                          // 各職業の必要な残りの人員数、コストでソートされています、オプション
                                           // 現在のバージョンでは実装されておらず、インタフェース上の表示のみです
        "Pioneer": 13,                     // 中英両方の名前が受け入れられます
        "Guards": 2,
        "Medic": 2
    },
    "drops": [                        // 戦闘開始時および戦闘中にオペレータを採用し、装備を入手する優先順位
        "Silence",
        "Exusiai",                    // オペレーターの名前、職業の名前をサポートします
        "Vanguard",
        "Support",
        "无需增调干员",                // 誰も採用しない場合
        "重整导能组件",                // 装備の名前をサポートし、一緒に書かれます
        "反制导能组件",
        "战备激活阀",                  // レベルの途中でのオプション装備、ここにも置かれます
        "改派发讯器",
    ],
    "blacklist": [ // ブラックリスト、オプション。これらの人々は採用されません。
                   // 後続バージョンでは、編成の人々もこれらの人々を選ばないようにします。
        "Midnight",
        "Mayer"
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上",       // 単一レベルステージ名、必須
                                           // 名前、stageId、levelId、おすすめのstageIdまたはlevelIdをサポートします
                                           // 他の保全ステージと競合するコード（例：LT-1）は使用しないでください

            "strategies": [                // 必須
                                         // 順番に従って各オブジェクトのtool_menを展開します。
                                         // 現在の手にtool_menが存在しない場合、次のオブジェクトを展開します。
                {
                    "core": "Thorns",
                    "tool_men": {
                        "Pioneer": 1,     // 中英両方の名前が受け入れられます
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [10, 1],
                    "direction": "Left"
                },
                {
                    "core": "Mudrock",
                    "tool_men": {
                        "Pioneer": 1,
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [2, 8],
                    "direction": "Left"
                },
                {
                    // "core" が提供されていない場合、カードを引くために補助タイプを展開するために使用できます。
                    "tool_men": {
                        "Support": 100
                    },
                    "location": [2, 8],
                    "direction": "Left"
                }
            ],
            "draw_as_possible": true,   // 「支援要請」ボタン、利用可能な場合に使用するかどうか。オプション、デフォルトはtrueです。
            "actions": [
                // オプション
                // 自動戦闘のロジックを再利用します、自動戦闘API を参照してください。
                // action の条件が満たされると、 action が実行されます。それ以外の場合は、上記の戦略が実行されます。
                {
                    "type": "调配干员" // 新しいタイプ。「支援要請」ボタンをクリックします。 "draw_as_possible" が true の場合は無効です。
                },
                {
                    "type": "CheckIfStartOver", // 新しいタイプ。オペレータが利用可能かどうかを確認します。利用できない場合は終了して再起動します。
                    "name": "Thorns"
                },
                {
                    "name": "Firewatch",
                    "location": [4, 5],
                    "direction": "Left"
                },
                {
                    "kills": 10,
                    "type": "Retreat",
                    "name": "Firewatch"
                }
            ],
            "retry_times": 3 // バトルが失敗した場合の再試行回数。この制限を超えると、ステージ全体を諦めます。
        },
        {
            "stage_name": "见者有份"
            // ...
        }
        // プレイしたいステージはいくつでも書くことができます。たとえば、4だけ書かれている場合、ステージ4を完了した後に自動的に再起動します。
    ]
}
```

## サンプルファイル

[SSS_阿卡胡拉丛林.json](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/SSS_阿卡胡拉丛林.json)

[SSS_多索雷斯在建地块.json](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/SSS_多索雷斯在建地块.json)
