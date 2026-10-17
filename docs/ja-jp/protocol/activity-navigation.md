---
order: 9
icon: material-symbols:route
---

# イベントナビゲーション

`ActivityNavigation` はイベントのエリアに移動するだけのタスクです。ステージ選択、編成、戦闘開始は行いません。必須パラメータは `stage_id`（Tile-Pos の内部 ID）です。`client_type` は省略時に接続中のクライアントを使用し、`is_raid` は既定値 false で難易度の検証のみ行います。実行前にゲームへ接続し、ログインを完了してください。

`Copilot` の `copilot_list` モードと `Fight` は `navigate_to_activity: true` で同じ入口を使用できます（既定値 false）。Copilot は明示的な `client_type` も必要です。単独の作戦も要素 1 個のリストを使ってください。`filename` と同時に有効化すると拒否されます。登録済みステージはタスク受付時と各項目の実行時に検証されます。エリア移動後は既存のステージ選択、番号確認、強襲切替、編成、戦闘を継続します。WPF の作戦リストでは有効化され、単一ファイルの動作は従来通りです。

`resource/activity_navigation.json` は `schema_version: 1` と `activities` を持ちます。活動ごとに `id`、`enabled`、`clients`、`routes`、`stages`、説明用の `name` と `sources` を記録します。各エリアは標準タスク名 `task`、タイムゾーンと秒を含む ISO 8601 の `opens_at` / `closes_at` を持ちます。内部ステージ ID は `code`、`route`、`modes`（normal / raid）へ対応します。開始時刻を含み終了時刻を含みません。タスクと番号は Tile-Pos と一致する必要があります。登録済みの閉鎖、無効、クライアント不一致、非対応難易度は失敗し、フォールバックしません。未登録の活動は従来通りです。有効な完全データで前の設定を置換し、空の `activities` で消去できます。無効なデータでは置換しません。

互換タスクは `resource/tasks/hotfix/activity_navigation/` に置きます。MaaResource は設定ファイルと `template/StageNavigation/` を配信対象に含め、主リポジトリの同期処理で資源バージョンを更新します。新しいタスク項目やコア機能にはコア更新が必要です。`AsstGetActivityNavigationVersion()` は対応プロトコル（本実装は 1）を返します。呼出前にシンボルの存在を確認し、旧コアでは外部 Custom ナビゲーションを維持してください。通知は `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed` で、ステージ、エリア、失敗理由を含みます。
