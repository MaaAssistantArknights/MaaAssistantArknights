---
order: 2
icon: material-symbols:u-turn-left
---

# コールバック図式

::: info 注意  
This document is outdated due to the rapid update of the interface. Since the developers are not good at foreign languages, it is recommended that you refer to the Chinese or English documentation for the latest content  
:::

::: info 注意  
コールバックメッセージがバージョンとともに更新される高速反復では、本書は古くなる可能性があります。最新のコンテンツを入手するには、[C#統合ソースコード](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/src/MaaWpfGui/Main/AsstProxy.cs) を参照してください。  
:::

## Prototype

```cpp
typedef void(ASST_CALL* AsstApiCallback)(AsstMsgId msg, const char* details_json, void* custom_arg);
```

なお、`AsstMsgId` は `int32_t` のエイリアスです。

## 概要

- `AsstMsgId msg`  
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

      /* Web Request */
      ReportRequest    = 30000,   // レポートリクエスト
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
- `MuMuExtrasInputStatus`  
  MuMu タッチ強化の実際の有効状態、`details` 構造：
  - `available` (boolean, required): 有効になったかどうか。
  - `deferred` (boolean, required): まだ判定されていないかどうか（接続時にゲームがまだ描画されておらず、描画開始後に自動的に再判定されます）。
- `ResolutionGot`  
  解像度を取得しました
- `ResolutionChanged`  
  実行中に解像度が変更され、接続が無効化され現在のタスクが中断されます、`details` 構造：
  - `width` (number, required): 現在の幅。
  - `height` (number, required): 現在の高さ。
- `FastestWayToScreencap`  
  最速のスクリーンショット方式が見つかりました、`details` 構造：
  - `method` (string, required): 最速のスクリーンショット方式。
  - `cost` (number, required): 所要時間（ミリ秒）。
  - `alternatives` (array`<object>`, required): 全候補方式とその所要時間。

- `ScreencapCost`  
  スクリーンショット所要時間統計（10回ごとに報告）、`details` 構造：
  - `min` (number, required): 最小所要時間（ミリ秒）。
  - `max` (number, required): 最大所要時間（ミリ秒）。
  - `avg` (number, required): 平均所要時間（ミリ秒）。
  - `fault_times` (number): 失敗回数（失敗がある場合のみ存在）。

- `EmulatorFPS`  
  エミュレータリフレッシュレート（1分ごとに検出）、`details` 構造：
  - `fps` (number, required): エミュレータ/システムのリフレッシュレート（FPS）。
  - `refresh_period_ns` (number, required): フレーム更新周期（ナノ秒）。

### AsyncCallInfo

```json
{
    "uuid": string,             // デバイス固有コード，UUID
    "what": string,             // コールバック タイプ，"Connect" | "AttachWindow" | "Click" | "Screencap" | ...
    "async_call_id": int,       // 非同期要求 id、つまり AsstAsyncXXX を呼び出したときの戻り値
    "details": {
        "ret": bool,            // 実際に呼び出された戻り値
        "cost": int64,          // 経過時間、単位ミリ秒
        "error": string,        // 未処理例外の種類、未処理例外により呼び出しが失敗した場合にのみ存在
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // 最後の一連のタスク
    "taskid": int,                  // 最後の一連のタスクに対応するタスク ID
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
- `ParadoxCopilot`  
  自動逆理演算作戦
- `Depot`  
  倉庫の識別
- `OperBox`  
  オペレーターボックス識別
- `Reclamation`  
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

`TaskChainError` / `TaskChainStart` / `TaskChainCompleted` / `TaskChainExtraInfo` / `TaskChainStopped` は以下のフィールドを共有します：

```json
{
    "taskchain": string,            // 現在の一連タスク
    "taskid": int,                  // 現在のタスク ID
    "uuid": string                  // UUID
}
```

なお、`TaskChainError` はタスクチェーンが未処理の例外によって中断された場合、追加で `details` フィールドを伴います：

- `error` (string, required): 例外の種類。`OpenCVException` | `OutOfMemory` | `GpuDeviceRemoved` | `UnhandledException` | `UnknownException` のいずれか。このうち `OutOfMemory` と `GpuDeviceRemoved`（GPU 推論デバイスのロスト、グラフィックドライバーの更新や GPU のリセットなど）が発生した場合、残りのタスクは破棄され、続行するにはプログラムの再起動が必要です。

### TaskChainExtraInfo

デフォルトでは上記の共通フィールドのみを伴います。ローグライク（界園テーマ）で経路計画の結果、避けられない戦闘が多すぎると判定されて自ら再開する場合、メッセージに追加で以下が含まれます：

- `what` (string, required): `RoutingRestart` に固定。
- `why` (string, required): `TooManyBattlesAhead` に固定。
- `node_cost` (number, required): 計画された次ノードのコスト。

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
      "task": "StartButton2",         // タスク名
      "action": "ClickSelf",          // アクション名、例：`ClickSelf` | `DoNothing` | `Swipe`
      "exec_times": 1,                // 実行回数
      "max_times": 999,               // 最大実行回数
      "algorithm": "MatchTemplate",   // 識別アルゴリズム名、例：`MatchTemplate` | `OcrDetect` | `FeatureMatch` | `JustReturn`
      "result": {}                    // 今回の識別結果、アルゴリズムごとに構造が異なる。識別結果がない場合は空オブジェクト
  }
  ```

  なお、タスクの実行回数が上限に達した場合、まず `what` が `ExceededLimit` である `SubTaskExtraInfo` メッセージが送信され、`details` には `task`（タスク名）、`exec_times`（実行回数）、`max_times`（最大実行回数）が含まれます。

  認識対象のタスクリストに未定義のタスク名が存在する場合（タスクリソースとプログラムのバージョン不一致など）、認識開始前に `SubTaskError` として送信され、タスクチェーン全体が停止されます。`what` は `UnknownTask`、`details` には `tasks`（文字列配列、未定義のタスク名のリスト）が含まれます。

- `ReportToPenguinStats` / `ReportToYituliu`  
  戦闘ドロップをペンギン急便統計 / Yituliu ビッグデータへ報告します（報告失敗時は `SubTaskError` で通知されます）。
- `StartGameTask`  
  クライアントの起動に失敗（設定ファイルと渡された `client_type` が不一致）。
- Todo Other

##### 多用される `task` フィールドの値

- `StartButton2`  
  開始
- `MedicineConfirm`  
  理性回復剤使用確認
- `StoneConfirm`  
  純正源石使用確認
- `RecruitRefreshConfirm`  
  公開求人リストの更新確認
- `RecruitConfirm`  
  公開求人の確認
- `RecruitNowConfirm`  
  緊急招集票の使用確認
- `InfrastDormDoubleConfirmButton`  
  基地の二重確認ボタン、配置予定のオペレーターが既に他の施設に配属中の場合にのみ表示されます（MAAが自動でクリックします）
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
- `StageCombatOpsEnter`  
  統合戦略: 作戦
- `StageEmergencyOps`  
  統合戦略: 緊急作戦
- `StageDreadfulFoe`  
  統合戦略: 悪路凶敵
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
      ],
      "cur_times": 2, // 作戦結果画面で認識された連戦回数（認識された場合のみ存在）
      "annihilation_weekly_process": [1600, 1800] // 殲滅作戦の今週の獲得進捗、形式: [完了数, 上限]（殲滅作戦のみ存在）
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

