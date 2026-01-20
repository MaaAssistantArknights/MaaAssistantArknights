---
order: 4
icon: material-symbols:task
---

# 任務流程協定

`resource/tasks` 的使用方法及各欄位說明

::: tip
推薦使用 [Visual Studio Code](https://code.visualstudio.com/) 並安裝 [Maa Pipeline Support](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support) 擴充功能以實作高效編輯，詳情請查閱擴充功能首頁和 [文件](../develop/vsc-ext-tutorial.md)。
:::

::: warning
請注意，json 檔案不支援註解。文件中的註解僅供說明參考，請勿直接複製使用。
:::

## 完整欄位一覽

```json
{
    "TaskName": {                           // 任務名稱，帶 @ 時可能為特殊任務，欄位預設值會有不同，詳見下方特殊任務類型

        "baseTask": "xxx",                  // 以 xxx 任務為範本產生任務，詳細見下方特殊任務類型中的 Base Task

        "algorithm": "MatchTemplate",       // 選填，表示辨識演算法的類型
                                            // 不填寫時預設為 MatchTemplate
                                            //      - JustReturn:       不進行辨識，直接執行 action
                                            //      - MatchTemplate:    比對圖片
                                            //      - OcrDetect:        文字辨識
                                            //      - FeatureMatch:     特徵比對

        "action": "ClickSelf",              // 選填，表示辨識到後的動作
                                            // 不填寫時預設為 DoNothing
                                            //      - ClickSelf:        點擊辨識到的位置（辨識到的目標範圍內隨機點）
                                            //      - ClickRect:        點擊指定的區域，對應 specificRect 欄位，不建議使用該選項
                                            //      - DoNothing:        什麼都不做
                                            //      - Stop:             停止目前任務
                                            //      - Swipe:            滑動，對應 specificRect 與 rectMove 欄位
                                            //      - Input:            輸入文字，要求 algorithm 為 JustReturn，對應 inputText 欄位

        "sub": ["SubTaskName1", "SubTaskName2"],
                                            // 選填，子任務，不推薦使用。會在執行完目前任務後，依序執行每一個子任務
                                            // 可以套娃，子任務再套子任務。但要注意不要寫出死循環

        "subErrorIgnored": true,            // 選填，是否忽略子任務的錯誤。
                                            // 不填寫預設為 false
                                            // 為 false 時，若某個子任務出錯，則不繼續執行後續任務（相當於本任務出錯了）
                                            // 為 true 時，子任務是否出錯沒有影響

        "next": ["OtherTaskName1", "OtherTaskName2"],
                                            // 選填，表示執行完目前任務和 sub 任務後，下一個要執行的任務
                                            // 會從前往後依序辨識，執行第一個比對成功的
                                            // 不填寫預設執行完目前任務直接停止
                                            // 對於相同任務，第一次辨識後第二次就不再辨識：
                                            //      "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // 不允許 JustReturn 型任務位於非最後一項

        "maxTimes": 10,                     // 選填，表示該任務最大執行次數
                                            // 不填寫時預設為無窮大
                                            // 達到最大次數後，若存在 exceededNext 欄位，則執行 exceededNext；否則直接任務停止

        "exceededNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 選填，表示達到了最大執行次數後要執行的任務
                                            // 不填寫時，達到了最大執行次數則停止；填寫後就執行這裡的，而不是 next 裡的
        "onErrorNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 選填，表示執行出錯時，後續要執行的任務

        "preDelay": 1000,                   // 選填，表示辨識到後延遲多久才執行 action，單位毫秒；不填寫時預設為 0
        "postDelay": 1000,                  // 選填，表示 action 執行完後延遲多久才去辨識 next, 單位毫秒；不填寫時預設為 0

        "roi": [0, 0, 1280, 720],           // 選填，表示辨識的範圍，格式為 [ x, y, width, height ]
                                            // 以 1280 * 720 為基準自動縮放；不填寫時預設為 [ 0, 0, 1280, 720 ]
                                            // 儘量填寫，減小辨識範圍可以減少效能消耗，加快辨識速度

        "cache": true,                      // 選填，表示該任務是否使用快取，預設為 true;
                                            // 第一次辨識到後，以後永遠只在第一次辨識到的位置進行辨識，開啟可大幅節省效能
                                            // 但僅適用於待辨識目標位置完全不會變的任務，若待辨識目標位置會變請設為 false

        "rectMove": [0, 0, 0, 0],           // 選填，辨識後的目標移動，不建議使用該選項。以 1280 * 720 為基準自動縮放
                                            // 例如辨識到了 A ，但實際要點擊的是 A 下方 10 像素 5 * 2 區域的某個位置，
                                            // 則可填寫 [ 0, 10, 5, 2 ]，可以的話儘量直接辨識要點擊的位置，不建議使用該選項
                                            // 額外的，當 action 為 Swipe 時有效且必填，表示滑動終點。

        "reduceOtherTimes": ["OtherTaskName1", "OtherTaskName2"],
                                            // 選填，執行後減少其他任務的執行計數。
                                            // 例如執行了使用理智藥，則說明上一次點擊藍色開始行動按鈕沒生效，所以藍色開始行動要 -1

        "specificRect": [100, 100, 50, 50],
                                            // 當 action 為 ClickRect 時有效且必填，表示指定的點擊位置（範圍內隨機一點）。
                                            // 當 action 為 Swipe 時有效且必填，表示滑動起點。
                                            // 以 1280 * 720 為基準自動縮放
                                            // 當 algorithm 為 OcrDetect 時，specificRect[0] 和 specificRect[1] 表示灰度上下限門檻值。

        "specialParams": [int, ...],        // 某些特殊辨識器需要的參數
                                            // 額外的，當 action 為 Swipe 時選填，[0] 表示 duration，[1] 表示是否啟用額外滑動

        "highResolutionSwipeFix": false,    // 選填，是否啟用高解析度滑動修復
                                            // 現階段應該只有關卡導航未使用 unity 滑動方式所以需要開啟
                                            // 預設為 false

        /* 以下欄位僅當 algorithm 為 MatchTemplate 時有效 */

        "template": "xxx.png",              // 選填，要比對的圖片檔案名稱，可以是字串或字串列表
                                            // 預設為 "任務名稱.png"
                                            // 範本圖檔案可放在 template 及其子資料夾下，載入時會進行遞迴搜尋

        "templThreshold": 0.8,              // 選填，圖片範本比對得分的門檻值，超過門檻值才認為辨識到了，可以是數字或數字列表
                                            // 預設 0.8, 可根據記錄檔查看實際得分是多少

        "maskRange": [1, 255],              // 選填，比對時的灰階遮罩範圍。 array<int, 2>
                                            // 例如將圖片不需要辨識的部分塗成黑色（灰階值為 0）
                                            // 然後設定為 [ 1, 255 ], 比對的時候即忽略塗黑的部分

        "colorScales": [                    // 當 method 為 HSVCount 或 RGBCount 時有效且必填，數色遮罩範圍。
            [                               // list<array<array<int, 3>, 2>> / list<array<int, 2>>
                [23, 150, 40],              // 結構為 [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              // 內層為 int 時是灰階，
            ],                              // 為 array<int, 3> 時是三通道顏色，method 決定其是 RGB 或 HSV；
            ...                             // 中間一層的 array<*, 2> 是顏色（或灰階）下限與上限：
        ],                                  // 最外層代表不同的顏色範圍，待辨識區域為它們對應在範本圖片上遮罩的聯集。

        "colorWithClose": true,             // 選填，當 method 為 HSVCount 或 RGBCount 時有效，預設為 true
                                            // 數色時是否先用閉運算處理遮罩範圍。
                                            // 閉運算可以填補小黑點，一般會提高數色比對效果，但若圖片中包含文字建議設為 false

        "pureColor": false,                 // 選填，當 method 為 HSVCount 或 RGBCount 時有效，預設為 false
                                            // 如果為 true，則忽略範本比對得分，完全依賴顏色比對結果
                                            // 適用於顏色特徵明顯但範本比對效果不佳的場景
                                            // 使用此選項時建議相應提高 templThreshold 門檻值

        "method": "Ccoeff",                 // 選填，範本比對演算法，可以是列表
                                            // 不填寫時預設為 Ccoeff
                                            //      - Ccoeff:       對顏色不敏感的範本比對演算法，對應 cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     對顏色敏感的範本比對演算法，
                                            //                      先將待比對區域和範本圖片依據 colorScales 二值化，
                                            //                      以 F1-score 為指標計算 RGB 顏色空間內的相似度，
                                            //                      再將結果與 Ccoeff 的結果內積
                                            //      - HSVCount:     類似 RGBCount，顏色空間換為 HSV

        /* 以下欄位僅當 algorithm 為 OcrDetect 時有效 */

        "text": [ "接管作戰", "代理指揮" ],  // 必填項，要辨識的文字內容，只要任一比對成功即認為辨識到了

        "ocrReplace": [                     // 選填，針對常見辨識錯誤的文字進行替換（支援正規表示式）
            [ "千員", "幹員" ],
            [ ".+擊幹員", "狙擊幹員" ]
        ],

        "fullMatch": false,                 // 選填，文字辨識是否需要全字比對（不能多字），預設為 false
                                            // false 時只要是子字串即可：例如 text: [ "開始" ]，實際辨識到了 "開始行動"，也算辨識成功；
                                            // true 時則必須辨識到了 "開始"，多一個字都不行

        "isAscii": false,                   // 選填，要辨識的文字內容是否為 ASCII 碼字元
                                            // 不填寫預設為 false

        "withoutDet": false,                // 選填，是否不使用檢測模型
                                            // 不填寫預設為 false

        /* 以下欄位僅當 algorithm 為 OcrDetect 且 withoutDet 為 true 時有效 */

        "useRaw": true,                     // 選填，是否使用原圖比對
                                            // 不填寫預設為 true，false 時為灰階比對

        "binThreshold": [140, 255],         // 選填，二值化灰階門檻值（預設 [140, 255]）
                                            // 灰階值不在範圍的像素將被視為背景，排除在文字區域之外
                                            // 最終保留 [lower, upper] 區間的像素作為文字前景

        /* 以下欄位僅當 algorithm 為 JustReturn，action 為 Input 時有效 */

        "inputText": "A string text.",      // 必填項，要輸入的文字內容，以字串的形式

        /* 以下欄位僅當 algorithm 為 FeatureMatch 時有效 */

        "template": "xxx.png",              // 選填，要比對的圖片檔案名稱，可以是字串或字串列表
                                            // 預設為 "任務名稱.png"

        "count": 4,                         // 比對的特徵點的數量要求 (門檻值), 預設值 = 4

        "ratio": 0.6,                       // KNN 比對演算法的距離比值, [0 - 1.0], 越大則比對越寬鬆, 更容易連線. 預設 0.6

        "detector": "SIFT",                 // 特徵點檢測器類型, 可選值為 SIFT, ORB, BRISK, KAZE, AKAZE, SURF; 預設值 = SIFT
                                            // SIFT: 計算複雜度高，具有尺度不變性、旋轉不變性。效果最好。
                                            // ORB: 計算速度非常快，具有旋轉不變性。但不具有尺度不變性。
                                            // BRISK: 計算速度非常快，具有尺度不變性、旋轉不變性。
                                            // KAZE: 適用於 2D 和 3D 圖像，具有尺度不變性、旋轉不變性。
                                            // AKAZE: 計算速度較快，具有尺度不變性、旋轉不變性。
    }
}
```

## 運算式計算

任務列表類型欄位（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）支援運算式計算。

|    符號     |                           含義                           |                  實例                   |
| :---------: | :------------------------------------------------------: | :-------------------------------------: |
|     `@`     |                        `@` 型任務                        |            `Fight@ReturnTo`             |
| `#`（一元） |                          虛任務                          |                 `#self`                 |
| `#`（二元） |                          虛任務                          |          `StartUpThemes#next`           |
|     `*`     |                       重複多個任務                       | `(ClickCornerAfterPRTS+ClickCorner)*10` |
|     `+`     | 任務列表合併（在 next 系列欄位中同名任務只保留最靠前者） |                  `A+B`                  |
|     `^`     |         任務列表差（在前者但不在後者，順序不變）         |    `(A+A+B+C)^(A+B+D)`（結果為 `C`）    |

運算子 `@`, `#`, `*`, `+`, `^` 有優先順序：`#`（一元）> `@` = `#`（二元）> `*` > `+` = `^`。

## 特殊任務類型

### 範本任務

**範本任務**包括衍生任務與 `@` 型任務。範本任務的核心可以理解為根據父任務**修改欄位的預設值**。

#### 衍生任務

存在欄位 `baseTask` 的任務即為衍生任務，欄位 `baseTask` 對應的任務稱為這個衍生任務的**父任務**。對於一個衍生任務：

1. 若是範本比對任務，則**欄位 `template`** 的預設值仍為 `"任務名稱.png"`；
2. 若欄位 `algorithm` 與父任務不同，則衍生類別參數不繼承（只繼承 `TaskInfo` 定義的參數）；
3. 其餘欄位的預設值均為**父任務對應欄位**。

#### 隱式 `@` 型任務

存在任務 `"A"` 且所有任務檔案中均未直接定義的型如 `"B@A"` 的任務即為隱式 `@` 型任務，任務 `"A"` 稱為任務 `"B@A"` 的**父任務**。對於一個隱式 `@` 型任務，

1. 其任務列表類型欄位（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）的預設值為父任務對應欄位**直接**增加 `B@` 前綴（如遇任務名稱開頭為 `#`則增加 `B` 前綴）；
2. 其餘欄位的預設值均為**父任務對應欄位**（包括欄位 `template`）。

#### 顯式 `@` 型任務

存在任務 `"A"` 且存在任務檔案中直接定義的型如 `"B@A"` 的任務即為顯式 `@` 型任務，任務 `"A"` 稱為任務 `"B@A"` 的**父任務**。對於一個顯式 `@` 型任務，

1. 任務列表類型欄位（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）的預設值為父任務對應欄位**直接**增加 `B@` 前綴（如遇任務名稱開頭為 `#`則增加 `B` 前綴）；
2. 若是範本比對任務，則**欄位 `template`** 的預設值仍為 `"任務名稱.png"`；
3. 若欄位 `algorithm` 與父任務不同，則衍生類別參數不繼承（只繼承 `TaskInfo` 定義的參數）；
4. 其餘欄位的預設值均為**父任務對應欄位**。

### 虛任務（`#` 型任務）

虛任務即型如 `"#{sharp_type}"` 或 `"B#{sharp_type}"` 的任務，其中 `{sharp_type}` 可以是 `none`, `self`, `back`, `next`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`。

可以將虛任務分為**指令虛任務**（`#none` / `#self` / `#back`）和**欄位虛任務**（`#next` 等）。

|  虛任務類型  |        含義        |                                                                  簡單範例                                                                  |
| :----------: | :----------------: | :----------------------------------------------------------------------------------------------------------------------------------------: |
|     none     |       空任務       |        直接跳過<sup>1</sup><br>`"A": {"next": ["#none", "T1"]}` 被視為 `"A": {"next": ["T1"]}`<br>`"A#none + T1"` 被視為 `"T1"`        |
|     self     |     目前任務名稱     | `"A": {"next": ["#self"]}` 中的 `"#self"` 被視為 `"A"`<br>`"B": {"next": ["A@B@C#self"]}` 中的 `"A@B@C#self"` 被視為 `"B"`<sup>2</sup> |
|     back     |   # 前面的任務名稱   |                                 `"A@B#back"` 被視為 `"A@B"`<br>`"#back"` 直接出現則會被跳過<sup>3</sup>                                  |
| next, sub 等 | # 前任務名稱對應欄位 |                       以 `next` 為例：<br>`"A#next"` 被視為 `Task.get("A")->next`<br>`"#next"` 直接出現則會被跳過                        |

_Note<sup>1</sup>: `"#none"` 一般配合範本任務增加前綴的特性使用，或用在欄位 `baseTask` 中避免多檔案繼承不必要的欄位。_

_Note<sup>2</sup>: `"XXX#self"` 與 `"#self"` 含義相同。_

_Note<sup>3</sup>: 當幾個任務有 `"next": [ "#back" ]` 時，`"T1@T2@T3"` 代表依序執行 `T3`, `T2`, `T1`。_

### 多檔案任務

如果後載入的任務檔案（例如外服 `tasks.json`；下稱檔案二）中定義的任務，在先載入的任務檔案（例如國服 `tasks.json`；下稱檔案一）中也定義了同名任務，那麼：

- 如果檔案二中任務沒有 `baseTask` 欄位，則直接繼承檔案一中同名任務的欄位。
- 如果檔案二中任務有 `baseTask` 欄位，則不繼承檔案一中同名任務的欄位，而是直接覆蓋。特別地，在沒有範本任務時，你可以使用 `"baseTask": "#none"` 來避免繼承不必要的欄位。

### 使用範例

- 衍生任務範例（欄位 `baseTask`）

  假設定義了如下兩個任務：

  ```json
  "Return": {
      "action": "ClickSelf",
      "next": [ "Stop" ]
  },
  "Return2": {
      "baseTask": "Return"
  },
  ```

  則 `"Return2"` 任務的參數會直接從 `"Return"` 任務繼承，實際上包含以下參數：

  ```json
  "Return2": {
    "algorithm": "MatchTemplate", // 直接繼承
    "template": "Return2.png",    // "任務名稱.png"
    "action": "ClickSelf",        // 直接繼承
    "next": [ "Stop" ]            // 直接繼承，與範本任務相比這裡沒有前綴
  }
  ```

- `@` 型任務範例

  假設定義了包含以下參數的任務 `"A"`：

  ```json
  "A": {
      "template": "A.png",
      ...,
      "next": [ "N1", "#back" ]
  },
  ```

  如果任務 `"B@A"` 沒有被直接定義，那麼任務 `"B@A"` 實際上有以下參數：

  ```json
  "B@A": {
      "template": "A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

  如果任務 `"B@A"` 有定義 `"B@A": {}`，那麼任務 `"B@A"` 實際上有以下參數：

  ```json
  "B@A": {
      "template": "B@A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

- 虛任務範例

  ```json
  {
      "A": { "next": ["N1", "N2"] },
      "C": { "next": ["B@A#next"] },

      "Loading": {
          "next": ["#self", "#next", "#back"]
      },
      "B": {
          "next": ["Other", "B@Loading"]
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

### 注意事項

如果任務列表類型欄位（`next` 等）中定義的任務包含低優先權運算，則實際結果可能不符預期，需要特別注意。

1. `@` 與二元 `#` 的運算順序導致的特例

   ```json
   {
       "A": { "next": ["N0"] },
       "B": { "next": ["A#next"] },
       "C@A": { "next": ["N1"] }
   }
   ```

   以上這種情況， `"C@B" -> next`（即 `C@A#next`）為 `[ "N1" ]` 而不是 `[ "C@N0" ]`。

2. `@` 與 `+` 的運算順序導致的特例

   ```json
   {
       "A": { "next": ["#back + N0"] },
       "B@A": {}
   }
   ```

   以上這種情況：

   ```cpp
   Task.get("A")->next = { "N0" };

   Task.get_raw("B@A")->next = { "B#back + N0" };
   Task.get("B@A")->next = { "B", "N0" }; // 注意不是 [ "B", "B@N0" ]
   ```

   事實上，你可以利用這個特性來避免增加不必要的前綴，只需要定義：

   ```json
   {
       "A": { "next": ["#none + N0"] }
   }
   ```

## 執行中更改任務

- `Task.lazy_parse()` 可以在執行中載入 json 任務配置檔案。`lazy_parse` 規則與[多檔案任務](#多檔案任務)相同。
- `Task.set_task_base()` 則可以修改任務的 `baseTask` 欄位。

### 使用範例

假設有任務配置檔案如下：

```json
{
    "A": {
        "baseTask": "A_default"
    },
    "A_default": {
        "next": ["xxx"]
    },
    "A_mode1": {
        "next": ["yyy"]
    },
    "A_mode2": {
        "next": ["zzz"]
    }
}
```

以下程式碼可以實作根據 mode 的值改變任務 "A"，同時會改變其他依賴任務 "A" 的任務（例如 "B@A"）： 

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // 基本上相當於用 A_mode1 的內容直接替換 A，下同
    break;
case 2:
    Task.set_task_base("A", "A_mode2");
    break;
default:
    Task.set_task_base("A", "A_default");
    break;
}
```

## Schema 檢驗

本專案為 `tasks.json` 配置了 json Schema 檢驗，Schema 檔案為 `docs/maa_tasks_schema.json`。

### Visual Studio

在 `MaaCore.vcxporj` 中已對其進行設定，內建功能開箱即用。提示效果較為晦澀，且有部分資訊缺失。

### Visual Studio Code

在 `.vscode/settings.json` 中已對其進行設定，使用 Visual Studio Code 開啟該**專案資料夾**即可使用。提示效果較好。