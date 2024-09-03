---
order: 4
icon: material-symbols:task
---

# 任務流程協議

`resource/tasks.json` 的使用方法及各欄位說明

::: tip
請注意 JSON 檔是不支援註解的，下方的註解僅用於說明，請勿直接複製使用
:::

## 完整欄位一覽

```json
{
    "TaskName" : {                          // 任務名

        "baseTask": "xxx",                  // 以 xxx 任務為範本產生任務，詳細見下方特殊任務類型中的 Base Task

        "algorithm": "MatchTemplate",       // 可選項，表示辨識演算法的類型
                                            // 不填寫時預設為 MatchTemplate
                                            //      - JustReturn:       不進行辨識，直接執行 action
                                            //      - MatchTemplate:    匹配圖片
                                            //      - OcrDetect:        文字辨識
                                            //      - Hash:             哈希計算

        "action": "ClickSelf",              // 可選項，表示辨識到後的動作
                                            // 不填寫時預設為 DoNothing
                                            //      - ClickSelf:        點擊辨識到的位置（辨識到的目標範圍內隨機點）
                                            //      - ClickRand:        點擊整個畫面中隨機位置
                                            //      - ClickRect:        點擊指定的區域，對應 specificRect 欄位，不建議使用該選項
                                            //      - DoNothing:        什麼都不做
                                            //      - Stop:             停止目前任務
                                            //      - Swipe:            滑動，對應 specificRect 與 rectMove 欄位

        "sub": [ "SubTaskName1", "SubTaskName2" ],
                                            // 可選項，子任務。會在執行完目前任務後，依次執行每一個子任務
                                            // 可以套娃，子任務再套子任務。但要注意不要寫出了死迴圈

        "subErrorIgnored": true,            // 可選項，是否忽略子任務的錯誤。
                                            // 不填寫預設 false
                                            // 為 false 時，若某個子任務出錯，則不繼續執行後續任務（相當於本任務出錯了）
                                            // 為 true 時，子任務是否出錯沒有影響

        "next": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可選項，表示執行完目前任務和 sub 任務後，下一個要執行的任務
                                            // 會從前往後依次去辨識，去執行第一個匹配上的
                                            // 不填寫預設執行完目前任務直接停止
                                            // 對於相同任務，第一次辨識後第二次就不再辨識：
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // 不允許 JustReturn 型任務位於非最後一項

        "maxTimes": 10,                     // 可選項，表示該任務最大執行次數
                                            // 不填寫時預設無限大
                                            // 達到最大次數後，若存在 exceededNext 欄位，則執行 exceededNext，否則直接任務停止

        "exceededNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可選項，表示達到了最大執行次數後要執行的任務
                                            // 不填寫時，達到了最大執行次數則停止，填寫後就執行這裡的，而不是 next 裡的
        "onErrorNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可選項，表示執行出錯時，後續要執行的任務

        "preDelay": 1000,                   // 可選項，表示辨識到後延遲多久才執行 action，單位毫秒，不填寫時預設 0
        "postDelay": 1000,                  // 可選項，表示 action 執行完後延遲多久才去辨識 next，單位毫秒，不填寫時預設 0

        "roi": [ 0, 0, 1280, 720 ],         // 可選項，表示辨識的範圍，格式為 [ x, y, width, height ]
                                            // 以 1280 * 720 為基準自動縮放，不填寫時預設 [ 0, 0, 1280, 720 ]
                                            // 儘量填寫，減小辨識範圍可以減少性能消耗，加快辨識速度

        "cache": false,                      // 可選項，表示該任務是否使用快取，預設為 false
                                            // 第一次辨識到後，以後永遠只在第一次辨識到的位置進行辨識，開啟可大幅節省性能
                                            // 但僅適用於待辨識目標位置完全不會變的任務，若待辨識目標位置會變請設為 false

        "rectMove": [ 0, 0, 0, 0 ],         // 可選項，辨識後的目標移動，不建議使用該選項。以 1280 * 720 為基準自動縮放
                                            // 例如辨識到了 A ，但實際要點擊的是 A 下方 10 像素 5 * 2 區域的某個位置，
                                            // 則可填寫 [ 0, 10, 5, 2 ]，可以的話儘量直接辨識要點擊的位置，不建議使用該選項
                                            // 額外的，當 action 為 Swipe 時有效且必選，表示滑動終點。

        "reduceOtherTimes": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可選項，執行後減少其他任務的執行計數。
                                            // 例如執行了吃理智藥，則說明上一次點擊藍色開始行動按鈕沒生效，所以藍色開始行動要 -1

        "specificRect": [ 100, 100, 50, 50 ],
                                            // 當 action 為 ClickRect 時有效且必選，表示指定的點擊位置（範圍內隨機一點）。
                                            // 當 action 為 Swipe 時有效且必選，表示滑動起點。
                                            // 以 1280 * 720 為基準自動縮放

        "specialParams": [ int, ... ],      // 某些特殊辨識器需要的參數
                                            // 額外的，當 action 為 Swipe 時可選，[0] 表示 duration，[1] 表示 是否啟用額外滑動

        /* 以下欄位僅當 algorithm 為 MatchTemplate 時有效 */

        "template": "xxx.png",              // 可選項，要匹配的圖片檔案名稱
                                            // 預設 "任務名稱.png"

        "templThreshold": 0.8,              // 可選項，圖片範本匹配得分的閾值，超過閾值才認為辨識到了。
                                            // 預設 0.8，可根據日誌查看實際得分是多少

        "maskRange": [ 1, 255 ],            // 可選項，灰階遮罩範圍。
                                            // 例如將圖片不需要辨識的部分塗成黑色（灰階值為 0）
                                            // 然後設定 "maskRange" 的範圍為 [ 1, 255 ]，匹配的時候立刻忽略塗黑的部分

        "colorScales": [                    // 當 method 為 HSVCount 或 RGBCount 時有效且必選，數色掩碼範圍。
            [                               // list<array<array<int, 3>, 2> | array<int, 2>>
                [23, 150, 40],              // 結構為 [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     內層為 int 時是灰度，
            ],                              //     　　為 array<int, 3> 時是三通道顏色，method 決定其是 RGB 或 HSV；
            ...                             //     中間一層的 array<*, 2> 是顏色（或灰度）下限與上限：
        ],                                  //     最外層代表不同的顏色範圍，待識別區域為它們對應在模板圖片上掩碼的併集。

        "colorWithClose": true,             // 可選項，當 method 為 HSVCount 或 RGBCount 時有效，默認為 true
                                            // 數色時是否先用閉運算處理掩碼範圍。
                                            // 閉運算可以填補小黑點，一般會提高數色匹配效果，但若圖片中包含文字建議設為 false

        "method": "Ccoeff",                 // 可選項，模板匹配算法，可以是列表
                                            // 不填寫時默認為 Ccoeff
                                            //      - Ccoeff:       對顏色不敏感的模板匹配算法，對應 cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     對顏色敏感的模板匹配算法，
                                            //                      先將待匹配區域和模板圖片依據 maskRange 二值化，
                                            //                      以 F1-score 為指標計算 RGB 顏色空間內的相似度，
                                            //                      再將結果與 Ccoeff 的結果點積
                                            //      - HSVCount:     類似 RGBCount，顏色空間換為 HSV

        /* 以下欄位僅當 algorithm 為 OcrDetect 時有效 */

        "text": [ "接管作戰", "代理指揮" ],  // 必選項，要辨識的文字內容，只要任一匹配上了即認為辨識到了

        "ocrReplace": [                     // 可選項，針對常見辨識錯的文字進行替換（支援正則）
            [ "千員", "幹員" ],
            [ ".+擊幹員", "狙擊幹員" ]
        ],

        "fullMatch": false,                 // 可選項，文字辨識是否需要完全匹配（不能多字），預設為 false
                                            // false 時只要是子串即可：例如 text: [ "開始" ]，實際辨識到了 "開始行動"，也算辨識成功
                                            // true 時則必須辨識到了 "開始"，多一個字都不行

        "isAscii": false,                   // 可選項，要辨識的文字內容是否為 ASCII 碼字元
                                            // 不填寫預設 false

        "withoutDet": false                 // 可選項，是否不使用檢測模型
                                            // 不填寫預設 false

        /* 以下欄位僅當 algorithm 為 Hash 時有效 */
        // 演算法不成熟，僅部分特例情況中用到了，暫不推薦使用
        // Todo
    }
}
```

