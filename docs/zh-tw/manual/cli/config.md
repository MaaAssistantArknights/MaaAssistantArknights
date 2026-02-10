---
order: 3
icon: material-symbols:settings
---

# 配置

## 配置目錄

maa-cli 配置檔案位於特定的配置目錄中，您可以透過 `maa dir config` 獲取配置目錄路徑。配置目錄也可以透過環境變數 `MAA_CONFIG_DIR` 更改。在下文的範例中，我們將以 `$MAA_CONFIG_DIR` 代表配置目錄。

所有的配置檔案皆可使用 TOML、YAML 或 JSON 格式。在下文範例中，我們將使用 TOML 格式並以 `.toml` 作為副檔名，但您可以根據需求混合使用這三種格式，只要副檔名正確即可。

此外，部分任務接受 `filename` 作為參數。若您使用相對路徑，該路徑將相對於配置目錄下的對應子目錄。例如：自定義基建計畫檔案的相對路徑應相對於 `$MAA_CONFIG_DIR/infrast`，而保全派駐的作業檔案則相對於 `$MAA_CONFIG_DIR/ssscopilot`。

## 自定義任務

每一個自定義任務都是獨立的檔案，應存放於 `$MAA_CONFIG_DIR/tasks` 目錄中。

### 基本結構

一個任務檔案包含多個子任務，每個子任務都是一個 MAA 任務，包含以下選項：

```toml
[[tasks]]
name = "啟動遊戲" # 任務名稱，選填，預設為任務類型
type = "StartUp" # 任務類型
params = { client_type = "Official", start_game_enabled = true } # 對應任務的參數
```

具體的任務類型與參數可以參考 [MAA 整合文件][task-types]。請注意，目前 maa-cli 不會驗證參數名稱與數值是否正確，即使出錯也不會產生任何錯誤訊息，除非 MaaCore 在執行時偵測到錯誤。

### 任務條件

如果您想要根據某些條件執行不同參數的任務，您可以定義多個任務的變體：

```toml
[[tasks]]
name = "基建換班"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # 自定義的基建計畫檔案名稱應位於 `$MAA_CONFIG_DIR/infrast`

# 在 18:00:00 到隔天的 04:00:00 使用計畫 0，在 12:00:00 之前使用計畫 1，之後使用計畫 2
[[tasks.variants]]
condition = { type = "Time", start = "18:00:00", end = "04:00:00" } # 當結束時間小於開始時間時，結束時間被視為隔天的時間
params = { plan_index = 0 }

[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 如果省略開始時間，那麼只要目前時間小於結束時間，這個條件就會被匹配
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00" } # 如果省略結束時間，那麼只要目前時間大於開始時間，這個條件就會被匹配
params = { plan_index = 2 }
```

這裡的 `condition` 欄位用於確定應使用哪一個變體，而匹配成功的變體其 `params` 欄位將會被合併到任務的參數中。

**請注意**：如果您的自定義基建計畫檔案使用相對路徑，應該相對於 `$MAA_CONFIG_DIR/infrast`。此外，由於基建檔案是由 MaaCore 而非 maa-cli 讀取的，因此這些檔案的格式必須是 JSON。同時，maa-cli 不會讀取基建檔案，也不會根據其中定義的時間段來選擇對應的子計畫。因此，必須透過 `condition` 欄位來指定在對應時間段使用正確的基建計畫參數中的 `plan_index` 欄位。這樣可以確保在適當的時間段使用正確的基建計畫。

除了 `Time` 條件，還有 `DateTime`，`Weekday`，`DayMod` 條件。`DateTime` 條件用於指定一個時間段，`Weekday` 條件用於指定一週中的某些天，`DayMod` 用於指定一個自定義週期的某些天。

```toml
[[tasks]]
type = "Fight"

# 在夏活期間，刷 SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 在夏活期間以外的週二、週四和週六，刷 CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"], timezone = "Official"}
params = { stage = "CE-6" }

# 其他時間刷 1-7
[[tasks.variants]]
params = { stage = "1-7" }
```

對於上述所有與時間相關的條件，都可以透過 `timezone` 參數來指定時區，這個參數的值可以是一個數字，表示與 UTC 的偏移量，如果您的時區是東八區，那麼您可以設定 `timezone = 8`。這個參數也可以是一個客戶端類型，例如 `timezone = "Official"`，這樣將會使用官服對應的伺服器時間來判斷。**注意**，官服的時區不是東八區而是東四區，因為遊戲中每天開始時間是 04:00:00 而不是 00:00:00。如果不指定時區，那麼直接使用您的本地時區。

