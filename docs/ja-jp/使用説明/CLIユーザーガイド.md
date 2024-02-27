---
icon: material-symbols:terminal
---

# CLI ユーザーガイド

## 機能紹介

- 定義済みタスクまたはカスタムタスクの実行、例 `maa fight`， `maa run <task>`;
- `maa install` と `maa update` を使用して `MaaCore`と `resources` をインストールおよび更新する；
- `maa self-update` を使用して自身を更新する。

## インストール

### Appimage

CLIはLinuxプラットフォーム上のMAAのデフォルト インタフェースであり、最新のAppimageパッケージを直接ダウンロードしてCLIを使用できます。

### パッケージマネージャー

#### macOS

[Homebrew](https://brew.sh/) を使用してインストールします：

```bash
brew install MaaAssistantArknights/tap/maa-cli
```

#### Linux

ArchLinux ユーザーは [AUR パッケージ](https://aur.archlinux.org/packages/maa-cli/) をインストールできます:

```bash
yay -S maa-cli
```

LinuxBrew ユーザーは [LinuxBrew](https://docs.brew.sh/Homebrew-on-Linux) を使用してインストールできます：

```bash
brew install MaaAssistantArknights/tap/maa-cli
```

### プリコンパイル済みバイナリ

プリコンパイルされたバイナリーを [maa-cli リリース・ページ](https://github.com/MaaAssistantArknights/maa-cli/releases/latest) からダウンロードし、結果の実行可能ファイルを任意の場所に配置できます。 各プラットフォームのファイル名は次のとおりです：

<table>
    <thead>
        <tr>
            <th>オペレーティング システム</th>
            <th>プロセッサのアーキテクチャ</th>
            <th>ファイル名</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan=2>Linux</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td>aarch64</td>
            <td>maa_cli-aarch64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td rowspan=2>macOS</td>
            <td>x86_64</td>
            <td rowspan=2>
              maa_cli-universal-apple-darwin.zip
            </td>
        </tr>
        <tr>
            <td>aaarch64</td>
        </tr>
        <tr>
            <td rowspan=2>Windows</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-pc-windows-msvc.zip</td>
        </tr>
    </tbody>
</table>

## 使う

`maa-cli` の主な機能はタスクを実行することであり、 `maa run <task>` でタスクを実行できます。ここでの `<task>` はタスクの名前であり、`maa list` を使用して使用可能なすべてのタスクを一覧表示できます。

詳細については、 `maa help` で見ることができます。

### タスクの実行

`maa-cli` の主な機能は、事前定義タスクやカスタム タスクなどのタスクを実行することです。

#### 事前定義タスク

- `maa startup [client]`: ゲームを起動し、メインインターフェイスに入ります。 `[client]` はクライアントバージョンで、空白のままにするとゲームクライアントは起動しません。
- `maa closedown`: ゲームを閉じます；
- `maa fight [stage]`: 作戦タスクを実行する。 `[stage]` はステージ名、例 `1-7`；空白のままにして、最後または現在のレベルを選択します；
- `maa copilot <maa_uri>`: 自動戦闘ミッションを実行します。ここで、 `<maa_uri>` は `maa://1234` ジョブの URI で、ローカル ファイル パス `./1234.json` のいずれかです；
- `maa roguelike [theme]`: ローグライクモードで戦闘ミッションを実行する、 `[theme]` はローグライクモードのテーマであり、オプションの値は `Phantom` `Mizuki` と `Sami`；

#### カスタム タスク

カスタムタスクは、 `maa run <task>` で実行できます。ここの `<task>` はクエストの名前であり、利用可能なすべてのクエストを `maa list` で一覧表示できます。
特定のタスク定義は、 [設定サブセクション](#カスタムタスクの定義) にあります。

#### タスクの概要

`maa-cli` タスクの概要は、タスクの実行後に、各サブタスクの実行時間と結果を含む標準出力に出力されます。 `--no-summary` オプションを使用して、タスクの要約を無効にすることができます。

タスクの概要には、主に各タスクの実行時間が含まれます。また、以下のタスクに関する追加情報も含まれています：

- 自動戦闘 `fight`: レベル名、回数、ドロップ統計；
- インフラシフト `infrast`: 各施設に常駐するオペレーター（製造および取引ステーション向けの製品の種類を含む）；
- 公開求人 `recruit`: 公開求人タグの更新回数、採用数、検出されたタグと星評価；
- ローグ `roguelike`: 実行される回数、それが投資される回数。

#### ログ出力

`maa-cli` デフォルトでは、ログは stderr に出力されます。低から高までのログ出力レベルは、 `Error`，`Warn`，`Info`，`Debug` と `Trace`。デフォルトのログ出力レベルは `Warn`。ログレベルは、 `MAA_LOG` で `MAA_LOG=debug` などの環境変数で設定できます。また、ログ出力のレベルを増減するには、 `-v` または `-q` を使用します。

`--log-file` オプションを使用してログをファイルに出力し、ログを `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log` に保存します。ここで、 `$(maa dir log)` はログディレクトリで、 `maa dir log` から取得できます。ログ・ファイルへのパスは、 `--log-file=path/to/log` で指定することもできます。

### インストールとアップデート

#### MaaCore のインストールとアップデート

`MaaCore`およびリソースをインストールおよびアップデートするには、 `maa install` および `maa update` を使用できます。詳細な情報は、 `maa help install` および `maa help update` を使用して取得できます。

#### リソースのホットアップデート

ゲームのアップデートにより、 `MaaCore` は最新のリソースが必要になる場合があります。リソースを更新するには、 `maa hot-update` を使用するか、リソースの自動更新を設定できます。詳細については、[CLI関連設定](#maa-cli-関連設定)を参照してください。

#### maa-cliのアップデート

`maa-cli`自体を更新するには、 `maa self update` を使用できます。パッケージマネージャーでインストールされた `maa-cli` の場合、パッケージマネージャーを使用して `maa-cli` を更新する必要があります。

その他のコマンドについては、 `maa help` を使用して取得できます。

### その他のサブコマンド

- `maa list`: 利用可能なすべてのタスクを一覧表示します。
- `maa dir <dir>`: 特定ディレクトリのパスを取得します。たとえば、 `maa dir config` を使用して構成ディレクトリのパスを取得できます。
- `maa version`: `maa-cli`および`MaaCore`のバージョン情報を取得します。
- `maa convert <input> [output]`: `JSON`、`YAML`、または`TOML`形式のファイルを他の形式に変換します。
- `maa complete <shell>`: 自動完了スクリプトを生成します。
- `maa activity [client]`: ゲームの現在のアクティビティ情報を取得します。 `client` はクライアントタイプであり、デフォルトは `Official` です。


## 設定

### 設定ディレクトリー

`maa-cli` 設定ファイルは、 `maa dir config` で取得できる特定の設定ディレクトリーにあります。 設定ディレクトリは、環境変数 `MAA_CONFIG_DIR` で変更することもできます。 以下の例では、設定ディレクトリを表すために `$MAA_CONFIG_DIR` を使用します。

すべての構成ファイルは、TOML、YAML、またはJSON形式にすることができますが、以下の例では、TOML形式を使用し、ファイル拡張子 `.toml` を使用します。 ただし、正しいファイル拡張子がある限り、これら3つの形式のいずれかを混在させることができます。

### カスタムタスクの定義

各カスタムタスクは個別のファイルであり、`$MAA_CONFIG_DIR/tasks` ディレクトリに配置する必要があります。

#### 基本構造

タスクファイルには複数のサブタスクが含まれており、各サブタスクは、次のオプションを持つ [MAAタスク](../プロトコルドキュメント/統合ドキュメント.md#asstappendtask) です：

```toml
[[tasks]]
name = "ゲームを起動する" # タスク名、オプション、デフォルトでタスクタイプです
type = "StartUp" # MAA タスクのタイプ
params = { client_type = "Official", start_game_enabled = true } # MAA タスクのパラメーター
```

#### タスク条件

いくつかの条件に基づいて異なるパラメータでタスクを実行する場合は、タスクの複数のバリエーションを定義できます：

```toml
[[tasks]]
name = "基地シフト"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # カスタム インフラスト ラクチャ プランのファイル名は `$MAA_CONFIG_DIR/infrast` に配置する必要があります。

# プラン 1 は 12:00:00 まで、プラン 2 は 12:00:00 から 18:00:00 まで、プラン 0 は 18:00:00 以降に使用します。
[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # start が定義されていない場合は、00:00:00 になります。
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00", end = "18:00:00" }
params = { plan_index = 2 }

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
params = { plan_index = 0 }
```

ここでの `condition` フィールドは、どの変種を使用するかを決定するために使用され、一致する変種の `params` フィールドがタスクのパラメータにマージされます。

**注意**: カスタムインフラ計画ファイルが相対パスを使用する場合は、 `$MAA_CONFIG_DIR/infrast` を基準にする必要があります。また、インフラファイルは `MaaCore` によって読み込まれるため、ファイルの形式は `JSON` である必要があります。同時に、`maa-cli` はインフラファイルを読み込まず、定義された時間帯に基づいてサブプランを選択しません。したがって、`condition` フィールドを使用して、正しいインフラ計画を正しい時間帯に使用するように、パラメータ内の `plan_index` フィールドを指定する必要があります。これにより、適切な時間帯に正しいインフラ計画を使用できます。

`Time` 条件に加えて、`DateTime`、`Weekday`、`DayMod` 条件もあります。 `DateTime` 条件は時間帯を指定するために使用され、`Weekday` 条件は週の特定の日を指定するために使用されます。`DayMod` は以下の複数日のシフトを参照してください。

```toml
[[tasks]]
type = "Fight"

# 夏のイベントでは SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 夏のイベント以外、火曜、木曜、土曜では CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 別の時間には 1-7
[[tasks.variants]]
params = { stage = "1-7" }
```

上記の条件に加えて、ホットアップデートリソースに依存する `OnSideStory` 条件があります。この条件を有効にすると、`maa-cli` は対応するリソースを読み込んで現在有効なイベントがあるかどうかを判断し、対応する変種を選択します。たとえば、前述の夏イベント中に `SL-8` を周回する条件は、`{ type = "OnSideStory", client = "Official" }` と簡略化することができます。ここでの `client` パラメータは使用するクライアントを決定するために使用されます。異なるクライアントでは、イベントの時間が異なるため、公式サーバーまたは bilibili サーバーを使用するユーザーにとってはこれを省略できます。この条件を使用すると、毎回のイベント更新後に、特定のステージのみを更新する必要があり、対応するイベントの開始時間を手動で編集する必要がありません。

基本条件の他に、条件を論理演算するために `{ type = "And", conditions = [...] }`、`{ type = "Or", conditions = [...] }`、`{ type = "Not", condition = ... }` を使用できます。

複数日のシフトを設定したい場合は、`DayMod` と `Time` を組み合わせて使用することができ、複数日のシフトを実現できます。たとえば、2日ごとに6回のシフトを切り替えたい場合は、次のように記述できます：

```toml
[[tasks]]
name = "基地シフト (2天6班)"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json"

# 第一班，第一天 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 0 }
[tasks.variants.condition]
type = "And"
conditions = [
    # ここでは、 divisor は期間を指定するために使用され、 remainder はオフセットを指定するために使用されます
    # オフセットは num_days_since_ce % divisor に等しい
    # num_days_since_ce これは西暦からの日数で、0001-01-01は最初の日です
    # `maa remainder <divisor>` で当日のオフセットを取得できます
    # たとえば、 2024-1-27 は 738,912 日目であり、738912 % 2 = 0 です
    # 当日のオフセットが 0 の場合、この条件が一致します
    { type = "DayMod", divisor = 2, remainder = 0 },
    { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第二班，第一天 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 1 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 0 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第三班，第一天 20:00:00 - 第二天 4:00:00
[[tasks.variants]]
params = { plan_index = 2 }
[tasks.variants.condition]
# ここでは Or 条件を使用する必要があり、直接使用することはできません Time { start = "20:00:00", end = "04:00:00" }
# この場合、翌日の 00:00:00 - 04:00:00 はマッチしません
# もちろん、スケジュールを調整して日を越えないようにする方が良いですが、ここではデモンストレーションのためだけに
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

# 第四班，第二天 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 3 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第五班，第二天 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 4 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第六班，第二天 20:00:00 - 第三天（新的第一天）4:00:00
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

デフォルトの戦略では、複数の変種が一致した場合、最初の変種が使用されます。条件が指定されていない場合、変種は常に一致するため、条件がない変種を最後に置いて、デフォルトの状況として使用できます。

`strategy` フィールドを使用して一致戦略を変更できます：

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # 或いは "first" (デフォルト)

# 日曜日の夜に期限切れになる理性回復薬をすべて使います
[[tasks.variants]]
params = { expiring_medicine = 1000 }

[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# デフォルトは 1-7
[[tasks.variants]]
params = { stage = "1-7" }

# 夏のイベント以外、火曜、木曜、土曜では CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 夏のイベントでは SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

この例と前述の例では同じステージを周回しますが、日曜日の夜には期限切れの理性薬をすべて使用します。`merge` 戦略では、複数の変種が一致した場合、後続の変種のパラメータが前の変種のパラメータにマージされます。複数の変種が同じパラメータを持つ場合、後続の変種のパラメータが前の変種のパラメータを上書きします。

一致する変種がない場合、タスクは実行されません。これは特定の条件下でのみサブタスクを実行するために使用できます：

```toml
# FPショップの操作は、18:00:00 以降にのみ利用できます。
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

#### ユーザー入力

一部のタスクでは、実行時にいくつかのパラメータを入力する必要がある場合があります。たとえば、ステージ名などです。対応する入力が必要なパラメータを `Input` または `Select` タイプに設定することができます：

```toml
[[tasks]]
type = "Fight"

# ステージを選択
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
[tasks.variants.params.stage]
# オプションステージの場合、少なくとも1つのオプション値を指定する必要があります
# 省略可能な値は、単一の値、または値と説明の両方を含むテーブルです
alternatives = [
    "SL-7", # "1. SL-7" に表示されます
    { value = "SL-8", desc = "轻锰矿" } # "2. SL-8 (マンガン)" に表示されます
]
default_index = 1 # デフォルト値のインデックスは、1から始まって、設定されていない場合は、null値を入力すると、再度プロンプトが表示されます
description = "a stage to fight in summer event" # 説明、オプション
allow_custom = true # カスタム値を許可するかどうかがfalseであるかどうかにかかわらず、許可されている場合、非整数の値はカスタム値として扱われます

# 入力は不要
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# ステージを入力する
[[tasks.variants]]
[tasks.variants.params.stage]
default = "1-7" # デフォルトステージ、オプション(デフォルト値がない場合は、null値を入力すると再度プロンプトが表示されます)
description = "a stage to fight" # 説明、オプション
[tasks.variants.params.medicine]
# 依存パラメータ、キーはパラメータ名であり、値は依存パラメータの期待値です
# 設定すると、このパラメータは、すべての依存パラメータが期待値を満たしている場合にのみ入力を求められます
deps = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

`Input` タイプの場合、タスクを実行すると値の入力を求められます。空の値を入力した場合、デフォルト値があればそれが使用され、そうでない場合は再入力を求められます。

`Select` タイプの場合、タスクを実行するとインデックスまたはカスタム値（許可されている場合）の入力が求められます。空の値を入力した場合、デフォルト値があればそれが使用され、そうでない場合は再入力を求められます。

`--batch` オプションは、タスクを実行する際にすべての入力をスキップするために使用できます。これにより、デフォルト値が使用されます。ただし、デフォルト値の設定されていない入力がある場合はエラーが発生します。

### MaaCore 関連の設定

MaaCore 関連の設定は `$MAA_CONFIG_DIR/asst.toml` に配置する必要があります。
現在、以下の設定が含まれています：

```toml
[connection]
type = "ADB"
adb_path = "adb"
device = "emulator-5554"
config = "CompatMac"

[resource]
global_resource = "YoStarEN"
platform_diff_resource = "iOS"
user_resource = true

[static_options]
cpu_ocr = false
gpu_ocr = 1

[instance_options]
touch_mode = "MAATouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```

#### 接続設定

`[connection]` 関連のフィールドは、MaaCore との接続方法とパラメータを指定するために使用されます。現在利用可能な接続方法は `ADB` と `PlayTools` です。

`ADB` 接続を使用する場合、`adb` のパスとデバイスのシリアル番号を指定する必要があります：

```toml
[connection]
type = "ADB"
adb_path = "adb" # adb 実行可能ファイルのパス
device = "emulator-5554" # Android端末のシリアル番号
config = "General" # MAA Connect の設定
```

注意：ここでのデバイスは、`adb -s` が受け入れる任意の値（例: `emulator-5554` や `127.0.0.1:5555`）です。

`PlayTools` 接続を使用する場合、`PlayTools` のアドレスを指定する必要があります：

```toml
[connection]
type = "PlayCover"
address = "localhost:1717" # PlayTools のアドレス
config = "CompatMac" # MAA Connect の設定
```

いずれの場合も `config` を提供する必要があります。この値は、いくつかのプラットフォームやエミュレータに関連する設定を指定するために MaaCore に渡されます。Linux の場合はデフォルトで `CompatPOSIXShell`、macOS の場合は `CompatMac`、Windows の場合は `General` が使用されます。さらに詳細な設定については、リソースフォルダ内の `config.json` ファイルを参照してください。

#### リソース設定

`[resource]` 関連のフィールドは、MaaCore がロードするリソースを指定するために使用されます：

```toml
[resource]
global_resource = "YoStarEN" # 中国語以外のクライアントのリソース
platform_diff_resource = "iOS" # Android バージョン以外のリソース
user_resource = true # ユーザー定義リソースをロードするかどうか
```

簡体字中国語以外のゲームクライアントを使用する場合、`MaaCore` はデフォルトで簡体字中国語のリソースをロードしますので、非中国語版のリソースを読み込むために `global_resource` フィールドを指定する必要があります。iOS バージョンのゲームクライアントを使用する場合、`platform_diff_resource` フィールドを指定して iOS バージョンのリソースを読み込む必要があります。これらはどちらもオプションであり、これらのリソースを読み込む必要がない場合は、これらのフィールドを空に設定することができます。また、これらは自動的に設定される場合もあります。`startup` タスクで `client_type` フィールドを指定した場合、`global_resource` が対応するクライアントのリソースとして設定され、`PlayTools` 接続を使用する場合は `platform_diff_resource` が `iOS` に設定されます。最後に、ユーザー定義のリソースをロードする場合は、`user_resource` フィールドを `true` に設定する必要があります。

#### スタチチオプチン

`[static_options]` 関連のフィールドは、MaaCore のスタチチオプチンを指定するために使用されます。詳細については [統合ドキュメント](../プロトコルドキュメント/統合ドキュメント.html#asstsetstaticoption) を参照してください：

```toml
[static_options]
cpu_ocr = false # CPU OCR を使用するかどうか、デフォルトで CPU OCR が使用されます
gpu_ocr = 1 # GPU OCR を使用するときに使用される GPU ID で、この値を空白のままにすると、CPU OCR が使用されます
```

#### インスタンスのオプション

`[instance_options]` 関連フィールドは、MaaCore インスタンスのオプションを指定するために使用されます。詳細については [統合ドキュメント](../プロトコルドキュメント/統合ドキュメント.html#asstsetinstanceoption) を参照してください：

```toml
[instance_options]
touch_mode = "ADB" # タッチモードを使用し、"ADB"，"MiniTouch"，"MAATouch" または "MacPlayTools" として選択可能な値を使用します
deployment_with_pause = false # デプロイ中にゲームを一時停止するかどうか
adb_lite_enabled = false # adb-liteを使用するかどうか
kill_adb_on_exit = false # 終了時にADBを強制終了するかどうか
```

注意，`touch_mode` のオプションは `MacPlayTools` と `PlayTools` 接続方式に関連付けられています。`PlayTools` 接続を使用する場合、`touch_mode` は自動的に `MacPlayTools` に設定されます。

### `maa-cli` 関連設定

`maa-cli` に関連する設定は、 `$MAA_CONFIG_DIR/cli.toml` に配置する必要があります。現在、以下の設定が含まれています：

```toml
# MaaCoreのインストールと更新に関する設定
[core]
channel = "Stable" # 更新チャネル。"Alpha"、"Beta"、"Stable"のいずれかを選択します。デフォルトは"Stable"です。
test_time = 0    # ミラーリング速度をテストするための時間。0の場合、テストしないことを意味します。デフォルトは3です。
# MaaCoreの最新バージョンを取得するAPIのアドレス。空欄の場合、デフォルトのアドレスが使用されます。
api_url = "https://github.com/MaaAssistantArknights/MaaRelease/raw/main/MaaAssistantArknights/api/version/"

# MaaCoreに関連するコンポーネントをインストールするかどうかの設定。推奨しません。個別にインストールするとバージョンの不一致が発生し、問題が発生する可能性があります。このオプションは将来のバージョンで削除される可能性があります。
[core.components]
library = true  # MaaCore のライブラリをインストールするかどうか。デフォルトは true です。
resource = true # MaaCore のリソースをインストールするかどうか。デフォルトは true です。

# CLIの更新に関する設定
[cli]
channel = "Stable" # 更新チャネル。"Alpha"、"Beta"、"Stable"のいずれかを選択します。デフォルトは"Stable"です。
# maa-cliの最新バージョンを取得するAPIのアドレス。空欄の場合、デフォルトのアドレスが使用されます。
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# プリコンパイルされたバイナリファイルをダウンロードするアドレス。空欄の場合、デフォルトのアドレスが使用されます。
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

# CLIに関連するコンポーネントをインストールするかどうかの設定
[cli.components]
binary = true # maa-cliのバイナリファイルをインストールするかどうか。デフォルトはtrueです。

# リソースのホットアップデートに関する設定
[resource]
auto_update = true  # タスクを実行するたびにリソースを自動的に更新するかどうか。デフォルトは false です。
backend = "libgit2" # リソースのホットアップデートバックエンド。 "git" または "libgit2" を選択します。デフォルトは "git" です。

# リソースのホットアップデートリモートリポジトリに関連する設定
[resource.remote]
branch = "main" # リモートリポジトリのブランチ、デフォルトは "main" です
# リモートリポジトリの URL 、 SSH を使用する場合は、ssh_key へのパスを設定する必要があります
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# ssh_key = "~/.ssh/id_ed25519" # SSH 秘密鍵のパス (拡張可能) 、ただし、他の環境変数はサポートされていません
```

**注意事項**：

- MaaCore の更新チャネルの  `Alpha` は、Windows でのみ使用できます。
- CLIのデフォルトのAPIリンクとダウンロードリンクは全てGitHubリンクなので、中国では多少の問題があるかもしれませんが、 `api_url` と `download_url` を設定することでミラーサイトを利用することができます。
- リソースのホット アップデートを有効にした場合でも、 `MaaCore` リソースをインストールする必要があります。リソースのホット アップデートにはすべてのリソース ファイルが含まれているわけではなく、更新可能なリソース ファイルの一部のみが含まれるため、基本リソース ファイルをインストールする必要があります。
- リソースのホット アップデートはGitを介してリモート リポジトリから取得され、バックエンドが `git` に設定されている場合は、 `git` コマンドラインツールが使用可能である必要があります。
- SSH プロトコルを使用してリモートリポジトリをプルする場合は、SSH 秘密鍵へのパスである `ssh_key` フィールドを構成する必要があります。
- リモートリポジトリの `url` 設定は現在、リソースの最初のインストールでのみ有効であり、リモートリポジトリのアドレスを変更する場合は、 `git` コマンドラインツールを使用して手動で変更するか、或いはリポジトリを削除する。 リポジトリの場所は、 `maa dir hot-update` で取得できます。
- リモートリポジトリの `url` はローカル言語に従って自動的に設定され、言語が簡体字中国語の場合、リモートリポジトリの `url` は国内ミラーの `https://git.maa-org.net/MAA/MaaResource.git` に設定され、それ以外の場合は Github に設定されます。 中国にいて簡体字中国語を使用していない場合、または海外で簡体字中国語を使用している場合は、最適なエクスペリエンスを得るために手動で設定する必要があります。

### JSON Schema

`maa-cli` の JSON Schema ファイルは、CLI リポジトリーの `maa-cli/schemas` ディレクトリーにあり、これを使用して構成ファイルを検証したり、エディターでオートコンプリートを取得したりできます。
タスク ファイルの JSON Schema ファイルは [`task.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)；
MaaCore の構成の JSON Schema ファイルは、 [`asst.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)；
CLI 設定の JSON Schema ファイルは [`cli.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)。
