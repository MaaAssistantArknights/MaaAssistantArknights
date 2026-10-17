---
order: 9
icon: material-symbols:route
---

# 活動ナビゲーション

既定ではクライアントが読み込んだ既存タスクを再利用します。Tile-Pos の内部 ID から表示コードを取得し、`YW-1` は `YW-OpenOcr`、`YW-EX-1` は `YW-EX-OpenOcr` に対応します。共通タスクは戻る操作に対応した画面からターミナルへ移動し、既存の OCR 入口と章のタスクを実行して、正確なステージコードを認識した後に従来の選択処理へ戻します。EX 入口がない場合は `MissingTask` で停止し、通常区画には入りません。

入口と章のタスクは既存の MaaRelease `resource/tasks.json` 更新経路を使用し、MaaCore 自体は API を取得しません。活動や区画の追加時は同じ JSON で完全な OCR タスク列を公開し、同時配信が必要な新しい画像への依存を避けてください。ターミナル復帰には既存の基本画像を使用します。テンプレート配信や必須画像の欠落検証は変更せず、個別ステージの既存テンプレート選択も維持します。

独立タスク `ActivityNavigation` は対象ステージのマップへ入り、編成や戦闘開始は行いません。`stage_id` は必須、`client_type` と `is_raid` は任意です。事前に接続とログインが必要です。Copilot リストと Fight は `navigate_to_activity: true` で有効化します（API 既定値 false、WPF リストでは有効）。Copilot では `client_type` を明示し、単一作業も単一要素のリストにしてください。`filename` との同時有効化は拒否されます。既存ナビゲーションに対応したメインストーリー・資源ステージもターミナルへ自動復帰でき、未対応ステージは現在のマップで従来の選択を行います。Fight の `is_raid` は既定値 false で、活動検証とステージ選択後の難易度切り替えに使用します。

`resource/activity_navigation.json` は任意のユーザー上書きです。スキーマ 1 の活動は `id`、`name`、`enabled`、`clients`、`sources`、`routes`、`stages` を保持します。区画は `task`、タイムゾーン付き秒精度の `opens_at`／`closes_at`、ステージは `code`、`route`、`modes`（normal／raid）を指定します。上書きは自動検出より優先し、開始時刻を含み終了時刻を含まない期間、クライアント、モードを検証します。既定の OCR タスクには追加メタデータがなく、名前から日付やクライアント制限を推測しません。活動時間は引き続き呼び出し側が StageActivity API で扱います。有効な全体設定で置換し、空の `activities` は上書きを消して自動検出に戻し、不正な設定は置換しません。

Copilot は受付時のルート値を保存し、削除や変更時に停止します。同じ内容の再読み込みは有効です。ユーザー上書きの取得と置換は読み書きロックで同期します。コールバックは `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed` です。事前検証には `phase: Preflight` と `MissingTask`、`InvalidEntryTask`、またはユーザー方針の `Closed`、`Disabled`、`WrongClient`、`StageCodeMismatch`、`UnsupportedMode` が含まれ、待機中ルートの変更は `RouteChanged` です。`AsstGetActivityNavigationVersion()` はプロトコル 2（既存タスクからの自動検出を含む）を返し、呼び出す前にシンボルを確認してください。旧コアは外部 Custom ナビゲーションを使用できます。