除了上述確定的條件之外，還有一個依賴於熱更新資源的條件 `OnSideStory`，當您啟用該條件後，maa-cli 會嘗試讀取對應的資源來判斷目前是否有正在開啟的活動，如果有，那麼對應的變體會被匹配。例如上述夏活期間刷 `SL-8` 的條件就可以簡化為 `{ type = "OnSideStory", client = "Official" }`，這裡的 `client` 參數用於確定您使用的客戶端，因為不同客戶端的活動時間不同，對於使用官服或 B 服的使用者，這可以省略。透過這個條件，每次活動更新之後，您只需要更新需要刷的關卡，而不需要手動編輯對應活動的開放時間。

除了以上基礎條件之外，您可以使用 `{ type = "And", conditions = [...] }`，`{ type = "Or", conditions = [...] }`, `{ type = "Not", condition = ... }` 來對條件進行邏輯運算。
對於想要基建多天排班的使用者，可以將 `DayMod` 和 `Time` 組合使用，實現多天排班。例如，您想要實現每兩天換六次班，那麼您可以這樣寫：

```toml
[[tasks]]
name = "基建换班 (2天6班)"
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
    # 這裡的 divisor 用來指定週期，remainder 用來指定偏移量
    # 偏移量等於 num_days_since_ce % divisor
    # 這裡的 num_days_since_ce 是公元以來的天數，0001-01-01 是第一天
    # 當天偏移量您可以透過 `maa remainder <divisor>` 來獲取.
    # 比如，2024-1-27 是第 738,912 天，那麼 738912 % 2 = 0
    # 當天的偏移量為 0，那麼本條件將會被匹配
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
# 注意這裡必須使用 Or 條件，不能直接使用 Time { start = "20:00:00", end = "04:00:00" }
# 在這種情況下，第二天的 00:00:00 - 04:00:00 不會被匹配
# 當然透過調整您的排班時間避免跨天是更好的選擇，這裡只是為了演示
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

在預設策略下，如果有多個變體被匹配，第一個將會被使用。如果沒有給出條件，那麼變體將會總是被匹配，所以您可以把沒有條件的變體放在最後，作為預設的情況。

您可以使用 `strategy` 欄位來改變匹配策略：

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # 或者 "first" (預設)

# 在週日晚上使用所有將要過期的理智藥
[[tasks.variants]]
params = { expiring_medicine = 1000 }

[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# 預設刷 1-7
[[tasks.variants]]
params = { stage = "1-7" }

# 在週二、週四和週六，刷 CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 在夏活期間，刷 SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

這個範例和上面的範例將刷同樣的關卡，但是在週日晚上，將會使用所有將要過期的理智藥。在 `merge` 策略下，如果有多個變體被匹配，後面的變體參數將合併入前面變體的參數中。如果多個變體都有相同的參數，那麼後面變體的參數將會覆蓋前面變體的參數。

如果沒有變體被匹配，那麼任務將不會被執行，這可以用於只在特定的條件下執行子任務：

```toml
# 只在 18:00:00 之後進行信用商店相關的操作
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

### 使用者輸入

對於一些任務，您可能想要在執行時輸入一些參數，例如關卡名稱。您可以將對應需要輸入的參數設定為 `Input` 或者 `Select` 類型：

