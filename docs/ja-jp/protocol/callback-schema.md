---
order: 2
icon: material-symbols:u-turn-left
---

# コールバック図式

::: info 注意
This document is outdated due to the rapid update of the interface. Since the developers are not good at foreign languages, it is recommended that you refer to the Chinese or English documentation for the latest content
:::

::: info 注意
コールバックメッセージがバージョンとともに更新される高速反復では、本書は古くなる可能性があります。最新のコンテンツを入手するには、[C#統合ソースコード](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs) を参照してください。
:::

## Prototype

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## 概要

- `int msg`  
    The message type

    ```cpp
    enum class AsstMsg
    {
        /* Global Info */
        InternalError     = 0,      // 内部エラー
        InitFailed        = 1,      // 初期化に失敗しました
        ConnectionInfo    = 2,      // 接続情報
        AllTasksCompleted = 3,      // すべてのタスクが完了したかどうか
        AsyncCallInfo     = 4,      // 外部非同期呼び出し情報

        /* TaskChain Info */
        TaskChainError     = 10000, // 一連のタスク 実行/認識のエラー
        TaskChainStart     = 10001, // 一連のタスク 開始
        TaskChainCompleted = 10002, // 一連のタスク 完了
        TaskChainExtraInfo = 10003, // 一連のタスクの追加情報
        TaskChainStopped   = 10004, // 一連のタスク 手動停止

        /* SubTask Info */
        SubTaskError     = 20000,   // サブタスク 実行/認識におけるエラー
        SubTaskStart     = 20001,   // サブタスク 実行
        SubTaskCompleted = 20002,   // サブタスク 完了
        SubTaskExtraInfo = 20003,   // サブタスクの追加情報
        SubTaskStopped   = 20004,   // サブタスク 手動停止
    };
    ```

- `const char* details`  
    メッセージの詳細, JSON. 詳細: [Field Description](#field-description)
- `void* custom_arg`  
    呼び出し元のカスタム引数には、 `AsstCreateEx` インターフェースの `custom_arg` 引数が渡される。C ライクな言語では、`this` ポインタを一緒に渡すことができる。

## Field Description

### InternalError

Todo

### InitFailed

```json
{
    "what": string,     // Error タイプ
    "why": string,      // Error 理由
    "details": object   // Error 詳細
}
```

### ConnectionInfo

```json
{
    "what": string,     // Info タイプ
    "why": string,      // Info 理由
    "uuid": string,     // UUID (接続に失敗した場合は空白)
    "details": {
        "adb": string,     // AsstConnect インターフェースの adb_path 引数
        "address": string, // AsstConnect インターフェースの address 引数
        "config": string   // AsstConnect インターフェースの config 引数
    }
}
```

### 多用される `What` フィルドの値

- `ConnectFailed`  
    接続失敗
- `Connected`  
    接続成功。現段階では `uuid` フィールドが空であることに注意してください (次のステップで取得されます)
- `UuidGot`  
    UUID の取得
- `UnsupportedResolution`  
    この解像度はサポートされていません
- `ResolutionError`  
    解像度を取得できない
- `Reconnecting`  
    接続切断 (adb/emulator クラッシュ), 再接続開始
- `Reconnected`  
    接続切断 (adb/emulator クラッシュ), 再接続成功
- `Disconnect`  
    接続切断 (adb/emulator クラッシュ), 再接続失敗
- `ScreencapFailed`  
    画面取得失敗 (adb/emulator クラッシュ), 再接続失敗
- `TouchModeNotAvailable`  
    サポートされていないタッチモード

### AsyncCallInfo

```json
{
    "uuid": string,             // デバイス固有コード，UUID
    "what": string,             // コールバック タイプ，"Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // 非同期要求 id、つまり AsstAsyncXXX を呼び出したときの戻り値
    "details": {
        "ret": bool,            // 実際に呼び出された戻り値
        "cost": int64,          // 経過時間、単位ミリ秒
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // 最後の一連のタスク
    "uuid": string,                 // UUID
    "finished_tasks": [             // 最終動作タスクの ID
        int,
        ...
    ]
}
```

#### 多用される `taskchain` フィールドの値

- `StartUp`  
    ゲーム開始
- `CloseDown`  
    ゲームを閉じる
- `Fight`  
    作戦
- `Mall`  
    FPとFP交換所に買い物
- `Recruit`  
    自動公開求人
- `Infrast`  
    基地施設
- `Award`  
    デイリー報酬を受け取る
- `Roguelike`  
    統合戦略
- `Copilot`  
    自動作戦
- `SSSCopilot`  
    自動保全駐在作戦
- `Depot`  
    倉庫の識別
- `OperBox`  
    オペレーターボックス識別
- `ReclamationAlgorithm`  
    生息演算
- `Custom`  
    カストム タスク
- `SingleStep`  
    サブタスク
- `VideoRecognition`  
    ビデオ認識タスク
- `Debug`  
    デバッグ

### TaskChain 関連情報

```json
{
    "taskchain": string,            // 現在の一連タスク
    "taskid": int,                  // 現在のタスク ID
    "uuid": string                  // UUID
}
```

### TaskChainExtraInfo

Todo

### SubTask 関連情報

```json
{
    "subtask": string,             // サブタスク名
    "class": string,               // サブタスククラス
    "taskchain": string,           // 現在の一連のタスク
    "taskid": int,                 // 現在のタスク ID
    "details": object,             // 詳細
    "uuid": string                 // UUID
}
```

#### 多用される `subtask` フィールドの値

- `ProcessTask`  

    ```json
    // 対応する詳細フィールドの例
    {
        "task": "StartButton2",     // タスク名
        "action": 512,
        "exec_times": 1,            // 実行回数
        "max_times": 999,           // 最大実行回数
        "algorithm": 0
    }
    ```

- Todo Other

##### 多用される `task` フィールドの値

- `StartButton2`  
    開始
- `MedicineConfirm`  
    理性回復剤使用確認
- `ExpiringMedicineConfirm`  
    48時間以内に期限が切れる理性回復剤使用確認
- `StoneConfirm`  
    純正源石使用確認
- `RecruitRefreshConfirm`  
    公開求人リストの更新確認
- `RecruitConfirm`  
    公開求人の確認
- `RecruitNowConfirm`  
    緊急招集票の使用確認
- `ReportToPenguinStats`  
    ペンギン急便への報告
- `ReportToYituliu`  
    Yituliu へビッグデータの報告
- `InfrastDormDoubleConfirmButton`  
    基地施設での二重確認は、他のオペレーターとの競合がある場合のみ発生します
- `StartExplore`  
    統合戦略: 開始
- `StageTraderInvestConfirm`  
    統合戦略: 源石錐とアイテム交換
- `StageTraderInvestSystemFull`  
    統合戦略: 投資満額
- `ExitThenAbandon`  
    統合戦略: 終了確認
- `MissionCompletedFlag`  
    統合戦略: ミッション完了
- `MissionFailedFlag`  
    統合戦略: ミッション失敗
- `StageTraderEnter`  
    統合戦略: 怪しい旅商人
- `StageSafeHouseEnter`  
    統合戦略: 安全な片隅
- `StageEncounterEnter`  
    統合戦略: 思わぬ遭遇
- `StageCombatDpsEnter`  
    統合戦略: 作戦
- `StageEmergencyDps`  
    統合戦略: 緊急作戦
- `StageDreadfulFoe`  
    統合戦略: 悪路凶敵
- `StartGameTask`
    クライアントの起動に失敗 (client_type と設定ファイルの互換性なし)
- Todo Other

### SubTaskExtraInfo

```json
{
    "taskchain": string,           // 一連のタスク
    "class": string,               // サブタスクのクラス
    "what": string,                // Information タイプ
    "details": object,             // Information 詳細
    "uuid": string,                // UUID
}
```

#### 多用される `what` と `details` フィールドの値

- `StageDrops`  
    ステージドロップインフォメーション

    ```json
    // 対応する詳細フィールドの例
    {
        "drops": [              // 今回のドロップされた素材
            {
                "itemId": "3301",
                "quantity": 2,
                "itemName": "アーツ学1"
            },
            {
                "itemId": "3302",
                "quantity": 1,
                "itemName": "アーツ学2"
            },
            {
                "itemId": "3303",
                "quantity": 2,
                "itemName": "アーツ学3"
            }
        ],
        "stage": {              // レベル情報
            "stageCode": "CA-5",
            "stageId": "wk_fly_5"
        },
        "stars": 3,             // ステージクリア評価
        "stats": [              // この実行中にドロップされた素材の総量
            {
                "itemId": "3301",
                "itemName": "アーツ学1",
                "quantity": 4,
                "addQuantity": 2 // 今回の新規ドロップ数
            },
            {
                "itemId": "3302",
                "itemName": "アーツ学2",
                "quantity": 3,
                "addQuantity": 1
            },
            {
                "itemId": "3303",
                "itemName": "アーツ学3",
                "quantity": 4,
                "addQuantity": 2
            }
        ]
    }
    ```

- `RecruitTagsDetected`  
    採用タグの検出

    ```json
    // 対応するフィールドの詳細
    {
        "tags": [
            "COST回復",
            "防御",
            "先鋒タイプ",
            "補助タイプ",
            "近距離"
        ]
    }
    ```

- `RecruitSpecialTag`  
    特別な採用タグの検出

    ```json
    // 対応する詳細フィールドの例
    {
        "tag": "上級エリート"
    }
    ```

- `RecruitResult`  
    公開求人結果

    ```json
    // 対応する詳細フィールドの例
    {
        "tags": [                   // 全てのタグ、今のところは5つに違いない
            "弱化",
            "減速",
            "術師タイプ",
            "補助タイプ",
            "近距離"
        ],
        "level": 4,                 // 総合的なレアリティ
        "result": [
            {
                "tags": [
                    "弱化"
                ],
                "level": 4,         // レアリティに対応するタグ
                "opers": [
                    {
                        "name": "プラマニクス",
                        "level": 5  // レアリティに対応するオペレーター
                    },
                    {
                        "name": "メテオリーテ",
                        "level": 5
                    },
                    {
                        "name": "ワイフー",
                        "level": 5
                    },
                    {
                        "name": "ヘイズ",
                        "level": 4
                    },
                    {
                        "name": "メテオ",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "減速",
                    "術師タイプ"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "ナイトメア",
                        "level": 5
                    },
                    {
                        "name": "グレイ",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "弱化",
                    "術師タイプ"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "ヘイズ",
                        "level": 4
                    }
                ]
            }
        ]
    }
    ```

- `RecruitTagsRefreshed`  
    公開求人タグの更新

    ```json
    // 対応する詳細フィールドの例
    {
        "count": 1,               // スロットが更新された回数
        "refresh_limit": 3        // 更新最大回数
    }
    ```

- `RecruitNoPermit`  
    求人票が切れた

    ```json
    // 対応する詳細フィールドの例
    {
        "continue": true,   // 更新を続けるかどうか
    }
    ```

- `RecruitTagsSelected`  
    公開求人タグの選択

    ```json
    // 対応する詳細フィールドの例
    {
        "tags": [
            "減速",
            "術師タイプ"
        ]
    }
    ```

- `RecruitSlotCompleted`  
    公開求人スロットの完了

- `RecruitError`  
    公開求人認識時のエラー

- `EnterFacility`  
    施設へ入る

    ```json
    // 対応する詳細フィールドの例
    {
        "facility": "Mfg",  // 施設名
        "index": 0          // 施設 ID
    }
    ```

- `NotEnoughStaff`  
    オペレーター不足

    ```json
    // 対応する詳細フィールドの例
    {
        "facility": "Mfg",  // 施設名
        "index": 0          // 施設 ID
    }
    ```

- `ProductOfFacility`  
    施設の生産

    ```json
    // 対応する詳細フィールドの例
    {
        "product": "Money", // 生産物
        "facility": "Mfg",  // 施設名
        "index": 0          // 施設 ID
    }
    ```

- `StageInfo`  
    自動戦闘ステージの情報

    ```json
    // 対応する詳細フィールドの例
    {
        "name": string  // ステージ名
    }
    ```

- `StageInfoError`  
    自動戦闘ステージの情報エラー

- `PenguinId`  
    PenguinStats ID

    ```json
    // 対応する詳細フィールドの例
    {
        "id": string
    }
    ```

- `DepotInfo`  
    倉庫のアイテムの認識結果

    ```json
    // 対応する詳細フィールドの例
    "done": bool,       // 認識が完了したかどうか，false はまだ進行中かどうか（処理中のデータ）
    "arkplanner": {     // https://penguin-stats.cn/planner
        "object": {
            "items": [
                {
                    "id": "2004",
                    "have": 4,
                    "name": "上級作戦記録"
                },
                {
                    "id": "mod_unlock_token",
                    "have": 25,
                    "name": "モジュールデータ"
                },
                {
                    "id": "2003",
                    "have": 20,
                    "name": "中級作戦記録"
                }
            ],
            "@type": "@penguin-statistics/depot"
        },
        "data": "{\"@type\":\"@penguin-statistics/depot\",\"items\":[{\"id\":\"2004\",\"have\":4,\"name\":\"上級作戦記録\"},{\"id\":\"mod_unlock_token\",\"have\":25,\"name\":\"モジュールデータ\"},{\"id\":\"2003\",\"have\":20,\"name\":\"中級作戦記録\"}]}"
    },
    "lolicon": {     // https://arkntools.app/#/material
        "object": {
            "2004" : 4,
            "mod_unlock_token": 25,
            "2003": 20
        },
        "data": "{\"2003\":20,\"2004\": 4,\"mod_unlock_token\": 25}"
    }
    // 現在は ArkPlanner と Lolicon (Arkntools) 形式のみ対応、今後対応するサイトが増える可能性あり
    ```

- `OperBoxInfo`  
    オペレーターボックス識別結果

    ```json
    // 対応する詳細フィールドの例
    "done": bool,       // 認識が完了したかどうか，false はまだ進行中かどうか（処理中のデータ）
    "all_oper": [
        {
            "id": "char_002_amiya",
            "name": "阿米娅",
            "own": true
        },
        {
            "id": "char_003_kalts",
            "name": "凯尔希",
            "own": true
        },
        {
            "id": "char_1020_reed2",
            "name": "焰影苇草",
            "own": false
        },
    ]
    "own_opers": [
        {
            "id": "char_002_amiya", // オペレーターID
            "name": "阿米娅",        // 氏名、中国語で出力
            "own": true,            // 持っているかどうか
            "elite": 2,             // 昇進段階 0, 1, 2
            "level": 50,            // レベル
            "potential": 6,         // 潜在 [1, 6]
            "rarity": 5             // レア度 [1, 6]
        },
        {
            "id": "char_003_kalts",
            "name": "凯尔希",
            "own": true,
            "elite": 2,
            "level": 50,
            "potential": 1,
            "rarity": 6
        }
    ]
    ```

- `UnsupportedLevel`  
    自動作戦で、サポートされていないレベル名
