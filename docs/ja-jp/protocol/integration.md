---
order: 1
icon: bxs:book
---

# インテグレーション

## インターフェース紹介

### `AsstAppendTask`

#### インターフェース プロトタイプ

```cpp
AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### インターフェースの説明

タスクを追加します。

#### 返回値

- `AsstTaskId`  
   追加に成功した場合、タスク ID を返します。後続のタスク パラメータ設定に使用できます；  
   追加に失敗した場合、0 を返します。

#### パラメータ説明

:::: field-group  
::: field handle  
@type AsstHandle
@required
インスタンス ハンドル  
:::  
::: field type  
@type const char*
@required
タスク タイプ  
:::  
::: field params  
@type const char*
@required
タスク パラメータ、JSON 文字列  
:::  
::::

##### タスク タイプ一覧

- `StartUp`  
  ゲーム起動

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field client_type  
@type string
@required
クライアント バージョン。  
<br>
オプション：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field start_game_enabled  
@type boolean
@default false
@optional
クライアントを自動的に起動するかどうか。  
:::  
::: field account_name  
@type string
@optional
アカウントを切り替えます。デフォルトでは切り替えません。  
<br>
ログイン済みのアカウントへの切り替えのみ可能で、ログイン名で検索し、入力内容がすべてのログイン済みアカウント間で一意であることを確認してください。  
<br>
Official：`123****4567`、入力可能：`123****4567`、`4567`、`123`、`3****4567`  
<br>
Bilibili：`张三`、入力可能：`张三`、`张`、`三`  
<br>
繁体字中国語サーバー：Eメール形式（例：`ab****01@gmail.com`）。アスタリスクを含まない明文部分（例：`01@gmail`）の入力を推奨  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official",
   "start_game_enabled": true,
   "account_name": "123****4567"
}
```

</details>

- `CloseDown`  
  ゲームを閉じます。

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field client_type  
@type string
@required
クライアント バージョン。空白を入力すると実行されません。  
<br>
オプション：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official"
}
```

</details>

- `Fight`  
  理智作战

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field stage  
@type string
@optional
ステージ名。デフォルトは空で、現在/最後のステージを認識します。実行中の設定はサポートされていません。  
ナビゲーション対象のステージは以下の通りです：

- すべてのメイン ストーリー ステージ。ステージ末尾に `-NORMAL` または `-HARD` を付けて難易度を切り替えできます（10〜14章は標準/厄難、15章以降は通常/険地）。
- 龍門幣、作戦記録の 5/6 関。ただし `CE-6` / `LS-6` と入力する必要があります。MAA は 6 関が代理できない場合は自動的に 5 関に切り替わります。
- スキル本、購買資格証、碳素材の第 5 関。`CA-5` / `AP-5` / `SK-5` と入力する必要があります。
- すべてのチップ本。完全なステージ番号を入力する必要があります（例：`PR-A-1`）。
- 殲滅モードは以下の入力値をサポートし、対応する Value を使用する必要があります：
  - 当期殲滅：Annihilation
  - チェルノボーグ：Chernobog@Annihilation
  - 龍門郊外：LungmenOutskirts@Annihilation
  - 龍門市街：LungmenDowntown@Annihilation
- 別伝の OF-1 / OF-F3。
- 当期 SS イベントの最後の 3 ステージ。[API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivityV2.json) にアクセスして、サポートされているステージ リストを取得できます。[tasks.json](https://api.maa.plus/MaaAssistantArknights/api/resource/tasks.json) ファイルのイベント ステージ ナビゲーションを追加でロードする必要があります。
- 復刻 SS イベント。`SSReopen-<ステージ プレフィックス>` と入力します。例えば `SSReopen-IC` と入力すると、XX-1～XX-9 ステージを一度に完了できます。
  :::  
  ::: field medicine  
  @type number
  @default 0
  @optional
  理性回復剤の最大使用数。  
  :::  
  ::: field medicine_expire_days  
  @type number
  @default 0
  @optional
  指定日数以内に期限切れになる理性回復剤を使用します。`0` は期限切れの理性回復剤を使用しないことを意味します。  
  :::  
  ::: field expiring_medicine  
  @type number
  @default 0
  @optional
  @deprecated
  v6.8.0 で非推奨になりました。代わりに `medicine_expire_days` を使用してください。  
  :::  
  ::: field stone  
  @type number
  @default 0
  @optional
  純正源石の最大使用数。  
  :::  
  ::: field times  
  @type number
  @default 2147483647
  @optional
  戦闘回数。  
  :::  
  ::: field series  
  @type number
  @default 1
  @optional
  連戦回数。-1～10。
  <br>
  `-1` は切り替えを無効にします。
  <br>
  `0` は現在利用可能な最大回数に自動的に切り替わります。現在の理智が最大回数未満の場合は、利用可能な最小回数を選択します。
  <br>
  `1～10` は指定した連戦回数です。
  <br>
  ::: info サーバー差異
  入力検証はリソースに `FightSeries-OldMethodFlag` が存在するかどうかで決まります：
  <br>
  - 新リスト（中国版 2026/8/1 以降の主リソース、当該 flag なし）：`-1～10` を受け付け
  - 旧リスト（海外リソースに当該 flag あり）：`-1～6` のみ受け付け、それより大きい値は拒否

  海外サーバーは約半年後に追従予定で、その際に上限はリソースに合わせて 10 になります。Windows GUI の連戦回数ドロップダウンは現在固定で 10 まで表示されます。海外で手動で 7～10 を選ぶと、タスク投入時に Core に拒否されます。
  :::  
  ::: field drops  
  @type object
  @optional
  ドロップ数を指定します。デフォルトで指定なし。キーは item_id、値は数量です。キーは `resource/item_index.json` ファイルを参照できます。  
  <br>
  例：`{ "30011": 10, "30062": 5 }`  
  <br>
  上記はすべて OR 関係です。つまり、いずれかに達するとタスクが停止します。  
  :::  
  ::: field report_to_penguin  
  @type boolean
  @default false
  @optional
  ペンギン統計にレポートするかどうか。  
  :::  
  ::: field penguin_id  
  @type string
  @optional
  ペンギン統計レポート ID。デフォルトは空です。`report_to_penguin` が true の場合のみ有効です。  
  :::  
  ::: field report_to_yituliu  
  @type boolean
  @default false
  @optional
  一图流にレポートするかどうか。  
  :::  
  ::: field yituliu_id  
  @type string
  @optional
  一图流レポート ID。デフォルトは空です。`report_to_yituliu` が true の場合のみ有効です。  
  :::  
  ::: field server  
  @type string
  @default CN
  @optional
  サーバー。ドロップ認識とアップロードに影響します。
  <br>
  オプション：`CN` | `US` | `JP` | `KR`  
  :::  
  ::: field client_type  
  @type string
  @optional
  クライアント バージョン。デフォルトは空です。ゲームがクラッシュした場合にクライアントを再起動して接続し直すために使用されます。空の場合、この機能は有効になりません。
  <br>
  オプション：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
  :::  
  ::: field DrGrandet  
  @type boolean
  @default false
  @optional
  理智節約源石モード。源石効果が発生する可能性がある場合にのみ有効です。
  <br>
  源石確認画面で待機し、現在の 1 ポイントの理智回復が完了するまで待ってから、すぐに源石を使用します。  
  :::  
  ::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "stage": "1-7",
   "medicine": 1,
   "medicine_expire_days": 2,
   "stone": 0,
   "times": 10,
   "series": 0,
   "drops": {
      "30011": 10
   },
   "report_to_penguin": true,
   "penguin_id": "123456",
   "report_to_yituliu": true,
   "yituliu_id": "123456",
   "server": "CN",
   "client_type": "Official",
   "DrGrandet": false
}
```