```toml
[[tasks]]
type = "Fight"

# 選擇一個關卡
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
[tasks.variants.params.stage]
# 可選的關卡，必須提供至少一個可選值
# 可選值可以是一個值，也可以是同時包含值和描述的一個表
alternatives = [
    "SL-7", # 將被顯示為 "1. SL-7"
    { value = "SL-8", desc = "輕錳礦" } # 將被顯示為 "2. SL-8 (輕錳礦)"
]
default_index = 1 # 預設值的索引，從 1 開始，如果沒有設定，輸入空值將會重新提示輸入
description = "a stage to fight in summer event" # 描述，選填
allow_custom = true # 是否允許輸入自定義的值，預設為 false，如果允許，那麼非整數的值將會被視為自定義的值

# 無需任何輸入
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 輸入一個關卡
[[tasks.variants]]
[tasks.variants.params.stage]
default = "1-7" # 預設的關卡，選填（如果沒有預設值，輸入空值將會重新提示輸入）
description = "a stage to fight" # 描述，選填

# 當輸入的關卡是 1-7 時，需要輸入使用理智藥的數量
[tasks.variants.params.medicine]
# 參數可以「設定」為條件參數，這樣只有滿足條件時才需要輸入
# conditions 欄位是一個表，其中鍵是同一層級下其他參數名稱，值是期望的值
# 這裡的條件是 stage 為 1-7，如果存在多個條件，那麼所有條件都必須滿足
conditions = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

對於 `Input` 類型，執行任務時您會被提示輸入一個值。若輸入為空且設有預設值，將自動採用該預設值，否則會提示重新輸入。
對於 `Select` 類型，執行任務時您會被提示輸入索引或自訂值（若允許）。若輸入為空且設有預設值，將自動採用該預設值，否則會提示重新輸入。

`--batch` 選項可用於在執行任務時跳過所有輸入，此時系統將自動採用預設值；若有任何輸入項未設定預設值，則會產生錯誤。

## MaaCore 相關配置

與 MaaCore 相關的配置需要放在 `$MAA_CONFIG_DIR/profiles` 目錄中。該目錄下的每一個檔案都是一個配置檔案，您可以透過 `-p` 或者 `--profile` 選項來指定配置檔案名稱，不指定時嘗試讀取 `default` 配置。

目前支援的配置欄位如下：

```toml
[connection]
preset = "MuMuPro"
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
touch_mode = "MaaTouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```

### 連線配置

`[connection]` 相關欄位用於指定 MaaCore 連線遊戲的參數：

```toml
[connection]
adb_path = "adb" # adb 執行檔的路徑，預設值為 "adb"，這意味著 adb 執行檔在環境變數 PATH 中
address = = "emulator-5554" # 連線位址，例如 "emulator-5554" 或者 "127.0.0.1:5555"
config = "General" # 連線配置，通常不需要修改
```

`adb_path` 是 `adb` 執行檔的路徑，您可以指定其路徑，或者將其添加到環境變數 `PATH` 中，以便 MaaCore 可以找到它。大多數模擬器內建 `adb`，您可以直接使用其內建的 `adb`，而不需要額外安裝，否則您需要自行安裝 `adb`。`address` 是 `adb` 的連線位址。對於模擬器，您可以使用 `127.0.0.1:[連接埠號]`，常用的模擬器連接埠號可參閱 [常見問題][emulator-ports]。如果您沒有指定 `address`，那麼會嘗試透過 `adb devices` 來獲取連線的設備，如果有多個設備連線，那麼將會使用第一個設備，如果沒有找到任何設備，那麼將會嘗試連線到 `emulator-5554`。`config` 於指定一些平台和模擬器相關的配置。對於 Linux 它預設為 `CompatPOSIXShell`，對於 macOS 它預設為 `CompatMac`，對於 Windows 它預設為 `General`。更多可選配置可以在資源資料夾中的 `config.json` 檔案中找到。

對於一些常用的模擬器，您可以直接使用 `preset` 來使用預設的配置：

```toml
[connection]
preset = "MuMuPro" # 使用 MuMuPro 預設的連線配置
adb_path = "/path/to/adb" # 如果您需要的話，可以覆蓋預設的 adb 路徑，大多數情況下您不需要這麼做
address = "127.0.0.1:7777" # 如果您需要的話，可以覆蓋預設的位址
```

目前只有 `MuMuPro` 一個模擬器的預設，如果有其他常用模擬器的預設，歡迎提交 issue 或者 PR。

#### 特殊預設

目前預先配置了兩種預設，為 `PlayCover (macOS)`, `Waydroid (Linux)`

- `PlayCover` 用於在 macOS 上連線直接透過 `PlayCover` 原生執行的遊戲客戶端。這種情況下不需要指定 `adb_path` 且 `address` 不是 `adb` 連線的位址而是 `PlayTools` 的位址，具體使用方式可參考 [PlayCover 支援文件][playcover-doc].

- `Waydroid` 用於在 Linux 上連線直接透過 `Waydroid` 原生執行的遊戲客戶端。這種情況下仍需要指定 `adb_path`，具體使用方式可參考 [Waydroid 支援文件][waydroid-doc].

