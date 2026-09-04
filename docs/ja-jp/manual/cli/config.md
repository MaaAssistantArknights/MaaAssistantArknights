---
order: 3
icon: material-symbols:settings
---

# 設定

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

## 設定ディレクトリ

maa-cli の設定ファイルは特定の設定ディレクトリに配置され、`maa dir config` で設定ディレクトリを取得できます。設定ディレクトリは環境変数 `MAA_CONFIG_DIR` で変更することもできます。以下の例では、`$MAA_CONFIG_DIR` で設定ディレクトリを表します。

すべての設定ファイルには TOML、YAML、JSON 形式のいずれも使用できます。以下の例では TOML 形式を使用し、`.toml` を拡張子として使用します。ただし、拡張子が正しければ、これら 3 つの形式は任意に組み合わせて使用できます。

また、一部のタスクは `filename` をパラメータとして受け付けます。相対パスを使用する場合、そのパスは設定ディレクトリ内の対応するサブディレクトリを基準とします。たとえば、カスタム基地計画ファイルの相対パスは `$MAA_CONFIG_DIR/infrast` を基準とし、保全駐在の作業ファイルは `$MAA_CONFIG_DIR/ssscopilot` を基準とします。

## カスタムタスク

各カスタムタスクは個別のファイルで、`$MAA_CONFIG_DIR/tasks` ディレクトリに配置する必要があります。

### 基本構造

タスクファイルは複数のサブタスクを含み、各サブタスクは 1 つの MAA タスクであり、次のいくつかのオプションを含みます：

```toml
[[tasks]]
name = "ゲーム起動" # タスクの名前、省略可、デフォルトはタスクタイプ
type = "StartUp" # タスクのタイプ
params = { client_type = "Official", start_game_enabled = true } # 対応するタスクのパラメータ
```

具体的なタスクタイプとパラメータは [MAA 統合ドキュメント][task-types] で確認できます。なお、現在 maa-cli はパラメータ名やパラメータ値の正しさを検証しません。エラーが発生してもエラーメッセージは生成されず、MaaCore が実行時にエラーを検出した場合にのみ検出されます。

### タスク条件

いくつかの条件に応じて異なるパラメータのタスクを実行したい場合は、タスクの複数のバリアントを定義できます：

```toml
[[tasks]]
name = "基地シフト交代"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # カスタム基地計画のファイル名は `$MAA_CONFIG_DIR/infrast` に配置する必要があります

# 18:00:00 から翌日の 04:00:00 までは計画 0 を使用し、12:00:00 より前は計画 1、それ以降は計画 2 を使用します
[[tasks.variants]]
condition = { type = "Time", start = "18:00:00", end = "04:00:00" } # 終了時刻が開始時刻より小さい場合、終了時刻は翌日の時刻とみなされます
params = { plan_index = 0 }

[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 開始時刻が省略された場合、現在時刻が終了時刻より小さいときにこの条件が一致します
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00" } # 終了時刻が省略された場合、現在時刻が開始時刻より大きいときにこの条件が一致します
params = { plan_index = 2 }
```

ここの `condition` フィールドは、どのバリアントを使用するかを決定するために使われ、一致したバリアントの `params` フィールドはタスクのパラメータにマージされます。

**注意**：カスタム基地計画ファイルに相対パスを使用する場合は、`$MAA_CONFIG_DIR/infrast` を基準にする必要があります。また、基地ファイルは maa-cli ではなく MaaCore によって読み込まれるため、これらのファイルの形式は JSON でなければなりません。同時に、maa-cli は基地ファイルを読み込まず、その中で定義された時間帯に基づいて対応するサブ計画を選択することもありません。したがって、`condition` フィールドで、対応する時間帯に正しい基地計画パラメータの `plan_index` フィールドを指定する必要があります。これにより、適切な時間帯に正しい基地計画が使用されることが保証されます。

`Time` 条件のほかに、`DateTime`、`Weekday`、`DayMod` 条件もあります。`DateTime` 条件は時間帯を指定するために、`Weekday` 条件は一週間のうちの特定の曜日を指定するために、`DayMod` はカスタム周期の特定の日を指定するために使用されます。