</details>

- `Recruit`  
  公開求人

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field refresh  
@type boolean
@default false
@optional
3 星タグを更新するかどうか。  
:::  
::: field select  
@type array<number>
@required
クリックするタグレベル。  
:::  
::: field confirm  
@type array<number>
@required
確認するタグレベル。計算のみの場合は、空配列に設定できます。  
:::  
::: field first_tags  
@type array<string>
@optional
優先タグ。タグレベルが 3 の場合のみ有効です。デフォルトは空です。
<br>
タグレベルが 3 の場合、ここのタグがあれば可能な限り多くを選択します（存在する場合）。強制選択であり、「3 星タグを選択しない」設定はすべて無視されます。  
:::  
::: field extra_tags_mode  
@type number
@default 0
@optional
その他のタグを選択します。
<br>
`0` - デフォルト動作
<br>
`1` - 3 つのタグを選択します（競合の可能性あり）
<br>
`2` - 可能な限り、より多くの高星タグの組み合わせを選択します（競合の可能性あり）  
:::  
::: field times  
@type number
@default 0
@optional
何回採用するか。計算のみの場合は 0 に設定できます。  
:::  
::: field set_time  
@type boolean
@default true
@optional
採用時間制限を設定するかどうか。`times` が 0 の場合のみ有効です。  
:::  
::: field expedite  
@type boolean
@default false
@optional
緊急招集票を使用するかどうか。  
:::  
::: field expedite_times  
@type number
@optional
緊急招集の回数。`expedite` が true の場合のみ有効です。現在のバージョンでは機能せず、緊急招集は回数制限なしで `times` の上限まで使用されます。  
:::  
::: field skip_robot  
@type boolean
@default true
@optional
非推奨です。旧パラメータ互換のためにのみ残されています。  
<br>
`preserve_tags` が指定されておらず、この値が `true` の場合は `支援机械` を認識したときのみスキップします。`元素` は旧来の 1★ タグとしては扱われません。  
:::
::: field preserve_tags  
@type array<string>
@optional
現在の公開求人枠を保持したまま今回の募集をスキップしたい Tag 名の一覧です。デフォルトは空です。  
<br>
指定した Tag のいずれかを認識した場合、MAA はその枠を保持して今回の募集をスキップします。  
:::  
::: field recruitment_time  
@type object
@optional
タグレベル（3 以上）と対応する希望採用時間（分単位）。デフォルト値は 540（つまり 09:00:00）です。
<br>
例：`{ "3": 540, "4": 540 }`  
:::  
::: field report_to_penguin  
@type boolean
@default false
@optional
ペンギン統計にレポートするかどうか。  
:::  
::: field penguin_id  
@type string
@optional
ペンギン統計レポート ID。デフォルトは空です。`report_to_penguin` が true の場合のみ有効です。  
:::  
::: field report_to_yituliu  
@type boolean
@default false
@optional
一图流にレポートするかどうか。  
:::  
::: field yituliu_id  
@type string
@optional
一图流レポート ID。デフォルトは空です。`report_to_yituliu` が true の場合のみ有効です。  
:::  
::: field server  
@type string
@default CN
@optional
サーバー。アップロードに影響します。
<br>
オプション：`CN` | `US` | `JP` | `KR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "refresh": true,
   "select": [5, 4],
   "confirm": [4, 3],
   "first_tags": ["高级资深干员"],
   "extra_tags_mode": 1,
   "times": 4,
   "set_time": true,
   "expedite": false,
   "expedite_times": 0,
   "preserve_tags": ["支援机械"],
   "recruitment_time": {
      "3": 540,
      "4": 540
   },
   "report_to_penguin": false,
   "penguin_id": "123456",
   "report_to_yituliu": false,
   "yituliu_id": "123456",
   "server": "CN"
}
```

