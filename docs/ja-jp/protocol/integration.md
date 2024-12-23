---
order: 1
icon: bxs:book
---

# インテグレーション

::: warning
This document is outdated due to the rapid update of the interface. Since the developers are not good at foreign languages, it is recommended that you refer to the Chinese documentation for the latest content.
:::

## API

### `AsstAppendTask`

#### Prototype

```cpp
TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### Description

タスクの追加

#### Return Value

- `TaskId`  
    以下の構成で、タスクの追加に成功した場合のタスクID;
    タスクの追加に失敗した場合は0。

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `const char* type`  
    Task type
- `const char* params`  
    Task parameters in JSON

##### List of Task Types

- `StartUp`  
    Start-up

```json5
// 対応するタスクのパラメータ
{
    "enable": bool,              // このタスクを有効にするかどうか、オプション、デフォルトは true
    "client_type": string,       // クライアントバージョン、オプション、デフォルトは空白
                                 // オプション: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "start_game_enabled": bool,  // クライアントを自動的に起動するかどうか、オプション, デフォルトはfalse
    "account_name": string       // アカウントの切り替え、オプション、デフォルトで切り替えしません
                                 // ログインしているアカウントに切り替えて、ログイン名を使用して検索し、入力内容がすべてのログインアカウント間で一意であることを確認することのみが可能です
                                 // Official: 123****4567 ならば、 123****4567、4567、123、3****4567 と入力できます
                                 // Bilibili: 文字列、上記と同じ
}
```

- `CloseDown`  
  ゲームを閉じる  

```json5
// 対応するタスクのパラメータ
{
    "enable": bool,              // このタスクを有効にするかどうか、オプション、デフォルトは true
    "client_type": string,       // クライアントのバージョンは必須です。空白を入力すると実行されません。
                                 // オプション："Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
}
```

- `Fight`  
    Operation

```json5
// 対応するタスクのパラメータ
{
    "enable": bool,             // このタスクを有効にするかどうか、オプション、デフォルトは true
    "stage": string,            // ステージ名、オプション、デフォルトは 現在 / 最終ステージ。 動作中の編集はサポートされていません。
                                // メイン章のステージがサポートされています、例 "1-7"、"S3-2"、など。
                                // レベルの最後にNormal/Hardを入力することで、標準とハードの難易度を切り替えることができます。
                                // 殲滅作戦の場合のみ、「Annihilation」を入力する必要があります。
                                // 現在のSSイベントの最後の3つのステージ、完全なステージ番号を入力する必要があります。
    "medicine": int,            // 理性回復剤の最大使用数、オプション、デフォルトは 0
    "expiring_medicine": int,   // 48 時間以内に期限切れになった理性回復剤の最大使用数、オプション、デフォルトは 0
    "stone": int,               // 純正源石の最大使用数、オプション、デフォルトは 0
    "times": int,               // 最大周回数、オプション、デフォルトは無限
    "series": int,              // 連戦回数、オプション、1~6
    "drops": {                  // ドロップ数の指定、オプション、デフォルトは指定なし
        "30011": int,           // Key: item_ID; value: 素材の数。Keyは resource/item_index.json に記載されています
        "30062": int            // OR 組み合わせ
    },
    /* 項目はOR演算子で結合され、いずれかの条件を満たしたときにタスクが停止する。 */

    "report_to_penguin": bool,  // Pengiun Stats にデータをアップロードするかどうか、オプション、デフォルトは false
    "penguin_id": string,       // Penguin Stats ID、オプション、デフォルトは空白。 `report_to_penguin` が `true` の時のみ使用可能。
    "server": string,           // サーバー、オプション、デフォルトは "CN"、ドロップ認識とアップロードに影響します
                                // 選択オプション："CN" | "US" | "JP" | "KR"
    "client_type": string,      // クライアントバージョン、オプション、デフォルトは空白。ゲームがクラッシュした後、再接続するために使用します。空白の場合この機能を無効にします。
                                // オプション: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "DrGrandet": bool,          // 純正源石を使用する際に理性を節約します、オプション、デフォルトは false。純正源石を使用の可能性がある場合にのみ機能します。
                                // 理性が1ポイント回復するまで、使用確認画面で待ち、その後ですぐに純正源石を使用することをする。
}
```

いくつかの特別ステージ名もサポートしています、[組み込み例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/ja-jp.xaml#L230)をご参照ください

- `Recruit`  
    公開求人

```json5
// 対応するタスクのパラメータ
{
    "enable": bool,             // このタスクを有効にするかどうか、オプション、デフォルトは true
    "refresh": bool,            // ★3 タグの場合更新、オプション、デフォルトは false
    "select": [                 // タグレベルのクリック、必須
        int,
        ...
    ],
    "confirm": [                // 確認するタグのレベル、必須。計算用に空配列を設定することも可能。
        int,
        ...
    ],
    "first_tags": [             // 優先タグ。タグレベルが3の場合のみ有効。 オプション、デフォルトは 空
        string,                 // ★3 タグの場合、可能な限り多くのタグが選択されます。
        ...                     // これは強制的に選択されます。つまり、「★3 タグは非選択にする」設定はすべて無視されます。
    ],
    "extra_tags_mode": int,     // その他のタグを選択、省略可能、デフォルトは 0
                                // 0 - デフォルトの動作
                                // 1 - 競合する可能性があっても3つのTagsを選択
                                // 2 - 可能であれば、競合する可能性があっても、より多くの高星Tagの組み合わせを選択します。
    "times": int,               // 採用回数、オプション、デフォルトは 0。 0 にセットすると、計算のみが実行されます。
    "set_time": bool,           // 時刻を9時間に設定するかどうか、 `times` が 0 の時のみ設定可能、オプション、デフォルトは true
    "expedite": bool,           // 緊急招集票の使用、オプション、デフォルトは false
    "expedite_times": int,      // 緊急招集票の使用回数、 `expedite` が `true` の場合のみ使用可能
                                // オプション。デフォルトでは `times` の限界まで使用します。
    "skip_robot": bool,         // ロボット タグが認識されたときにスキップするかどうか。オプション、デフォルトは skip。
    "recruitment_time": {       // タグレベルおよび設定時間（分）、オプション、デフォルトは 540 、つまり9時間
        "3": int,
        "4": int,
        ...
    },
    "report_to_penguin": bool,  // Pengiun Stats にデータをアップロードするかどうか、オプション、デフォルトは false
    "penguin_id": string,       // Penguin Stats ID、オプション、デフォルトは空白。 `report_to_penguin` が `true`の時のみ使用可能。
    "report_to_yituliu": bool,  // YITULIU にデータをアップロードするかどうか、オプション、デフォルトは false
    "yituliu_id": string,       // YITULIU user ID、オプション、デフォルトは空白。 `report_to_yituliu` が `true`の時のみ使用可能。
}
```

- `Infrast`  
    基地シフト

```json5
{
    "enable": bool,         // タスクを有効にするかどうか、オプション、デフォルトは true
    "mode": int,            // シフトモード、オプション、デフォルトは 0。
                            // 0 - デフォルトのシフト モード、単一施設の最適解
                            // 10000 - シフト変更モードをカスタマイズし、ユーザー構成を読み取り、プロトコルドキュメント/インフラストシフト.md を参照してください
    "facility": [           // シフト対象施設（順序付け）、必須。動作中の編集はサポートされていません.
        string,             // 施設名: "Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // ドローンの使用, オプション, デフォルトは "_NotUse"
                            // mode==10000でこのフィールドは無効です（無視されます）
                            // "_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
    "threshold": float,     // 宿舎に移動させる体力のしきい値 [0, 1.0]、オプション、デフォルトは 0.3
    "replenish": bool,      // 源石の欠片を自動で補充する、オプション、デフォルトは false
    "dorm_notstationed_enabled": bool, // 寮の「入居していない」オプションを有効にするかどうか、オプション、デフォルトfalse
    "dorm_trust_enabled": bool, // 寮の残りの場所を信頼未満のオペレーターに記入するかどうか、オプション、デフォルトfalse

    /* 次のパラメータは、mode=10000でのみ有効になります。そうしないと無視されます */
    "filename": string,     // カスタム構成パス、必須。実行中の設定はサポートされていません
    "plan_index": int,      // 構成内の方案順序番号を使用します、必須です。実行中の設定はサポートされていません
}
```

- `Mall`  
    FPの回収と自動購入  
    最初に `buy_first` にあるものを左から右に押して順番に一度購入し、次に左から右に2回購入して`blacklist` を回避し、FPオーバーフローの場合はブラックリストを無視して、オーバーフローしなくなるまで左から右に3回目に購入します。

```json5
// 対応するタスクのパラメータ
{
    "enable": bool,         // このタスクを有効にするかどうか、オプション、デフォルトは true
    "shopping": bool,       // 購買所から購入するかどうか、オプション、 デフォルトは false。 動作中に変更はできません。
    "buy_first": [          // アイテム購入の優先度、オプション。動作中に変更はできません。
        string,             // アイテム名。例 "招聘许可" (Recruitment Permit/求人票)、"龙门币" (LMD/龍門幣)、その他
        ...
    ],
    "blacklist": [          // ブラックリスト、オプション。動作中に変更はできません。
        string,             // アイテム名。例 "加急许可" (Expedited Plan/緊急招集票)、"家具零件" (Furniture Part/家具パーツ)、その他
        ...
    ]
    "only_buy_discount": bool // 割引対象のアイテムだけを購入するかどうかは。これは2回目の購入の際にのみ適用されます。デフォルトは false。
    "reserve_max_credit": bool // クレジットポイントが300未満になった場合に購入を停止するかどうかは。これは2回目の購入の際にのみ適用されます。デフォルトは false。
}
```

- `Award`  
    報酬の受け取り  

```json5
// 対応するタスクのパラメータ
{
    "enable": bool          // このタスクを有効にするかどうか、オプション、デフォルトは true
}
```

- `Roguelike`  
    統合戦略  

```json5
// 対応するタスクパラメータ
{
    "enable": bool,  // このタスクを有効にするかどうか、省略可能、デフォルト値 true
    "theme": string, // テーマ、省略可能、デフォルト値 "Phantom"
                     //   Phantom - 傀影と猩紅の血晶
                     //   Mizuki  - 水月と蒼青の樹
                     //   Sami    - 探索者の銀霜の果て
                     //   Sarkaz  - サルカズの無尽の奇譚
    "mode": int,     // モード、省略可能、デフォルト値 0
                     //   0 - ポイント稼ぎ、より安定して層数を増やす
                     //   1 - 源石錠稼ぎ、1層で投資後終了
                     //   2 - 【廃止済み】モード0と1を兼ね備え、投資後に終了、投資なしで続行
                     //   3 - 開発中…
                     //   4 - 開幕リセット、難易度0で3層に到達後リセットし、指定難易度で開幕リセットを狙う。アイテムが「湯沸かしポット」や「希望」以外の場合は難易度0に戻って再挑戦；
                     //       Phantomテーマでは難易度変更せず、現在の難易度で3層到達後リセット、開幕リセットを試行
                     //   5 - 崩壊パラダイム稼ぎ；Samiテーマのみ対応；戦闘で敵を漏らすなどして崩壊値を早く溜める。
                     //       初遭遇の崩壊パラダイムが expected_collapsal_paradigms のリストにあればタスクを停止し、なければリセット
    "squad": string,                // 開幕部隊名、省略可能、デフォルト値 "指揮部隊"
    "roles": string,                // 開幕職業グループ、省略可能、デフォルト値 "得意を活かす"
    "core_char": string,            // 開幕オペレーター名、省略可能；単一のオペレーター**日本語名**のみ対応、サーバー関係なく；空欄または "" に設定した場合はレベルに応じて自動選択
    "use_support": bool,            // 開幕オペレーターがサポートかどうか、省略可能、デフォルト値 false
    "use_nonfriend_support": bool,  // フレンド以外のサポートが使用可能かどうか、省略可能、デフォルト値 false；use_support が true の場合にのみ有効
    "starts_count": int,               // 探索を開始する回数、省略可能、デフォルト値 INT_MAX；到達後自動でタスクを停止
    "difficulty": int,                 // 難易度を指定、省略可能、デフォルト値 0；**Phantom以外**のテーマにのみ対応；
                                       // 未解放の場合は、現在解放されている最高難易度を選択
    "stop_at_final_boss": bool,        // 5層の最終ボスノードでタスクを停止するかどうか、省略可能、デフォルト値 false；**Phantom以外**のテーマにのみ対応
    "investment_enabled": bool,        // 源石錠を投資するかどうか、省略可能、デフォルト値 true
    "investments_count": int,          // 源石錠の投資回数、省略可能、デフォルト値 INT_MAX；到達後自動でタスクを停止
    "stop_when_investment_full": bool, // 投資上限に達した時自動でタスクを停止するかどうか、省略可能、デフォルト値 false
    "start_with_elite_two": bool,      // 開幕リセットと同時にエリート2昇格を狙うか、省略可能、デフォルト値 false；モード4のみ対応
    "only_start_with_elite_two": bool, // 開幕エリート2昇格のみ狙い、他の条件を無視するか、省略可能、デフォルト値 false；
                                       // モードが4で、start_with_elite_two が true の場合にのみ有効
    "refresh_trader_with_dice": bool,  // サイコロでショップをリフレッシュし、特定のアイテムを購入するか、省略可能、デフォルト値 false；Mizukiテーマのみ対応、指路鱗を狙う
    "first_floor_foldartal": string,   // 1層の遠見段階で取得を希望する密文、省略可能；Samiテーマにのみ対応、モード問わず；取得成功した場合はタスクを停止
    "start_foldartal_list": [          // 開幕リセット時に取得を希望する密文リスト、省略可能、デフォルト値 []；Samiテーマでモード4の場合にのみ対応；
        string,                        // 開幕でリスト内の全ての密文を持っている場合に開幕リセット成功とする；
        ...                            // 注意、このパラメータは「生活至上部隊」と同時に使用する必要がある。他の部隊では開幕報酬で密文を取得できない；
    ],
    "use_foldartal": bool,                    // 密文を使用するか、モード5ではデフォルト値 false、他のモードではデフォルト値 true；Samiテーマにのみ対応
    "check_collapsal_paradigms": bool,        // 取得した崩壊パラダイムを検査するか、モード5ではデフォルト値 true、他のモードではデフォルト値 false
    "double_check_collapsal_paradigms": bool, // 崩壊パラダイムの検査漏れ対策を行うか、モード5ではデフォルト値 true、他のモードではデフォルト値 false；
                                              // テーマが Sami で、check_collapsal_paradigms が true の場合にのみ有効
    "expected_collapsal_paradigms": [         // 希望する崩壊パラダイム、デフォルト値 ["少しの無視", "片目を閉じる", "画像が壊れる", "真っ暗"]；
        string,                               // テーマが Sami でモードが5の場合にのみ対応
        ...
    ]
}
```

- `Copilot`  
    自動戦闘  

```json5
{
    "enable": bool,             // このタスクを有効にするかどうか、オプション、デフォルトは true
    "filename": string,         // タスクのJSONのファイル名とパス、絶対/相対 パスのサポート。動作中に変更はできません。
    "formation": bool           // "自動編成" を行うかどうか、オプション、デフォルトは false。動作中に変更はできません。
}
```

自動操縦JSONの詳細については、[自動戦闘API](./copilot-schema.md)を参照してください

- `SSSCopilot`  
  保全駐在の自動戦闘  

```json5
{
    "enable": bool,             // このタスクを有効にするかどうか、オプション、デフォルトは true
    "filename": string,         // タスクのJSONのファイル名とパス、絶対/相対 パスのサポート。動作中に変更はできません。
    "loop_times": int           // ループの実行回数
}
```

保全駐在の自動操縦JSONの詳細については、[保全駐在API](./sss-schema.md)

- `Depot`
    倉庫アイテム認識

```json5
// 対応するタスクのパラメータ
{
    "enable": bool          // このタスクを有効にするかどうか, オプション, デフォルトは true
}
```

- `OperBox`  
    カドレー識別  

```json5
// 対応するタスクのパラメータ
{
    "enable": bool          // このタスクを有効にするかどうか, オプション, デフォルトは true
}
```

- `Reclamation`  
    生息演算  

```json5
{
    "enable": bool,
    "theme": string,            // テーマ、オプション、デフォルトは 1
                                // Fire  - *砂中の火*
                                // Tales - *沙洲遺聞*
    "mode": int,                // モード、オプション、デフォルトは 0
                                // 0 - すぐにバトルをやめることでバッジと建設ポイントを周回
                                // 1 - *砂中の火*：水を買ってから基地で鍛造して粗製純金を周回
                                //     *沙洲遺聞*：アイテムを自動的に製造してロードして通貨を稼ぐ
    "tools_to_craft": [
        string,                 // 自動的に製造されるアイテム、オプション、デフォルトは螢光棒
        ...
    ] 
                                // サブストリングを入力することをお勧めします
    "increment_mode": int,      // クリックタイプ、オプション、デフォルトは0
                                // 0 - 連続クリック
                                // 1 - 長押し
    "num_craft_batches": int    // 一度の最大製造バッチ数、オプション、デフォルトは 16
}
```

- `Custom`  
  カスタム タスク  

```json5
{
    "enable": bool,
    "task_names": [     // 配列内の最初の一致(および次のnextなど)でタスクを実行します。
                        // 複数のミッションを実行したい場合は、append Custom task を数回使用できます
        string,
        ...
    ]
}
```

- `SingleStep`  
  シングル ステップ タスク（現在は戦闘でのみ利用可能）  

```json5
{
    "enable": bool,
    "type": string,     // 現在、 "copilot" のみがサポートされています
    "subtask": string,  // "stage" | "start" | "action"
                        // "stage" レベル名を設定する、 "details": { "stage": "xxxx" } を必要とする
                        // "start" 作戦を開始、 details なし
                        // "action": シングルステップのオプションの場合、詳細は戦闘協定の単一のアクションである必要があります
                        //           例："details": { "name": "スルト", "location": [ 4, 5 ], "direction": "左" }，詳細については、[自動戦闘API](./自動戦闘API.md)を参照してください
    "details": {
        ...
    }
}
```

- `VideoRecognition`  
  ビデオ認識、現在は作戦ビデオのみ対応  

```json5
{
    "enable": bool,
    "filename": string, // ビデオのファイルパス、絶対パス、相対パスのどちらでもかまいません。動作中に変更はできません。
}
```

### `AsstSetTaskParams`

#### Prototype

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);
```

#### Description

タスク パラメーターの設定

#### Return Value

- `bool`  
    パラメータが正常に設定されたかどうか。  

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `TaskId task`  
    Task ID, `AsstAppendTask` の値を返します
- `const char* params`  
    JSONのタスクパラメーター, `AsstAppendTask` と同じ  
    "動作中に変更はできません" と記載されていないフィールドはランタイム中に変更することができます. そうでなければ、タスクの実行中にこれらの変更は無視される.

### `AsstSetStaticOption`

#### Prototype

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### Description

プロセスレベルパラメータの設定

#### Return Value

- `bool`  
    設定が成功したかどうか

#### Parameter Description

- `AsstStaticOptionKey key`  
    key
- `const char* value`  
    value

##### List of Key and value

None

### `AsstInstanceOptionKey`

#### Prototype

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### Description

インスタンス レベルのパラメータの設定

#### Return Value

- `bool`  
    設定が成功したかどうか

#### Parameter Description

- `AsstHandle handle`  
    handle
- `AsstInstanceOptionKey key`  
    key
- `const char* value`  
    value

##### List of Key and value

```cpp
    enum InstanceOptionKey
    {
        Invalid = 0,
        // 破棄されました // MinitouchEnabled = 1,   // Minitouch を有効にするかどうか
                                // 開いていても必ず使えるわけではなく、設備がサポートされていない可能性があるなど
                                // "1" - on，"0" - off
        TouchMode = 2,          // タッチモード、デフォルトは minitouch
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // パウスにオペレーターを部署する、自動戦闘やローグや保全駐在に影響を与える
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // Adblite を使用するかどうか， "0" | "1"
        KillAdbOnExit = 5,       // 終了時に ADB プロセスを強制終了するかどうか， "0" | "1"
    };
```