```toml
[[tasks]]
type = "Fight"

# 夏イベント期間中は SL-8 を周回
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 夏イベント期間外の火・木・土曜日は CE-6 を周回
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"], timezone = "Official"}
params = { stage = "CE-6" }

# その他の時間は 1-7 を周回
[[tasks.variants]]
params = { stage = "1-7" }
```

上記のすべての時間関連の条件は、`timezone` パラメータでタイムゾーンを指定できます。このパラメータの値は数値で、UTC からのオフセットを表します。タイムゾーンが UTC+8 の場合は `timezone = 8` と指定できます。このパラメータにはクライアントタイプも指定でき、たとえば `timezone = "Official"` とすると、公式サーバーに対応するサーバー時刻で判定が行われます。**注意**、公式サーバーのタイムゾーンは UTC+8 ではなく UTC+4 です。ゲーム内の 1 日の開始時刻が 00:00:00 ではなく 04:00:00 だからです。タイムゾーンを指定しない場合は、ローカルのタイムゾーンがそのまま使用されます。

上記の確定的な条件のほかに、ホットアップデートリソースに依存する条件 `OnSideStory` もあります。この条件を有効にすると、maa-cli は対応するリソースを読み込んで、現在進行中のイベントがあるかどうかを判断しようとし、あれば対応するバリアントが一致します。たとえば、上記の夏イベント期間中に `SL-8` を周回する条件は `{ type = "OnSideStory", client = "Official" }` と簡略化できます。ここの `client` パラメータは使用するクライアントを決定するために使われます。クライアントによってイベントの期間が異なるためです。公式サーバーまたは B サーバーを使用しているユーザーの場合、これは省略できます。この条件により、イベントが更新されるたびに、周回するステージを更新するだけで、対応するイベントの開催時間を手動で編集する必要がなくなります。

上記の基本条件のほかに、`{ type = "And", conditions = [...] }`、`{ type = "Or", conditions = [...] }`、`{ type = "Not", condition = ... }` を使用して条件の論理演算を行うことができます。
複数日にわたる基地シフトを組みたいユーザーは、`DayMod` と `Time` を組み合わせることで、複数日のシフトを実現できます。たとえば、2 日ごとに 6 回交代するシフトを実現したい場合は、次のように書きます：

```toml
[[tasks]]
name = "基地シフト (2日6交代)"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json"

# 第 1 シフト、1 日目 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 0 }
[tasks.variants.condition]
type = "And"
conditions = [
    # ここの divisor は周期を、remainder はオフセットを指定します
    # オフセットは num_days_since_ce % divisor に等しくなります
    # ここの num_days_since_ce は西暦 1 年 1 月 1 日（0001-01-01）を 1 日目とする経過日数です
    # 当日のオフセットは `maa remainder <divisor>` で取得できます。
    # たとえば、2024-1-27 は 738,912 日目なので、738912 % 2 = 0 となります
    # 当日のオフセットは 0 となり、この条件が一致します
    { type = "DayMod", divisor = 2, remainder = 0 },
    { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第 2 シフト、1 日目 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 1 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 0 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第 3 シフト、1 日目 20:00:00 - 2 日目 4:00:00
[[tasks.variants]]
params = { plan_index = 2 }
[tasks.variants.condition]
# ここでは Or 条件を使用する必要があります。Time { start = "20:00:00", end = "04:00:00" } を直接使用することはできません
# その場合、2 日目の 00:00:00 - 04:00:00 が一致しなくなります
# もちろん、シフト時間を調整して日をまたがないようにする方が良い選択ですが、ここではデモのためだけに示しています
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", end = "04:00:00" },
  ] },
]

# 第 4 シフト、2 日目 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 3 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第 5 シフト、2 日目 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 4 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第 6 シフト、2 日目 20:00:00 - 3 日目（新しい 1 日目）4:00:00
[[tasks.variants]]
params = { plan_index = 5 }
[tasks.variants.condition]
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", end = "04:00:00" },
  ] },
]
```

デフォルトの戦略では、複数のバリアントが一致した場合、最初のものが使用されます。条件が指定されていない場合、そのバリアントは常に一致するため、条件のないバリアントを最後に置いてデフォルトのケースとすることができます。

`strategy` フィールドを使用して一致戦略を変更できます：

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # または "first"（デフォルト）

# 日曜の夜に、期限切れが近い理性剤をすべて使用する
[[tasks.variants]]
params = { medicine_expire_days = 2 }