### 資源配置

`[resource]` 相關欄位用於指定 MaaCore 載入的資源：

```toml
[resource]
global_resource = "YoStarEN" # 非中文版本的資源
platform_diff_resource = "iOS" # 非安卓版本的資源
user_resource = true # 是否載入使用者自定義的資源
```

當使用非簡體中文遊戲客戶端時，由於 MaaCore 預設載入的資源是簡體中文的，您需要指定 `global_resource` 欄位來載入非中文版本的資源。當使用 iOS 版本的遊戲客戶端時，您需要指定 `platform_diff_resource` 欄位來載入 iOS 版本的資源。這兩者都是選填的，如果您不需要載入這些資源，可以將這兩個欄位設定為空。其次，這兩者也會被自動設定，如果您的 `startup` 任務中指定了 `client_type` 欄位，那麼 `global_resource` 將會被設定為對應客戶端的資源，而當您使用 `PlayTools` 連線時，`platform_diff_resource` 將會被設定為 `iOS`。最後，當您想要載入使用者自定義的資源時，您需要將 `user_resource` 欄位設定為 `true`。

### 靜態選項

`[static_options]` 相關欄位用於指定 MaaCore 靜態選項：

```toml
[static_options]
cpu_ocr = false # 是否使用 CPU OCR，預設使用 CPU OCR
gpu_ocr = 1 # 使用 GPU OCR 時使用的 GPU ID，如果這個值被留空，那麼將會使用 CPU OCR
```

### 實例選項

`[instance_options]` 相關欄位用於指定 MaaCore 實例的選項：

```toml
[instance_options]
touch_mode = "ADB" # 使用的觸控模式，可選值為 "ADB"、"MiniTouch"、"MaaTouch" 或者 "MacPlayTools"
deployment_with_pause = false # 是否在部署時暫停遊戲
adb_lite_enabled = false # 是否使用 adb-lite
kill_adb_on_exit = false # 是否在退出時殺死 adb
```

請注意，`touch_mode` 可選項 `MacPlayTools` 和連線方式 `PlayTools` 綁定。當您使用 `PlayTools` 連線時，`touch_mode` 將會被強制設定為 `MacPlayTools`。

## CLI 相關配置

CLI 相關的配置需要放在 `$MAA_CONFIG_DIR/cli.toml` 中。目前其包含的配置如下：