## 特殊任務類型

### Template Task（`@` 型任務）

Template task 與 base task 合稱**範本任務**。

允許把某個任務 "A" 當作範本，然後 "B@A" 表示由 "A" 產生的任務。

- 如果 `tasks.json` 中未顯式定義任務 "B@A"，則在 `sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes` 欄位中增加 `B@` 首碼（如遇任務名開頭為 `#` 則增加 `B` 首碼），其餘參數與 "A" 任務相同。就是說如果任務 "A" 有以下參數：

    ```json
    "A": {
        "template": "A.png",
        ...,
        "next": [ "N1", "N2" ]
    }
    ```

    就相當於同時定義了

    ```json
    "B@A": {
        "template": "A.png",
        ...,
        "next": [ "B@N1", "B@N2" ]
    }
    ```

- 如果 `tasks.json` 中定義了任務 "B@A"，則：
  1. 如果 "B@A" 與 "A" 的 `algorithm` 欄位不同，則派生類參數不繼承（只繼承 `TaskInfo` 定義的參數）
  2. 如果是圖片匹配任務，`template` 若未顯式定義則為 `B@A.png`（而不是繼承 "A" 的 `template` 名），其餘情況任何派生類參數若未顯式定義，直接繼承 "A" 任務的參數
  3. 對於 `TaskInfo` 基類中定義的參數（任何類型任務都有的參數，例如 `algorithm`, `roi`, `next` 等），若沒有在 "B@A" 內顯式定義，則除了上面提到的 `sub` 等五個欄位在繼承時會增加 "B@" 首碼外，其餘參數直接繼承 "A" 任務的參數