</details>

- `Infrast`  
  基地シフト

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field mode  
@type number
@default 0
@optional
シフト作業モード。
<br>
`0` - `Default`：デフォルト シフト モード。施設内および施設横断を含む効率の高いオペレーターコンビを自動計算します。
<br>
`10000` - `Custom`：カスタム シフト モード。ユーザー構成を読み込みます。[基地スケジューリング プロトコル](./base-scheduling-schema.md)を参照してください。
<br>
`20000` - `Rotation`：ワンキー ローテーション モード。制御中枢、発電所、宿舎、および事務室をスキップします。他の施設はシフトを変更しませんが、基本的な操作は保持されます（ドローン使用、応接室ロジックなど）。  
:::  
::: field facility  
@type array<string>
@required
シフト対象施設。実行中の設定はサポートされていません。
<br>
`mode = 0` の場合、この配列は有効化セットとして扱われ、順序と重複はスケジューリングに影響しません（交代順序はアルゴリズムが自動的に決定します）。`mode = 10000` / `20000` の場合は配列の順序で処理されます。
<br>
施設名：`Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field drones  
@type string
@default \_NotUse
@optional
ドローン使用目的。`mode = 10000` の場合、このフィールドは無効です。
<br>
オプション：`_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field threshold  
@type number
@default 0.3
@optional
作業心情しきい値。範囲は [0, 1.0] です。
<br>
`mode = 10000` の場合、このフィールドは「自動入力」にのみ有効です。
<br>
`mode = 20000` の場合、このフィールドは無効です。  
:::  
::: field replenish  
@type boolean
@default false
@optional
貿易所の「源石の欠片」を自動補充するかどうか。  
:::  
::: field dorm_notstationed_enabled  
@type boolean
@default false
@optional
宿舎の「未配置」オプションを有効にするかどうか。  
:::  
::: field dorm_trust_enabled  
@type boolean
@default false
@optional
宿舎の残りの位置を信頼が満たされていないオペレーターで追加するかどうか。  
:::  
::: field fiammetta_targets  
@type array<string>
@default ["清流", "可露希尔", "但书"]
@optional
フィアメッタの回復対象リスト。交代開始時、リスト内で現在の体力が最も低い対象オペレーターがフィアメッタとともに寮へ配置されて体力を交換します。`mode = 0` かつ `fiammetta_recovery_enabled` が true の場合のみ有効です。
<br>
オプション：`清流` | `可露希尔` | `但书` | `巫恋` | `龙舌兰` | `歌蕾蒂娅`（オプション外または重複するエントリは無視されます）  
:::  
::: field fiammetta_recovery_enabled  
@type boolean
@default false
@optional
交代開始時にフィアメッタで回復対象の体力を回復するかどうか。無効の場合、交代は宿舎準備ステップをスキップします。`mode = 0` の場合のみ有効です。  
:::  
::: field use_pinus_sylvestris  
@type boolean
@default false
@optional
｢レッドパイン騎士団｣の施設横断コンボを有効にするかどうか。`mode = 0` の場合のみ有効です。  
:::  
::: field use_perception_information  
@type boolean
@default false
@optional
｢感知情報｣の施設横断コンボを有効にするかどうか。｢俗世の憂い｣より優先されます。`mode = 0` の場合のみ有効です。  
:::  
::: field use_worldly_plight  
@type boolean
@default false
@optional
｢俗世の憂い｣の施設横断コンボを有効にするかどうか。`mode = 0` の場合のみ有効です。  
:::  
::: field use_abyssal_hunter  
@type boolean
@default false
@optional
｢アビサルハンター｣の施設横断コンボを有効にするかどうか。`mode = 0` の場合のみ有効です。｢レッドパイン騎士団｣と同時に有効化した場合、両者は同時に交代対象になりません。  
:::  
::: field reception_message_board  
@type boolean
@default true
@optional
応接室の伝言板FPを受け取るかどうか。  
:::  
::: field reception_clue_exchange  
@type boolean
@default true
@optional
情報共有を実施するかどうか。  
:::  
::: field reception_send_clue  
@type boolean
@default true
@optional
手がかりを譲り渡すかどうか。  
:::  
::: field filename  
@type string
@required
カスタム構成パス。実行中の設定はサポートされていません。
<br>
<Badge type="warning" text="mode = 10000 の場合のみ有効" />  
:::  
::: field plan_index  
@type number
@required
構成で使用する計画シーケンス番号。実行中の設定はサポートされていません。
<br>
<Badge type="warning" text="mode = 10000 の場合のみ有効" />  
:::  
::: field continue_training  
@type boolean
@default false
@optional
訓練室で未完了の専門化トレーニングを続行するかどうか。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "mode": 0,
   "facility": ["Mfg", "Trade", "Reception"],
   "drones": "PureGold",
   "threshold": 0.3,
   "replenish": true,
   "dorm_notstationed_enabled": false,
   "dorm_trust_enabled": true,
   "reception_message_board": true,
   "reception_clue_exchange": true,
   "reception_send_clue": true,
   "filename": "schedules/base.json",
   "plan_index": 1
}
```

</details>

- `Mall`  
  FP受け取りと商店購物。  
  まず `buy_first` にあるものを順番に 1 回購入し、次に左から右に 2 回購入して `blacklist` を回避し、FP溢出時はブラックリストを無視して左から右に 3 回購入して溢出しなくなるまで続けます。

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field visit_friends  
@type boolean
@default true
@optional
フレンドの基地にアクセスしてFPを獲得するかどうか。  
:::  
::: field shopping  
@type boolean
@default true
@optional
購物するかどうか。  
:::  
::: field buy_first  
@type array<string>
@default []
@optional
優先購買リスト。商品名、例：`"招聘许可"`、`"龙门币"` など。  
:::  
::: field blacklist  
@type array<string>
@default []
@optional
購物ブラックリスト。商品名、例：`"加急许可"`、`"家具零件"` など。  
:::  
::: field force_shopping_if_credit_full  
@type boolean
@default false
@optional
FP溢出時にブラックリストを無視するかどうか。  
:::  
::: field only_buy_discount  
@type boolean
@default false
@optional
割引品のみを購入するかどうか。2 回目の購買にのみ適用されます。  
:::  
::: field reserve_max_credit  
@type boolean
@default false
@optional
FP ポイントが 300 未満の場合、購買を停止するかどうか。2 回目の購買にのみ適用されます。  
:::  
::: field credit_fight  
@type boolean
@default false
@optional
サポートを借りて OF-1 を 1 回クリアし、翌日により多くのFPを獲得するかどうか。  
:::  
::: field formation_index  
@type number
@default 0
@optional
OF-1 実行時に使用する編成スロットのインデックス。
<br>
0～4 の整数。0 は現在の編成を意味し、1～4 は第 1～第 4 編成を表します。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "visit_friends": true,
   "shopping": true,
   "buy_first": ["招聘许可", "龙门币"],
   "blacklist": ["家具零件"],
   "force_shopping_if_credit_full": false,
   "only_buy_discount": true,
   "reserve_max_credit": false,
   "credit_fight": false,
   "formation_index": 0
}
```