- `RecruitPreservedTag`  
  保留対象の公開求人タグの検出

  ```json
  // 対応する詳細フィールドの例
  {
     "tag": "ロボット"
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

- `RecruitPermitReserved`
  現在の求人票数が星3募集用に設定された保留数以下のため、現在のスロットをスキップします。

  ```json
  // 対応する details フィールドの例
  {
      "current": 8     // 現在の求人票数
  }
  ```

- `RecruitPermitCountRecognitionFailed`
  星3用の求人票保留が有効な状態で求人票数の認識に失敗したため、現在のスロットをスキップします。`details` は空です。

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

- `RecruitError`  
  公開求人認識時のエラー。`details` は空。現在のスロットの更新回数が上限に達したことが原因の場合は、`refresh_limit`（スロットの更新回数上限）を含みます。

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

- `CustomInfrastRoomKeptUnchanged`  
  カスタム基建のシフトで、この部屋が現状維持された際に通知されます。`details` は常に `facility`（施設名）と `index`（施設 ID）を含み、さらに：

  ```json
  // 対応する詳細フィールドの例
  {
      "reason": "candidates_working"  // 部屋を現状維持した理由、下記参照
  }
  ```

  `reason` の値：
  - `facility_empty`：カスタム設定の rooms 配列が空。
  - `room_no_intent`：部屋の設定はあるが、配置の意思がない。
  - `candidates_unavailable`：候補オペレーターが全員利用不可。名簿の誤り（表記間違いや未所持）の可能性が高い。
  - `candidates_working`：候補オペレーターが全員他の施設で勤務中。
  - `candidates_low_mood`：候補オペレーターの気分が全員閾値未満。

- `CustomInfrastRoomsNotConfigured`  
  カスタム設定の rooms 配列がカバーしない施設は、現状維持されます。`details` はデフォルトの `facility`（施設名）と `index`（施設 ID）のみを含みます。

- `CustomInfrastTraineeNotFound`  
  カスタム基建のシフトで、訓練枠の候補オペレーターが誰も一致しませんでした。`details` は常に `facility`（施設名）と `index`（施設 ID）を含み、さらに：

  ```json
  // 対応する詳細フィールドの例
  {
      "candidates": ["エイヤフィヤトラ", "焔影リード"]  // 訓練枠の候補オペレーター一覧
  }
  ```

- `CustomInfrastAssistantNotFound`  
  カスタム基建のシフトで、補助者の候補オペレーターが誰も一致しませんでした。`details` は常に `facility`（施設名）と `index`（施設 ID）を含み、さらに：

  ```json
  // 対応する詳細フィールドの例
  {
      "candidates": ["ケルシー"],          // 補助者の候補オペレーター一覧
      "working_excluded": ["テキーラ"],    // 任意、他の施設で勤務中のため除外された候補、空でない場合のみ含まれる
      "low_mood_excluded": ["スルト"]      // 任意、気分が閾値未満のため除外された候補、空でない場合のみ含まれる
  }
  ```

- `CustomInfrastRoomGroupsMatchFailed`  
  カスタム基建のシフトで、オペレーターグループの一致に失敗しました。`details` は常に `facility`（施設名）と `index`（施設 ID）を含み、さらに：

  ```json
  // 対応する詳細フィールドの例
  {
      "reason": "all_unavailable",  // 一致失敗の理由、下記参照
      "groups": ["古+銀"]           // 任意、未定義または利用不可のグループ名一覧、`group_not_defined` と `all_unavailable` の場合のみ含まれる
  }
  ```

  `reason` の値：
  - `group_not_defined`：参照されたグループ名がいずれも groups で定義されていない。
  - `all_unavailable`：すべてのグループのオペレーターが利用不可。
  - `recognition_failed`：オペレーターリストの認識に失敗。

- `CopilotPrecheckFile`  
  マルチ作業モードで ｢Yituliu のデータで編隊を支援｣ が有効な場合、各作業の事前マッチングの前に通知されます。`details` の内容：

  ```json
  // 対応する詳細フィールドの例
  {
      "nav_name": "CV-EX-1",  // ステージ名
      "filename": "作業.json" // 作業ファイル名
  }
  ```

- `StageInfo`  
  自動戦闘ステージの情報

  ```json
  // 対応する詳細フィールドの例
  {
      "name": string,  // ステージ名
      "size": number   // マップのグリッド数
  }
  ```

- `StageInfoError`  
  自動戦闘ステージの情報エラー

- `DepotInfo`  
  倉庫のアイテムの認識結果

  ```json
  // 対応する詳細フィールドの例
  "done": bool,   // 認識が完了したかどうか，false はまだ進行中かどうか（処理中のデータ）
  "data": "{\"2001\":18000,\"31043\":317}"  // JSON文字列、形式: {"アイテムID": 数量, ...}
  ```

- `OperBoxInfo`  
  オペレーターボックス識別結果

  ```json
  // 対応する詳細フィールドの例
  "done": bool,       // 認識が完了したかどうか，false はまだ進行中かどうか（処理中のデータ）
  "all_opers": [
      {
          "id": "char_002_amiya",
          "name": "阿米娅",
          "name_en": "Amiya",
          "name_jp": "アーミヤ",
          "name_kr": "아미야",
          "name_tw": "阿米婭",
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
  自動作戦で、サポートされていないレベル名。`details` の内容：

  ```json
  // 対応する詳細フィールドの例
  {
      "level": string  // サポートされていないレベル名
  }
  ```

- `BlackFlowStartRun`  
  BlackFlow の初期報酬稼ぎ（ローグライクモード 30003）の進捗。（機械翻訳）

  ```json
  // 対応する詳細フィールドの例
  {
      "status": "reward_missed",       // 現在の進捗、下記参照
      "rewards": ["空间租赁", "金乌"]   // 任意、目標の探索の支援選択画面で認識した報酬名
  }
  ```

  `status` の値：
  - `preparing`：準備用の探索を開始。
  - `target_run`：目標の探索を開始。
  - `reward_missed`：支援選択画面で報酬は認識できたが狙うものがないため、その探索を放棄して準備用の探索をやり直す。
  - `reward_picked`：`blackflow_start_utopia` が有効で、支援選択画面に狙う報酬が出たため、`blackflow_start_reward_priority` の順に報酬を選び、第 1 層に入って实托邦か確認する。
  - `completed`：狙う報酬が出たため、テーマ画面へ退出してその探索を保持し、タスクを終了。`blackflow_start_utopia` が有効な場合は第 1 層が实托邦だったことを示す。
  - `reward_unrecognized`：支援選択画面の報酬を認識できないため、その画面に留まってタスクを停止。
  - `reward_page_missing`：目標の探索で支援選択画面が出なかったため、タスクを停止。
  - `reward_recognition_failed`：準備用または目標の探索で支援選択画面の報酬選択に失敗したため、タスクを停止。

  `rewards` は `reward_missed`、`reward_picked`、`completed`、`reward_unrecognized`、`reward_recognition_failed` のときのみ通知され、`blackflow_start_rewards` が 0 で `blackflow_start_utopia` が無効（報酬を認識しない）の場合や 1 つも認識できなかった場合は省略されます。報酬名は簡体字中国語で出力されます。

- `BlackFlowScrapTradeAborted`  
  BlackFlow の行商取引で異常が発生し、取引を中止します。`details` の内容：

  ```json
  // 対応する詳細フィールドの例
  {
      "reason": "sale_failed",  // 取引を中止した理由
      "item": "晶体元件"        // 中止のきっかけとなったアイテム名
  }
  ```

### ReportRequest

このフィールドは、コアモジュールがネットワークリクエスト情報をUI層に渡すために主に使用され、UIは実際のHTTP報告処理を担当します。

```json
{
  "url": "string",            // リクエストの完全なURL、例：https://penguin-stats.io/PenguinStats/api/v2/report
  "headers": {                // リクエストヘッダーのキーと値のペア（Content-Typeは含まず、UIが追加します）
    "authorization": "PenguinID 1234567890",
    "User-Agent": "MaaAssistantArknights/1.2.3 cpr/4.5.6"
  },
  "body": "string",           // リクエストボディの内容（通常はJSON形式の文字列）
  "subtask": "string"         // サブタスク名、具体的な報告タスクを識別します。例："ReportToPenguinStats"、"ReportToYituliu"
}
```