### Base Task

Base task 與 template task 合稱 **範本任務**。

有欄位 `baseTask` 的任務即為 base task。

Base task 的邏輯優先於 template task。這代表 `"B@A": { "baseTask": "C", ... }` 與任務 A 沒有任何相關。

任何參數若未顯式定義則不加首碼地直接使用 `baseTask` 對應任務的參數，除了 `template` 未顯式定義時仍為 `"任務名.png"`。

#### 多檔任務

如果後載入的任務檔案（例如外服 `tasks.json`，下稱文件二）中定義的任務在先載入的任務檔案（例如國服 `tasks.json`，下稱文件一）中也定義了同名任務，那麼：

- 如果文件二中任務沒有 `baseTask` 欄位，則直接繼承文件一中同名任務的欄位。
- 如果文件二中任務有 `baseTask` 欄位，則不繼承文件一中同名任務的欄位，而是直接取代。

### Virtual Task（虛任務）

Virtual task 也稱 sharp task（`#` 型任務）。

任務名帶 `#` 的任務即為 virtual task。 `#` 後可接 `next`, `back`, `self`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`。

| 虛任務類型 | 含義 | 簡單範例 |
|:---------:|:---:|:--------:|
| self | 父任務名 | `"A": {"next": "#self"}` 中的 `"#self"` 被解釋為 `"A"`<br>`"B": {"next": "A@B@C#self"}` 中的 `"A@B@C#self"` 被解釋為 `"B"`。<sup>1</sup> |
| back | # 前面的任務名 | `"A@B#back"` 被解釋為 `"A@B"`<br>`"#back"` 直接出現則會被跳過 |
| next, sub 等 | # 前任務名對應欄位 | 以 `next` 為例：<br>`"A#next"` 被解釋為 `Task.get("A")->next`<br>`"#next"` 直接出現則會被跳過 |

_Note<sup>1</sup>: `"XXX#self"` 與 `"#self"` 含義相同。_

#### 簡單範例

```json
{
    "A": { "next": [ "N1", "N2" ] },
    "C": { "next": [ "B@A#next" ] },

    "Loading": {
        "next": [ "#self", "#next", "#back" ]
    },
    "B": {
        "next": [ "Other", "B@Loading" ]
    }
}
```

可以得到：

```cpp
Task.get("C")->next = { "B@N1", "B@N2" };