[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# デフォルトでは 1-7 を周回
[[tasks.variants]]
params = { stage = "1-7" }

# 火・木・土曜日は CE-6 を周回
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 夏イベント期間中は SL-8 を周回
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

この例は上の例と同じステージを周回しますが、日曜の夜には期限切れが近い理性剤がすべて使用されます。`merge` 戦略では、複数のバリアントが一致した場合、後のバリアントのパラメータが前のバリアントのパラメータにマージされます。複数のバリアントが同じパラメータを持つ場合、後のバリアントのパラメータが前のバリアントのパラメータを上書きします。

バリアントが 1 つも一致しない場合、タスクは実行されません。これを利用して、特定の条件下でのみサブタスクを実行できます：

```toml
# 18:00:00 以降のみ信用ショップ関連の操作を行う
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

### ユーザー入力

一部のタスクでは、実行時にステージ名などのパラメータを入力したい場合があります。対応する入力が必要なパラメータを `Input` または `Select` タイプに設定できます：

```toml
[[tasks]]
type = "Fight"

# ステージを選択する
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
[tasks.variants.params.stage]
# 選択可能なステージ。少なくとも 1 つの選択肢を提供する必要があります
# 選択肢は値のみ、または値と説明を同時に含むテーブルのいずれかです
alternatives = [
    "SL-7", # "1. SL-7" と表示されます
    { value = "SL-8", desc = "軽マンガン鉱" } # "2. SL-8 (軽マンガン鉱)" と表示されます
]
default_index = 1 # デフォルト値のインデックス。1 から開始。設定しない場合、先頭の選択肢がデフォルト値として使用されます
description = "a stage to fight in summer event" # 説明、省略可
allow_custom = true # カスタム値の入力を許可するかどうか、デフォルトは false。許可されている場合、整数以外の値はカスタム値として扱われます

# 入力不要
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# ステージを入力する
[[tasks.variants]]
[tasks.variants.params.stage]
default = "1-7" # デフォルトのステージ、省略可（デフォルト値がない場合、空の値を入力すると再入力を求められます）
description = "a stage to fight" # 説明、省略可

# 入力されたステージが 1-7 の場合、使用する理性剤の数を入力する必要があります
[tasks.variants.params.medicine]
# パラメータは条件付きパラメータに設定でき、条件を満たす場合にのみ入力が必要になります
# conditions フィールドはテーブルで、キーは同じ階層にある他のパラメータ名、値は期待される値です
# ここの条件は stage が 1-7 であることです。複数の条件が存在する場合、すべての条件が満たされる必要があります
conditions = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

`Input` タイプでは、タスク実行時に値の入力を求められます。空の値を入力した場合、デフォルト値があればそれが使用され、なければ再入力を求められます。
`Select` タイプでは、タスク実行時にインデックスまたはカスタム値（許可されている場合）の入力を求められます。空の値を入力した場合、デフォルト値が使用されます。

`--batch` オプションを使用すると、タスク実行時のすべての入力をスキップでき、デフォルト値が使用されます。デフォルト値のない入力がある場合、エラーになります。

## MaaCore 関連設定

MaaCore に関連する設定は `$MAA_CONFIG_DIR/profiles` ディレクトリに配置する必要があります。このディレクトリ内の各ファイルは設定ファイルであり、`-p` または `--profile` オプションで設定ファイル名を指定できます。指定しない場合は `default` 設定ファイルの読み込みが試みられます。

現在サポートされている設定フィールドは次のとおりです：

```toml
[connection]
preset = "MuMuPro"
adb_path = "adb"
address = "emulator-5554"
config = "CompatMac"

[resource]
global_resource = "YoStarEN"
platform_diff_resource = "iOS"
user_resource = true

[static_options]
cpu_ocr = false
gpu_ocr = 1

[instance_options]
touch_mode = "MaaTouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false

[behavior]
auto_reconnect = true
```

### 接続設定

`[connection]` 関連フィールドは、MaaCore がゲームに接続するためのパラメータを指定するために使用されます：

```toml
[connection]
adb_path = "adb" # adb 実行ファイルのパス。デフォルトは "adb" で、adb 実行ファイルが環境変数 PATH 内にあることを意味します
address = "emulator-5554" # 接続アドレス。たとえば "emulator-5554" または "127.0.0.1:5555"
config = "General" # 接続設定。通常は変更不要
```

`adb_path` は `adb` 実行ファイルのパスです。パスを指定するか、環境変数 `PATH` に追加して MaaCore がそれを見つけられるようにできます。ほとんどのエミュレーターには `adb` が付属しており、追加インストールなしで内蔵の `adb` を直接使用できます。付属していない場合は自分で `adb` をインストールする必要があります。`address` は `adb` の接続アドレスです。エミュレーターの場合は `127.0.0.1:[ポート番号]` を使用でき、よく使われるエミュレーターのポート番号は[よくある質問][emulator-ports]を参照してください。`address` を指定しない場合、`adb devices` で接続中のデバイスを取得しようとします。複数のデバイスが接続されている場合は最初のデバイスが使用され、デバイスが見つからない場合は `emulator-5554` への接続が試みられます。`config` はプラットフォームやエミュレーターに関連する設定を指定するために使用されます。Linux ではデフォルトで `CompatPOSIXShell`、macOS ではデフォルトで `CompatMac`、Windows ではデフォルトで `General` です。その他の選択可能な設定は、リソースフォルダ内の `config.json` ファイルで確認できます。

よく使われる一部のエミュレーターについては、`preset` を使用してプリセット設定を直接利用できます：

```toml
[connection]
preset = "MuMuPro" # MuMuPro のプリセット接続設定を使用
adb_path = "/path/to/adb" # 必要に応じて、プリセットの adb パスを上書きできます。ほとんどの場合、その必要はありません
address = "127.0.0.1:7777" # 必要に応じて、プリセットのアドレスを上書きできます
```

現在、`MuMuPro` と `Androws` の 2 つのエミュレータープリセットが内蔵されています。`Androws` プリセットは Windows 上の Tencent Androws エミュレーター向けで、付属の `adb` をレジストリから自動検出し、デフォルトの接続アドレスは `127.0.0.1:5555` です。他のよく使われるエミュレーターのプリセットがあれば、issue または PR の提出を歓迎します。

#### 特別なプリセット

現在、`PlayCover (macOS)` と `Waydroid (Linux)` の 2 種類のプリセットがあらかじめ設定されています。

- `PlayCover` は、macOS で `PlayCover` によってネイティブに実行されているゲームクライアントに直接接続するために使用します。この場合、`adb_path` を指定する必要はなく、`address` は `adb` の接続アドレスではなく `PlayTools` のアドレスになります。具体的な使用方法は [PlayCover サポートドキュメント][playcover-doc] を参照してください。

- `Waydroid` は、Linux で `Waydroid` によってネイティブに実行されているゲームクライアントに直接接続するために使用します。この場合でも `adb_path` の指定が必要です。具体的な使用方法は [Waydroid サポートドキュメント][waydroid-doc] を参照してください。

### リソース設定

`[resource]` 関連フィールドは、MaaCore が読み込むリソースを指定するために使用されます：

```toml
[resource]
global_resource = "YoStarEN" # 中国語以外のバージョンのリソース
platform_diff_resource = "iOS" # Android 以外のバージョンのリソース
user_resource = true # ユーザーカスタムのリソースを読み込むかどうか
```

簡体字中国語以外のゲームクライアントを使用する場合、MaaCore がデフォルトで読み込むリソースは簡体字中国語版であるため、`global_resource` フィールドを指定して中国語以外のバージョンのリソースを読み込む必要があります。iOS 版のゲームクライアントを使用する場合は、`platform_diff_resource` フィールドを指定して iOS 版のリソースを読み込む必要があります。これらはどちらも省略可能で、これらのリソースを読み込む必要がない場合は、これら 2 つのフィールドを空に設定できます。次に、これら 2 つは自動的に設定されることもあります。`startup` タスクで `client_type` フィールドが指定されている場合、`global_resource` は対応するクライアントのリソースに設定され、`PlayTools` で接続する場合、`platform_diff_resource` は `iOS` に設定されます。最後に、ユーザーカスタムのリソースを読み込みたい場合は、`user_resource` フィールドを `true` に設定する必要があります。

### 静的オプション

`[static_options]` 関連フィールドは、MaaCore の静的オプションを指定するために使用されます：

```toml
[static_options]
cpu_ocr = false # CPU OCR を使用するかどうか。デフォルトでは CPU OCR を使用します
gpu_ocr = 1 # GPU OCR 使用時の GPU ID。この値が空の場合、CPU OCR が使用されます
```

### インスタンスオプション

`[instance_options]` 関連フィールドは、MaaCore インスタンスのオプションを指定するために使用されます：

```toml
[instance_options]
touch_mode = "ADB" # 使用するタッチモード。"ADB"、"MiniTouch"、"MaaTouch"、"MacPlayTools" または "MaaFwAdb" から選択
deployment_with_pause = false # デプロイ時にゲームを一時停止するかどうか
adb_lite_enabled = false # adb-lite を使用するかどうか
kill_adb_on_exit = false # 終了時に adb を強制終了するかどうか
```

なお、`touch_mode` の選択肢 `MacPlayTools` は接続方式 `PlayTools` と紐付いています。`PlayTools` で接続する場合、`touch_mode` は強制的に `MacPlayTools` に設定されます。

## CLI 関連設定

CLI 関連の設定は `$MAA_CONFIG_DIR/cli.toml` に配置する必要があります。現在含まれる設定は次のとおりです：

```toml
# GitHub プロキシのプレフィックス。設定すると、GitHub release のダウンロードアドレスがこのプロキシを経由するアドレスへ透過的に書き換えられます
# github_proxy = "https://gh-proxy.org/"

# MaaCore のインストールと更新に関する設定
[core]
channel = "Stable" # 更新チャンネル。"Alpha"、"Beta"、"Stable" から選択、デフォルトは "Stable"
test_time = 0    # ミラー速度のテストに使用する時間。0 はテストしないことを意味し、デフォルトは 3
# MaaCore の最新バージョンを照会する API アドレス。空欄でデフォルトアドレスを使用
api_url = "https://api.maa.plus/MaaAssistantArknights/api/version/"

# MaaCore の対応するコンポーネントをインストールするかどうかの設定。非推奨。別々にインストールするとバージョンの不一致により問題が発生する可能性があり、このオプションは将来のバージョンで削除される可能性があります
[core.components]
library = true  # MaaCore のライブラリをインストールするかどうか、デフォルトは true
resource = true # MaaCore のリソースをインストールするかどうか、デフォルトは true

# CLI の更新に関する設定
[cli]
channel = "Stable" # 更新チャンネル。"Alpha"、"Beta"、"Stable" から選択、デフォルトは "Stable"
# maa-cli の最新バージョンを照会する API アドレス。空欄でデフォルトアドレスを使用
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# プリコンパイル済みバイナリのダウンロードアドレス。空欄でデフォルトアドレスを使用
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

# maa-cli の対応するコンポーネントをインストールするかどうかの設定
[cli.components]
binary = true # maa-cli のバイナリをインストールするかどうか、デフォルトは true

# リソースのホットアップデートに関する設定
[resource]
auto_update = true  # タスク実行のたびにリソースを自動更新するかどうか、デフォルトは false
warn_on_update_failure = true # 更新失敗時に直接エラーにするのではなく警告を発するかどうか
backend = "libgit2" # リソースのホットアップデートのバックエンド。"git" または "libgit2" から選択、デフォルトは "git"

# リソースのホットアップデートのリモートリポジトリに関する設定
[resource.remote]
branch = "main" # リモートリポジトリのブランチ。空欄の場合はリモートリポジトリのデフォルトブランチを使用
# リモートリソースリポジトリの URL。空欄でデフォルト URL を使用
# GitHub リポジトリは HTTPS と SSH の両方のプロトコルでアクセスできます。通常は追加設定が不要なため、HTTPS プロトコルを推奨します
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# SSH プロトコルを使用する必要がある場合は、SSH キーを提供する必要があります。最も簡単な方法はキーのパスを提供することです
ssh_key = "~/.ssh/id_ed25519" # SSH キーのパス
# maa のデフォルトのキーは暗号化されていません。キーがパスワードで保護されている場合は、キーを復号するためのパスワードを提供する必要があります
# 注意：maa がパスワードを libgit2 に渡すのは libgit2 バックエンドを使用している場合のみです
# git バックエンドを使用する場合、git が自分でパスワードの入力を求めます
# git バックエンドを使用し、かつキーがパスワードで保護されている場合は、ssh-agent でキーを管理してください
passphrase = "password"       # SSH キーのパスワード
# しかし設定ファイルに平文のパスワードを保存するのは安全ではないため、これを回避する方法がいくつかあります
# 1. `passphrase` を true に設定すると、maa-cli は毎回パスワードの入力を求めます
# この方法は安全ですがやや面倒で、batch モードでは使用できません
# passphrase = true
# 2. `passphrase` を環境変数名に設定すると、maa-cli は環境変数をパスワードとして使用します
# この方法は平文のパスワードより安全ですが、環境変数は任意のプログラムからアクセスされる可能性があるため、依然として一定のリスクがあります
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. `passphrase` をコマンドに設定すると、maa-cli はそのコマンドを実行してパスワードを取得します
# パスワードマネージャーでパスワードを管理している場合、この方法が最も安全で便利かもしれません
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
# 4. ssh-agent でキーを管理する（**推奨**）
# ssh-agent はキーをメモリに保存するため、毎回パスワードを入力する必要がありません
# 注意、ssh-agent が起動しており、キーが追加され、SSH_AUTH_SOCK 環境変数が設定されていることを確認する必要があります
# use_ssh_agent = true # ssh-agent で認証を行います。true に設定すると ssh_key と passphrase フィールドは無視されます

# イベント情報とホットアップデートリソースファイルのダウンロードに関する設定
[hot_update]
# イベント情報の照会とホットアップデートリソースファイルのダウンロード用 API アドレス。空欄でデフォルトアドレスを使用
api_url = "https://api.maa.plus/MaaAssistantArknights/api"
# チェック間隔（秒）。この時間を超えるとファイルが再ダウンロードされます。0 はキャッシュを無効化することを意味し、デフォルトは 600
check_interval = 600
```

**注意事項**：

- MaaCore の更新チャンネルのうち `Alpha` は Windows でのみ利用可能です
- 一部のデフォルトリンクは GitHub を指しているため、中国国内では問題が発生する可能性があります。`api_url` と `download_url` を設定してミラーを使用できるほか、トップレベルの `github_proxy` を設定してプロキシ経由で GitHub release のダウンロードを高速化することもできます
- リソースのホットアップデートを有効にしていても、MaaCore のリソースをインストールする必要があります。リソースのホットアップデートにはすべてのリソースファイルは含まれず、更新可能な一部のリソースファイルのみが含まれるため、基本のリソースファイルは引き続きインストールが必要です
- リソースのホットアップデートは Git でリモートリポジトリをプルすることで行われます。バックエンドを `git` に設定した場合、`git` コマンドラインツールが利用可能である必要があります
- SSH プロトコルでリモートリポジトリをプルしたい場合は、`ssh_key` フィールドを設定する必要があります。このフィールドは SSH 秘密鍵を指すパスである必要があります
- SSH 秘密鍵がパスワードで保護されている場合は、鍵を復号するためのパスワードを提供するか、ssh-agent で鍵を管理する必要があります
- リモートリポジトリの `url` 設定は現時点ではリソースの初回インストール時にのみ有効です。リモートリポジトリのアドレスを変更したい場合は、`git` コマンドラインツールで手動で変更するか、対応するリポジトリを削除する必要があります。リポジトリの場所は `maa dir hot-update` で取得できます

## 参考設定

- [設定例][example-config]
- [個人設定][wangl-cc-dotfiles]

## JSON Schema

[`schemas` ディレクトリ][schema-dir] で maa-cli の JSON Schema ファイルを見つけることができます。これらのファイルを使用して設定ファイルを検証したり、エディタで自動補完を受け取ったりできます。

- カスタムタスクファイルの JSON Schema ファイルは [`task.schema.json`][task-schema]
- MaaCore 設定の JSON Schema ファイルは [`asst.schema.json`][asst-schema]
- CLI 設定の JSON Schema ファイルは [`cli.schema.json`][cli-schema]

[task-types]: ../../protocol/integration.md#タスク-タイプ一覧
[emulator-ports]: ../../manual/connection.md#ポート番号の取得
[playcover-doc]: ../../manual/device/macos.md
[waydroid-doc]: ../../manual/device/linux.md#✅-waydroid
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/main/home/dot_config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