```toml
# MaaCore 安裝和更新相關配置
[core]
channel = "Stable" # 更新通道，可選值為 "Alpha"、"Beta" "Stable"，預設為 "Stable"
test_time = 0    # 用於測試鏡像站速度的時間，0 表示不測試，預設為 3
# 查詢 MaaCore 最新版本的 api 位址，留空表示使用預設位址
api_url = "https://github.com/MaaAssistantArknights/MaaRelease/raw/main/MaaAssistantArknights/api/version/"

# 配置是否安裝 MaaCore 對應的組件，不推薦使用，分開安裝可能會導致版本不一致，從而導致一些問題，該選項可能在未來的版本中移除
[core.components]
library = true  # 是否安裝 MaaCore 的函式庫，預設為 true
resource = true # 是否安裝 MaaCore 的資源，預設為 true

# CLI 更新相關配置
[cli]
channel = "Stable" # 更新通道，可選值為 "Alpha"、"Beta" "Stable"，預設為 "Stable"
# 查詢 maa-cli 最新版本的 api 位址，留空表示使用預設位址
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# 下載預編譯執行檔的位址，留空表示使用預設位址
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

# 配置是否安裝 maa-cli 對應的組件
[cli.components]
binary = true # 是否安裝 maa-cli 的執行檔，預設為 true

# 資源熱更新相關配置
[resource]
auto_update = true  # 是否在每次執行任務時自動更新資源，預設為 false
warn_on_update_failure = true # 是否在更新失敗時發出警告而不是直接報錯
backend = "libgit2" # 資源熱更新後端，可選值為 "git" 或者 "libgit2"，預設為 "git"

# 資源熱更新遠端倉庫相關配置
[resource.remote]
branch = "main" # 遠端倉庫的分支，預設為 "main"
# 遠端資源倉庫的 URL，留空以使用預設 URL
# GitHub 倉庫支援 HTTPS 和 SSH 兩種協定存取，建議使用 HTTPS 協定，因為通常情況下不需要額外配置
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# 如果您必須使用 SSH 協定，您需要提供 SSH 金鑰，最簡單的方法是提供金鑰的路徑
ssh_key = "~/.ssh/id_ed25519" # ssh 金鑰的路徑
# maa 預設金鑰是未加密的，如果您的金鑰是受密碼保護的，您需要提供密碼來解密金鑰
# 注意：只有您使用 libgit2 後端時 maa 才會將密碼傳遞給 libgit2
# 當您使用 git 後端時，git 會自己提示您輸入密碼
# 如果您使用 git 後端且您的金鑰受密碼保護，請使用 ssh-agent 來管理您的金鑰
passphrase = "password"       # ssh 金鑰的密碼
# 然而在配置檔案中儲存明文密碼是不安全的，因此有一些方法可以避免這種情況
# 1. 將 `passphrase` 設定為 true，然後 maa-cli 將每次提示您輸入密碼
# 這種方法安全但是較為繁瑣且無法在 batch 模式下使用
# passphrase = true
# 2. 將 `passphrase` 設定為環境變數名稱，然後 maa-cli 將使用環境變數作為密碼
# 這種方法比明文密碼更安全，但是仍然有一定的風險，因為環境變數可能被任何程式存取
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. 將 `passphrase` 設定為指令，然後 maa-cli 將執行該指令以獲取密碼
# 如果您使用了密碼管理器來管理您的密碼，這種方法可能是最安全的且方便的
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
# 4. 使用 ssh-agent 來管理您的金鑰，**推薦**
# ssh-agent 會將您的金鑰保存在記憶體中，這樣您就不需要每次輸入密碼
# 注意，您需要確保 ssh-agent 已經啟動並且已經添加了您的金鑰，同時 SSH_AUTH_SOCK 環境變數已經設定
# use_ssh_agent = true # 使用 ssh-agent 進行身份驗證，如果設定為 true，將忽略 ssh_key 和 passphrase 欄位
```

**注意事項**：

- MaaCore 的更新通道中 `Alpha` 只在 Windows 上可用。
- 由於 CLI 預設的 API 連結和下載連結都是 GitHub 的連結，因此在連線較慢的地區可能會有一些問題，您可以透過配置 `api_url` 和 `download_url` 來使用鏡像站。
- 即使啟動了資源熱更新，您依然需要安裝 MaaCore 的資源，因為資源熱更新並不包含所有的資源檔案，只是包含部分可更新的資源檔案，基礎資源檔案仍然需要安裝。
- 資源熱更新是透過 Git 來拉取遠端倉庫，如果後端設定為 `git` 那麼 `git` 命令列工具必須可用。
- 如果您想要使用 SSH 協定來拉取遠端倉庫，您必須配置 `ssh_key` 欄位，這個欄位應該是一個路徑，指向您的 SSH 私鑰。
- 如果您的 SSH 私鑰是受密碼保護的，您需要提供密碼來解密私鑰，或者使用 ssh-agent 來管理您的金鑰。
- 遠端倉庫的 `url` 設定目前只對首次安裝資源有效，如果您想要更改遠端倉庫的位址，您需要透過 `git` 命令列工具手動更改，或者刪除對應的倉庫。倉庫所在位置可以透過 `maa dir hot-update` 獲取。

## 參考配置

- [範例配置][example-config]
- [自用配置][wangl-cc-dotfiles]

## JSON Schema

您可以在 [`schemas` 目錄][schema-dir] 中找到 maa-cli 的 JSON Schema 檔案，您可以使用這些檔案來驗證您的配置檔案，或者在編輯器中獲得自動補全。

- 自定義任務檔案的 JSON Schema 檔案為 [`task.schema.json`][task-schema]；
- MaaCore 配置的 JSON Schema 檔案為 [`asst.schema.json`][asst-schema]；
- CLI 配置的 JSON Schema 檔案為 [`cli.schema.json`][cli-schema]。

[task-types]: ../../protocol/integration.md#任務類型一覽
[emulator-ports]: ../../manual/connection.md#模擬器連接埠相關文件
[playcover-doc]: ../../manual/device/macos.md#✅-playcover-原生執行最流暢-🚀
[waydroid-doc]: ../../manual/device/linux.md#✅-waydroid
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/main/home/dot_config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