Task.get("B@Loading")->next = { "B@Loading", "Other", "B" };
Task.get("Loading")->next = { "Loading" };
Task.get_raw("B@Loading")->next = { "B#self", "B#next", "B#back" };
```

#### 一些用途

- 當幾個任務有 `"next": [ "#back" ]` 時，`"Task1@Task2@Task3"` 代表依次執行 `Task3`, `Task2`, `Task1`。

#### 其它相關

```json
{
    "A": { "next": [ "N0" ] },
    "B": { "next": [ "A#next" ] },
    "C@A": { "next": [ "N1" ] }
}
```

以上這種情況，`"C@B" -> next`（即 `C@A#next`）為 `[ "N1" ]` 而不是 `[ "C@N0" ]`.

## 運行時修改任務

- `Task.lazy_parse()` 可以在執行時載入 json 任務設定檔。 lazy_parse 規則與[多檔任務](#多檔任務)相同。
- `Task.set_task_base()` 可以修改任務的 `baseTask` 欄位。

### 使用範例

假設有任務設定檔如下：

```json
{
     "A": {
         "baseTask": "A_default"
     },
     "A_default": {
         "next": [ "xxx" ]
     },
     "A_mode1": {
         "next": [ "yyy" ]
     },
     "A_mode2": {
         "next": [ "zzz" ]
     }
}
   ```

以下程式碼可以實現根據 mode 的值改變任務 "A"，同時會改變其它依賴任務 "A" 的任務，如 "B@A":

```cpp
switch (mode) {
case 1:
     Task.set_task_base("A", "A_mode1"); // 基本上相當於用 A_mode1 的內容直接取代 A，下同
     break;
case 2:
     Task.set_task_base("A", "A_mode2");
     break;
default:
     Task.set_task_base("A", "A_default");
     break;
}
```

## 運算式計算

| 符號 | 含義 | 實例 |
|:---------:|:---:|:--------:|
| `@` | 範本任務 | `Fight@ReturnTo` |
| `#`（單目） | 虛任務 | `#self` |
| `#`（雙目） | 虛任務 | `StartUpThemes#next` |
| `*` | 重複多個任務 | `(ClickCornerAfterPRTS+ClickCorner)*5` |
| `+` | 任務清單合併（在 next 系列屬性中同名任務只保留最靠前者） | `A+B` |
| `^` | 任務列表差（在前者但不在後者，順序不變）| `(A+A+B+C)^(A+B+D)`（結果為 `C`） |

運算子 `@`, `#`, `*`, `+`, `^` 有優先順序：`#`（單目）> `@` = `#`（雙目）> `*` > `+` = `^`。

## Schema 檢驗

本項目為 `tasks.json` 配置了 json schema 檢驗，schema 文件為`docs/maa_tasks_schema.json`。

### Visual Studio

在 `MaaCore.vcxporj` 中已對其進行配置，開箱即用。提示效果較為晦澀，且有部分資訊缺失。

### Visual Studio Code

在 `.vscode/settings.json` 中已對其進行配置，用 vscode 打開該 **專案檔案夾** 即可使用。提示效果較好。