</details>

- `Award`  
  各種報酬の受け取り

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field award  
@type boolean
@default true
@optional
毎日/週間任務の報酬を受け取るかどうか。  
:::  
::: field mail  
@type boolean
@default false
@optional
すべてのメール報酬を受け取るかどうか。  
:::  
::: field recruit  
@type boolean
@default false
@optional
限定ガチャから送信される毎日の無料 1 回引きを受け取るかどうか。  
:::  
::: field orundum  
@type boolean
@default false
@optional
ラッキー ウォール/おみくじの合成玉報酬を受け取るかどうか。  
:::  
::: field mining  
@type boolean
@default false
@optional
期限付き採掘許可の合成玉報酬を受け取るかどうか。  
:::  
::: field specialaccess  
@type boolean
@default false
@optional
5 周年から送信された月パス報酬を受け取るかどうか。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "award": true,
   "mail": true,
   "recruit": true,
   "orundum": false,
   "mining": true,
   "specialaccess": false
}
```

</details>

- `Roguelike`  
  統合戦略

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field theme  
@type string
@default Phantom
@optional
テーマ。
<br>
`Phantom` - ファントムと緋き貴石
<br>
`Mizuki` - ミヅキと紺碧の樹
<br>
`Sami` - 探索者と銀氷の果て
<br>
`Sarkaz` - サルカズの炉辺奇談
<br>
`JieGarden` - 歳の界園志異
<br>
`BlackFlow` - 黒流樹海  
:::  
::: field mode  
@type number
@default 0
@optional
モード。
<br>
`0` - ポイント稼ぎ、より安定して層数を増やす。
<br>
`1` - 源石錐を稼ぎ、第 1 層投資後に終了。
<br>
`2` - <Badge type="danger" text="削除済み" /> かつてはモード 0 と 1 を兼ね備え、投資後に終了、投資なしで続行でしたが、現在のバージョンでは指定すると拒否されます。
<br>
`3` - <Badge type="danger" text="未開放" /> 指定すると拒否されます。
<br>
`4` - 開局リセット、難易度 0 で第 3 層に到達後リセット、指定難易度で開局リセット報酬を狙う。最初に遭遇した報酬が「湯沸かしポット」または「希望」以外の場合、難易度 0 に戻って再挑戦します。Phantom テーマでは難易度を変更せず、現在の難易度で第 3 層到達後リセット、開局リセットを試行します。
<br>
`5` - パラダイムロストを稼ぎます。Sami テーマのみに対応します。戦闘漏れなどで崩壊値の蓄積を加速化し、最初に遭遇したパラダイムロストが `expected_collapsal_paradigms` リストにある場合はタスクを停止、そうでなければリセット。
<br>
`6` - 月次小隊を稼ぎ、モード 0 と同じですがモード固有の適応あり。
<br>
`7` - 多面調査を稼ぎ、モード 0 と同じですがモード固有の適応あり。
<br>
`10001` - 第 1 層を素早く通過。Sarkaz テーマのみ対応。
<br>
`20001` - 常楽ノードを稼ぎます。第 1 層で洞窟に入り、必要なノードが見つからなければリセット。JieGarden テーマ専用で、`find_playTime_target` との併用が必要です。
<br>
`30001` - 襁褓動物の入手。BlackFlow テーマ専用。
:::  
::: field squad  
@type string
@default 指挥分队
@optional
開局分隊名。  
:::  
::: field roles  
@type string
@default 取长补短
@optional
開局職業グループ。  
:::  
::: field core_char  
@type string
@optional
開局オペレーター名。単一のオペレーター**中国語名**のみ対応、サーバー関係なし。空欄または空文字列 `""` の場合は練度に応じて自動選択。  
:::  
::: field use_support  
@type boolean
@default false
@optional
開局オペレーターがサポートかどうか。  
:::  
::: field use_nonfriend_support  
@type boolean
@default false
@optional
フレンド以外のサポートが使用可能かどうか。`use_support` が true の場合のみ有効。  
:::  
::: field starts_count  
@type number
@default 2147483647
@optional
探索を開始する回数。到達後自動でタスクを停止。  
:::  
::: field difficulty  
@type number
@default -1
@optional
難易度を指定。`-1` は指定なし。未解放の場合は、現在解放されている最高難易度を選択。  
:::  
::: field stop_at_final_boss  
@type boolean
@default false
@optional
第 5 層の最終ボス ノードの前でタスクを停止するかどうか。**Phantom 以外**のテーマのみに対応。  
:::  
::: field stop_at_max_level  
@type boolean
@default false
@optional
統合戦略レベルが最大に達した場合にタスクを停止するかどうか。  
:::  
::: field investment_enabled  
@type boolean
@default true
@optional
源石錐を投資するかどうか。  
:::  
::: field investments_count  
@type number
@default 2147483647
@optional
源石錐の投資回数。到達後自動でタスクを停止。  
:::  
::: field stop_when_investment_full  
@type boolean
@default false
@optional
投資が上限に達した場合に自動でタスクを停止するかどうか。  
:::  
::: field investment_with_more_score  
@type boolean
@default false
@optional
投資後に購物を試みるかどうか。モード 1 のみに適用。  
:::  
::: field start_with_elite_two  
@type boolean
@default false
@optional
開局リセットと同時にエリート 2 昇格を狙うか。モード 4 のみに対応。  
:::  
::: field only_start_with_elite_two  
@type boolean
@default false
@optional
開局エリート 2 昇格のみを狙い、他の条件を無視するか。モード 4 で `start_with_elite_two` が true の場合のみ有効。  
:::  
::: field refresh_trader_with_dice  
@type boolean
@default false
@optional
ダイスでショップをリフレッシュして特定の商品を購入するかどうか。Mizuki テーマのみに対応、ミチビキリンジュウ（指路鱗）を狙う。  
:::  
::: field first_floor_foldartal  
@type string
@optional
第 1 層の遠見段階で取得を希望する啓示板。Sami テーマのみに対応、モード問わず。取得成功時はタスクを停止。  
:::  
::: field start_foldartal_list  
@type array<string>
@default []
@optional
開局リセット時に取得を希望する啓示板リスト。Sami テーマでモード 4 の場合のみ対応。
<br>
開局時にリスト内のすべての啓示板を持つ場合のみ開局リセット成功と見なします。
<br>
注意、このパラメータは「生活重視分隊」と同時に使用する必要があります。他の分隊では開局報酬で啓示板を取得できません。  
:::  
::: field collectible_mode_start_list  
@type object
@optional
開局リセット時に取得したい報酬。デフォルトはすべて false。モード 4 の場合のみ有効。
<br>
`hot_water`: 電気ケトル報酬。お湯を沸かす機能のトリガー（共通）。
<br>
`shield`: シールド報酬。追加の HP 相当（共通）。
<br>
`ingot`: 源石錐報酬（共通）。
<br>
`hope`: 希望報酬（共通、注意：JieGarden テーマでは無効）。
<br>
`random`: ランダム報酬オプション：すべての源石錐を消費してランダムなコレクションを取得（共通）。
<br>
`key`: 鍵報酬。Mizuki テーマのみに有効。
<br>
`dice`: ダイスロール回数報酬。Mizuki テーマのみに有効。
<br>
`ideas`: 2 つの構想報酬。Sarkaz テーマのみに有効。
<br>
`ticket`: 遊覧券報酬。JieGarden テーマのみに有効。
:::  
::: field use_foldartal  
@type boolean
@optional
啓示板を使用するか。モード 5 ではデフォルト false、他のモードではデフォルト true。Sami テーマのみに対応。  
:::  
::: field check_collapsal_paradigms  
@type boolean
@optional
取得したパラダイムロストを検査するか。モード 5 ではデフォルト true、他のモードではデフォルト false。  
:::  
::: field double_check_collapsal_paradigms  
@type boolean
@default true
@optional
パラダイムロスト検査の漏れ対策を行うか。テーマが Sami で `check_collapsal_paradigms` が true の場合のみ有効。モード 5 ではデフォルト true、他のモードではデフォルト false。  
:::  
::: field expected_collapsal_paradigms  
@type array<string>
@default ['目空一些', '睁眼瞎', '图像损坏', '一抹黑']
@optional
希望するパラダイムロスト。テーマが Sami でモード 5 の場合のみに対応。  
:::  
::: field monthly_squad_auto_iterate  
@type boolean
@optional
月次小隊自動切り替えを有効にするかどうか。  
:::  
::: field monthly_squad_check_comms  
@type boolean
@optional
月次小隊通信も切り替え根拠とするかどうか。  
:::  
::: field deep_exploration_auto_iterate  
@type boolean
@optional
多面調査自動切り替えを有効にするかどうか。  
:::  
::: field collectible_mode_shopping  
@type boolean
@default false
@optional
湯沸かしモードで購物を有効にするかどうか。  
:::  
::: field collectible_mode_squad  
@type string
@optional
湯沸かしモードで使用する分隊、デフォルトで squad と同期、squad が空文字列で collectible_mode_squad が指定されていない場合は指揮分隊。  
:::  
::: field start_with_seed  
@type string
@optional
シード刷錠に使用する固定シード。空欄の場合は無効。
<br>
Sarkaz テーマ、Investment モード、「破棘成金分隊」または「支援分隊」の場合のみ有効。  
:::  
::: field blackflow_strategy  
@type string
@optional
黒流樹海テーマの戦略。空欄の場合は `mode` と `investment_enabled` から推測されます。
<br>
`baby_animal` - 1階層で一般商店を確認し、2・3階層を探索して秘境行商（シークレット商人）で種を育成します。`blackflow_cultivation_target` との併用が必要です
<br>
`investment` - 1階層を戦闘回数が最も少なく所要時間が最も短いルートで、固定の一般商店に到達します
<br>
`burn_with_investment` - 1階層で投資を完了した後、できるだけ早く3階層に到達し、到着次第再開します
<br>
`burn` - できるだけ早く3階層に到達し、到着次第再開します  
:::  
::: field blackflow_cultivation_target  
@type string
@default swaddled_cat
@optional
襁褓動物育成モードの目標。選択可能な値：`swaddled_cat`（襁褓の猫）| `swaddled_feathered_serpent`（襁褓の羽蛇）| `swaddled_dog`（襁褓の犬）| `swaddled_cerberus`（襁褓のケルベロス）。`blackflow_strategy` が `baby_animal` の場合のみ使用されます。  
:::  
::: field find_playTime_target  
@type number
@optional
常楽ノード稼ぎモードの目標常楽ノード。`1` - 令（掷地有声）；`2` - 黍（种因得果）；`3` - 年（三缺一）。テーマが JieGarden かつモードが 20001 の場合のみ使用され、そのモードでは必須。未指定やその他の値ではタスクパラメータの設定に失敗します。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Sami",
   "mode": 5,
   "squad": "指挥分队",
   "roles": "取长补短",
   "core_char": "塑心",
   "use_support": false,
   "use_nonfriend_support": false,
   "starts_count": 3,
   "difficulty": 8,
   "stop_at_final_boss": false,
   "stop_at_max_level": false,
   "investment_enabled": true,
   "investments_count": 2,
   "stop_when_investment_full": false,
   "investment_with_more_score": false,
   "start_with_elite_two": false,
   "only_start_with_elite_two": false,
   "refresh_trader_with_dice": false,
   "first_floor_foldartal": "",
   "start_foldartal_list": [],
   "collectible_mode_start_list": {
      "hot_water": true,
      "shield": false,
      "ingot": false,
      "hope": true,
      "random": false,
      "key": false,
      "dice": false,
      "ideas": false
   },
   "use_foldartal": true,
   "check_collapsal_paradigms": true,
   "double_check_collapsal_paradigms": true,
   "expected_collapsal_paradigms": ["目空一些", "睁眼瞎"],
   "monthly_squad_auto_iterate": false,
   "monthly_squad_check_comms": false,
   "deep_exploration_auto_iterate": false,
   "collectible_mode_shopping": false,
   "collectible_mode_squad": "",
   "start_with_seed": ""
}
```

