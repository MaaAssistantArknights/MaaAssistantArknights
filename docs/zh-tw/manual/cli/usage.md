---
order: 2
icon: material-symbols:summarize
---

# 使用

maa-cli 的主要功能是透過呼叫 MaaCore，自動化完成《明日方舟》的遊戲任務。此外，為了方便使用，maa-cli 還提供了管理 MaaCore 的功能。

::: tip

對於使用 Windows 套件管理員 (winget) 安裝 maa-cli 的使用者，以下指令中的 `maa` 需要替換為 `maa-cli`。

:::

## 管理 MaaCore

maa-cli 可以安裝和更新 MaaCore 及資源，只需執行以下指令：

```bash
maa install # 安裝 MaaCore 及資源
maa update # 更新 MaaCore 及資源
```

## 更新 maa-cli 自身

maa-cli 可以更新自身，只需執行以下指令：

```bash
maa self update
```

::: warning
使用套件管理員安裝 maa-cli 的使用者請使用套件管理員更新 maa-cli，此指令對這些使用者無效。
:::

## 初始化配置

一旦完成了 MaaCore 的安裝，通常情況下，您無需額外設定就可以直接執行任務。預設配置可能不適用於所有使用者，因此您可以透過以下指令來進行初始化：

```bash
maa init
```

透過這個指令，您可以互動式地進行 [MaaCore 的相關配置][config-core]。

## 執行任務

完成 MaaCore 的安裝與配置後，您就可以執行任務了。maa-cli 支援兩種類型的任務：預定義任務和自定義任務。

### 預定義任務

對於常見任務，maa-cli 提供了一些預定義的任務：

::: collapse

- :+ 啟動遊戲並進入主介面

  ```shell
  maa startup [client]
  ```
  
  `[client]` 是客戶端類型，如果留空則不會啟動遊戲客戶端。

- 關閉遊戲客戶端
  
  ```shell
  maa closedown [client]
  ```
  `[client]` 是客戶端類型，預設為 `Official`。

- 執行戰鬥任務

  ```shell
  maa fight [stage]
  ```

  `[stage]` 是關卡名稱，例如 `1-7`；留空則選擇上次或當前關卡。

- 自動抄作業
  ```shell
  maa copilot <maa_uri>
  ```
  `<maa_uri>` 是作業的 URI，多個 URI 會依序執行，`maa_uri` 可以是 `maa://1234` 或本地檔案路徑 `./1234.json`。

- 自動保全派駐
  ```shell
  maa sscopilot <maa_uri>
  ```

  `<maa_uri>` 是保全派駐作業的 URI。

- 自動集成戰略
  ```shell
  maa roguelike <theme>
  ```

  `<theme>` 是集成戰略的主題，可選值為 `Phantom`，`Mizuki`，`Sami`，`Sarkaz` 以及 `JieGarden`。

- 自動生息演算
  ```shell
  maa reclamation <theme>
  ```

  `<theme>` 是生息演算的主題，目前僅 `Tales` 主題可用。

:::

上述任務接受一些參數，您可以透過 `maa <task> --help` 查看具體的參數。

對於官服玩家，如果您想要開啟遊戲，使用 3 個理智藥刷 BB-7，然後關閉遊戲，您可以執行以下指令：

```bash
maa startup Official && maa fight BB-7 -m 3 && maa closedown
```

### 自定義任務

由於 MAA 支援的任務繁多，maa-cli 無法提供所有任務的預定義選項。除此之外，您可能需要像上述的例子一樣執行多個任務。為了解決這個問題，maa-cli 提供了自定義任務的功能。自定義任務能夠組合不同的任務，並且更精確地控制每個任務的參數以及執行順序。此外，自定義任務支援條件判斷，可以根據條件來決定是否執行某個任務，或者以何種參數執行某個任務。這可以用於自動化您的日常任務。自定義任務透過配置檔案定義，具體配置檔案的位置和編寫方式請參考 [自定義任務文件][custom-task]。在編寫好配置檔案後，您可以透過 `maa run <task>` 來執行自定義任務，這裡的 `<task>` 是一個自定義任務檔案名稱，不包括副檔名。

### 任務總結

不管是預定義任務還是自定義任務，maa-cli 都會在任務執行結束後輸出任務的總結資訊，
其包括每個子任務的執行時間（開始時間、結束時間、執行時長）。對於部分任務，還會輸出任務的結果彙整：

- `fight` 任務：關卡名稱、次數、消耗理智藥個數以及掉落統計。
- `infrast`：各設施進駐的幹員，對於製造站和貿易站，還會包括產物類型。
- `recruit`：每次公招的 tag、星級以及狀態，以及總共的招募次數。
- `roguelike`：探索次數、投資次數。

如果您不想要任務總結，可以透過 `--no-summary` 參數來關閉。

### 任務日誌

maa-cli 會輸出日誌，日誌輸出層級從低到高分別為 `Error`，`Warn`，`Info`，`Debug` 和 `Trace`。預設的日誌輸出層級為 `Warn`。日誌層級可以透過 `MAA_LOG` 環境變數來設定，例如 `MAA_LOG=debug`。您也可以透過 `-v` 或 `-q` 來增加或減少日誌輸出層級。

maa-cli 預設會向標準錯誤輸出 (stderr) 輸出日誌。`--log-file` 選項可以將日誌輸出到檔案中，日誌儲存在 `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log` 中，其中 `$(maa dir log)` 是日誌目錄，您可以透過 `maa dir log` 獲取。您也可以透過 `--log-file=path/to/log` 來指定日誌檔案的路徑。

預設情況下，所有輸出的日誌會包含時間戳記和日誌層級的前綴。您可以透過環境變數 `MAA_LOG_PREFIX` 來改變這個行為。設定為 `Always` 時，總是會包含前綴；設定為 `Auto` 時，輸出到日誌檔案時會包含前綴，而輸出到 stderr 時則不會包含前綴；設定為 `Never` 時，即使是寫入日誌檔案時也不會包含前綴。

### 其他子指令

除了上述的指令外，maa-cli 還提供了其他一些子指令：

::: collapse

- 列出所有可用的任務

  ```shell
  maa list
  ```

- 獲取特定目錄的路徑
  ```shell
  maa dir <dir>
  ```
  例如 `maa dir config` 可以用來獲取配置目錄的路徑。

- 獲取版本資訊

  ```shell
  maa version
  ```

- 轉換檔案格式

  ```shell
  maa convert <input> [output]
  ```

- 產生自動補全指令碼

  ```shell
  maa complete <shell>
  ```

- 獲取目前活動資訊

  ```shell
  maa activity [client]
  ```
  `client` 是客戶端類型，預設為 `Official`。

- 清除快取
  
  ```shell
  maa cleanup
  ```

- 匯入配置檔案

  ```shell
  maa import <file> [-t <type>]
  ```
  `file` 是配置檔案的路徑。`-t` 選項可以指定配置檔案的類型，如 `cli`, `profile`, `infrast` 等。

:::

更多指令的使用方法可以透過 `maa help` 查看，具體指令的使用方法可以透過 `maa help <command>` 查看。

[config-core]: config.md#maacore-相關配置
[custom-task]: config.md#自定義任務