</details>

パラダイムロスト稼ぎ機能の詳細は [統合戦略補助プロトコル](./integrated-strategy-schema.md#サーミテーマ--パラダイムロスト)を参照してください。

- `Copilot`  
  自動戦闘

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field filename  
@type string
単一作業 JSON ファイルのパス。copilot_list と二択（必須）。相対/絶対パスの両方対応。  
:::  
::: field copilot_list  
@type array`<object>`
作業リスト。filename と二択（必須）。filename と copilot_list が同時に存在する場合、copilot_list を無視。このパラメータが有効な場合、set_params は 1 回のみ実行可能。
<br>
各オブジェクトには以下を含みます：
<br>

- `filename`: 作業 JSON ファイルのパス。相対/絶対パスの両方対応
  <br>
- `nav_name_override`: ナビゲーション用のステージ名。可選、未指定または `null` の場合は、作業ファイルから自動的に推定されます
  <br>
- `is_raid`: 強襲作戦モードに切り替えるかどうか。可選、デフォルト false
  :::  
  ::: field loop_times  
  @type number
  @default 1
  @optional
  ループ回数。単一作業モード（filename 指定時）のみ有効。このパラメータが有効な場合、set_params は 1 回のみ実行可能。  
  :::  
  ::: field use_sanity_potion  
  @type boolean
  @default false
  @optional
  理智が不足した場合に理智薬を使用するかどうか。  
  :::  
  ::: field formation  
  @type boolean
  @default false
  @optional
  自動編成を行うかどうか。  
  :::  
  ::: field formation_index  
  @type number
  @default 0
  @optional
  自動編成で使用する編成スロットのインデックス。formation が true の場合のみ有効。
  <br>
  0～4 の整数。0 は現在の編成を意味し、1～4 は第 1～第 4 編成を表します。  
  :::  
  ::: field user_additional  
  @type array`<object>`
  @default []
  @optional
  カスタム追加オペレーター リスト。formation が true の場合のみ有効。
  <br>
  各オブジェクトには以下を含みます：
  <br>
- `name`: オペレーター名。可選、デフォルト ""。空の場合は無視
  <br>
- `skill`: 使用スキル。可選、デフォルト 0（ゲーム内デフォルトのスキル選択に従う）。1～3 の整数。範囲外の場合もゲーム内デフォルトを使用  
  :::  
  ::: field add_trust  
  @type boolean
  @default false
  @optional
  自動編成時に信頼度の昇順で空き枠を自動補充するか。formation が true の場合のみ有効。  
  :::  
  ::: field ignore_requirements  
  @type boolean
  @default false
  @optional
  自動編成時にオペレーター属性要件を無視するか。formation が true の場合のみ有効。  
  :::  
  ::: field support_unit_usage  
  @type number
  @default 0
  @optional
  サポートオペレーターの使用モード。0～3 の整数。formation が true の場合のみ有効。
  <br>
  `0` - サポートオペレーターを使用しない
  <br>
  `1` - 不足が 1 名のみの場合、サポートで補填。不足がなければサポートを使用しない
  <br>
  `2` - 不足が 1 名のみの場合、サポートで補填。不足がなければ指定サポートを使用  
  <br>
  `3` - 不足が 1 名のみの場合、サポートで補填。不足がなければランダムサポートを使用  
  :::  
  ::: field support_unit_name  
  @type string
  @optional
  指定サポートオペレーター名。support_unit_usage が 2 の場合のみ有効。  
  :::  
  ::::

作業 JSON は [戦闘流程プロトコル](./copilot-schema.md)を参照

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "copilot/1-7.json",
   "loop_times": 2,
   "use_sanity_potion": false,
   "formation": true,
   "formation_index": 1,
   "user_additional": [
      {
         "name": "能天使",
         "skill": 3
      }
   ],
   "add_trust": true,
   "ignore_requirements": false,
   "support_unit_usage": 2,
   "support_unit_name": "艾雅法拉"
}
```

</details>

- `SSSCopilot`  
  自動戦闘保全駐在

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field filename  
@type string
@required
作業 JSON のファイル パス。絶対/相対パスの両方対応。実行期設定非対応。  
:::  
::: field loop_times  
@type number
@optional
ループ実行回数。  
:::  
::::  
保全駐在作業 JSON は [保全派駐プロトコル](./sss-schema.md)を参照

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "sss/plan.json",
   "loop_times": 1
}
```

</details>

- `ParadoxCopilot`
  自動戦闘逆理演算

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field filename  
@type string
@required
単一作業 JSON ファイルのパス。絶対/相対パスの両方対応。実行期設定非対応。必須。list と二択。  
:::  
::: field list  
@type array`<object>` | array`<string>`
@required
作業リスト。実行期設定非対応。必須。filename と二択。
<br>
配列の要素は 2 つの形式をサポートします：オブジェクト形式は `id`（作業識別子。`CopilotListLoadTaskFileSuccess` コールバックにそのまま透過される）と `filename`（作業 JSON ファイルのパス。絶対/相対パスの両方可）を含みます。作業パスの文字列を直接指定することもできます。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "paradox/exusiai.json",
   "list": []
}
```

</details>

- `Depot`  
  倉庫識別

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `OperBox`  
  オペレーター box 識別

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `Reclamation`  
  生息演算

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field theme  
@type string
@default Tales
@optional
テーマ。
<br>
`Fire` - _砂中の火_（終了）
<br>
`Tales` - _熱砂秘聞_
<br>
`RelaunchAnchor` - _リローンチアンカー_  
:::  
::: field mode  
@type number
@default 0
@optional
モード。テーマごとにサポートするモードが異なります：
<br>
**Tales（熱砂秘聞）：**
<br>
`0` - セーブなし、ステージ出入りで生息ポイントを稼ぐ。
<br>
`1` - セーブあり、支援アイテムを組み立てて生息ポイントを稼ぐ。
<br>
**RelaunchAnchor（リローンチアンカー）：**
<br>
`16` (`RA1`) - RA-1、精耕細作→建設→資源納品→決算を自動ループ。
<br>
`32` (`RA15`) - RA-15、シヴィライト・エテルナで60撃破ミッションを達成。
<br>
`48` (`RA4`) - RA-4、「計画経営」で得た赤金を使ってエリアを解放し、ヴィシャデルでボス討伐を完了する。
:::  
::: field tools_to_craft  
@type array<string>
@default []
@optional
自動製造品。サブストリング入力推奨。空の場合は製造しません。Tales テーマのセーブありモード（mode = 1）のみ有効。  
:::  
::: field clear_store  
@type boolean
@default false
@optional
タスク完了後にショップの商品を購入（買い切る）するかどうか。Tales テーマのセーブなしモード（mode = 0）のみ有効。  
:::  
::: field increment_mode  
@type number
@default 0
@optional
クリック型。Tales テーマのみ有効。
<br>
`0` - 連続クリック
<br>
`1` - 長押し  
:::  
::: field num_craft_batches  
@type number
@default 16
@optional
単次最大製造バッチ数。Tales テーマのみ有効。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Fire",
   "mode": 1,
   "tools_to_craft": ["荧光棒", "发电机"],
   "increment_mode": 0,
   "num_craft_batches": 12
}
```

</details>

- `Custom`  
  カスタム タスク

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field task_names  
@type array<string>
@required
配列内の最初の一致（および後続の next など）でタスクを実行。複数のタスクを実行したい場合は、Custom task を複数回 append できます。  
シークレットフロント（`MiniGame@SecretFront`）の連結形式に対応：`MiniGame@SecretFront@Begin@Ending[A-E](@イベント名)?`。イベント名は省略可（支援作战平台 / 游侠 / 诡影迷踪）、例：`MiniGame@SecretFront@Begin@EndingA@支援作战平台`。  
:::  
::: field params  
@type object
@optional
タスクの追加パラメータ。現在はピクセル画タスク（`MiniGame@PixelPaint@Begin`）のみで使用：

- `params.pixel_paint.groups`：色ごとのマス座標リスト。`color` はパレットのスロット番号（0~39、ゲーム右側パレットの順序と一致）、`points` は `[x, y]` のマス座標配列（0~23、左上原点）。
- `params.pixel_paint.swipe`（bool、任意、デフォルト true）：同じ色の連続マスを1回のドラッグで描画し高速化。一部のタッチ方式では正常に動作しない可能性があります。
- `params.pixel_paint.grid_delay`（int、任意、デフォルト 0）：マスごとの追加待機時間（ms）。クリック後の待機とドラッグ時間の両方に加算されます。各タッチ方式に基礎間隔があるため、通常は調整不要。旧キー `grid_click_delay` も受け付けます。

:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "task_names": ["StartUp", "Infrast", "Fight"]
}
```

```json
{
   "enable": true,
   "task_names": ["MiniGame@PixelPaint@Begin"],
   "params": {
      "pixel_paint": {
         "groups": [
            { "color": 7, "points": [[0, 1], [3, 4]] }
         ]
      }
   }
}
```

</details>

- `SingleStep`  
  単一ステップ タスク（現在は戦闘のみ対応）

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field type  
@type string
@default copilot
@required
現在は `"copilot"` のみ対応。  
:::  
::: field subtype  
@type string
@required
サブタスク型。
<br>
`stage` - ステージ名を設定、`"details": { "stage_name": "xxxx" }` が必要。
<br>
`start` - 作戦開始、`details` なし。
<br>
`action` - 単一ステップ作戦操作、`details` は戦闘プロトコルの単一 action、例：`"details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" }`、詳細は [戦闘流程プロトコル](./copilot-schema.md)を参照。  
:::  
::: field details  
@type object
@optional
サブタスクの詳細パラメータ。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "type": "copilot",
   "subtype": "stage",
   "details": {
      "stage_name": "1-7"
   }
}
```

</details>

- `VideoRecognition`  
  ビデオ識別、現在は作業（戦闘）ビデオのみ対応

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
このタスクを有効にするかどうか。  
:::  
::: field filename  
@type string
@required
ビデオのファイル パス。絶対/相対パスの両方対応。実行期設定非対応。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "videos/copilot.mp4"
}
```

</details>

### `AsstSetTaskParams`

#### インターフェース プロトタイプ

```cpp
AsstBool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### インターフェースの説明

タスク パラメータを設定します。

#### 返回値

- `AsstBool`  
   設定が成功したかどうかを返す

#### パラメータ説明

:::: field-group  
::: field handle  
@type AsstHandle
@required
インスタンス ハンドル  
:::  
::: field id  
@type AsstTaskId
@required
タスク ID、`AsstAppendTask` インターフェイスの返回値  
:::  
::: field params  
@type const char\*
@required
タスク パラメータ、JSON 文字列、`AsstAppendTask` インターフェイスと同じ。  
「実行期設定非対応」と注記されていないフィールドはすべてリアルタイム修正に対応。そうでなければ、タスク実行中は対応するフィールドを無視します  
:::  
::::

### `AsstSetStaticOption`

#### インターフェース プロトタイプ

```cpp
AsstBool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### インターフェースの説明

プロセス レベル パラメータを設定します。

#### 返回値

- `AsstBool`  
   設定が成功したかどうかを返す

#### パラメータ説明

:::: field-group  
::: field key  
@type AsstStaticOptionKey
@required
キー  
:::  
::: field value  
@type const char\*
@required
値  
:::  
::::

##### キー値一覧

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
無効なプレースホルダ。列挙値：0。  
:::  
::: field CpuOCR  
@type boolean
@optional
CPU で OCR を行います。値はパースに参加しません。リソースロード後の切り替えは非対応。列挙値：1。  
:::  
::: field GpuOCR  
@type string
@optional
GPU で OCR を行います。値は GPU デバイスの序数（整数）。Windows では `luid:<16 進数 LUID>` も指定できます。リソースロード後の切り替えは非対応。列挙値：2。  
:::  
::::

### `AsstSetInstanceOption`

#### インターフェース プロトタイプ

```cpp
AsstBool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### インターフェースの説明

インスタンス レベル パラメータを設定します。

#### 返回値

- `AsstBool`  
   設定が成功したかどうかを返す

#### パラメータ説明

:::: field-group  
::: field handle  
@type AsstHandle
@required
インスタンス ハンドル  
:::  
::: field key  
@type AsstInstanceOptionKey
@required
キー  
:::  
::: field value  
@type const char\*
@required
値  
:::  
::::

##### キー値一覧

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
無効なプレースホルダ。列挙値：0。  
:::  
::: field MinitouchEnabled  
@type boolean
@optional
廃止済み。元は Minitouch を有効にするかどうか。"1" オン、"0" オフ。デバイスがサポートされていない可能性がります。列挙値：1（廃止済み）。  
:::  
::: field TouchMode  
@type string
@default minitouch
@optional
タッチ モード設定。可能な値：minitouch | maatouch | adb | MacPlayTools | MaaFwAdb | MumuExtras | MaaFwLinux。デフォルト minitouch。列挙値：2。  
:::  
::: field DeploymentWithPause  
@type boolean
@optional
暫停状態でオペレーターを配置するかどうか。自動戦闘、統合戦略、保全駐在に同時に影響。可能な値："1" または "0"。列挙値：3。  
:::  
::: field AdbLiteEnabled  
@type boolean
@optional
AdbLite を使用するかどうか。可能な値："0" または "1"。列挙値：4。  
:::  
::: field KillAdbOnExit  
@type boolean
@optional
終了時に Adb プロセスをキルするかどうか。可能な値："0" または "1"。列挙値：5。  
:::  
::: field ClientType  
@type string
@optional
クライアント種別（ゲームチャネル）。ほとんどの接続設定では不要です。`AsstConnect` / `AsstAsyncConnect` に渡す `config` が、接続時に実行するコマンド内で `[PackageName]` を使用する場合にのみ、接続前に `AsstSetInstanceOption(..., ClientType, ...)` を設定してください。現在の組み込み設定では、`Androws` と `WSA` の `displayId` 取得のみがこの値に依存します。このオプションは StartUp / CloseDown などのタスクパラメータ `client_type` を置き換えるものではありません。列挙値：6。  
:::  
::::
