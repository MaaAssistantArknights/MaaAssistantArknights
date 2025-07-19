---
order: 4
icon: material-symbols:task
---

# 任务流程协议

`resource/tasks` 的使用方法及各字段说明

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

## 完整字段一览

```json
{
    "TaskName": {                           // 任务名，带 @ 时可能为特殊任务，字段默认值会有不同，详见下方特殊任务类型

        "baseTask": "xxx",                  // 以 xxx 任务为模板生成任务，详细见下方特殊任务类型中的 Base Task

        "algorithm": "MatchTemplate",       // 可选项，表示识别算法的类型
                                            // 不填写时默认为 MatchTemplate
                                            //      - JustReturn:       不进行识别，直接执行 action
                                            //      - MatchTemplate:    匹配图片
                                            //      - OcrDetect:        文字识别
                                            //      - FeatureMatch:     特征匹配

        "action": "ClickSelf",              // 可选项，表示识别到后的动作
                                            // 不填写时默认为 DoNothing
                                            //      - ClickSelf:        点击识别到的位置（识别到的目标范围内随机点）
                                            //      - ClickRect:        点击指定的区域，对应 specificRect 字段，不建议使用该选项
                                            //      - DoNothing:        什么都不做
                                            //      - Stop:             停止当前任务
                                            //      - Swipe:            滑动，对应 specificRect 与 rectMove 字段
                                            //      - Input:            输入文本，要求 algorithm 为 JustReturn，对应 inputText 字段

        "sub": ["SubTaskName1", "SubTaskName2"],
                                            // 可选项，子任务，不推荐使用。会在执行完当前任务后，依次执行每一个子任务
                                            // 可以套娃，子任务再套子任务。但要注意不要写出了死循环

        "subErrorIgnored": true,            // 可选项，是否忽略子任务的错误。
                                            // 不填写默认 false
                                            // 为 false 时，若某个子任务出错，则不继续执行后续任务（相当于本任务出错了）
                                            // 为 true 时，子任务是否出错没有影响

        "next": ["OtherTaskName1", "OtherTaskName2"],
                                            // 可选项，表示执行完当前任务和 sub 任务后，下一个要执行的任务
                                            // 会从前往后依次去识别，去执行第一个匹配上的
                                            // 不填写默认执行完当前任务直接停止
                                            // 对于相同任务，第一次识别后第二次就不再识别：
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // 不允许 JustReturn 型任务位于非最后一项

        "maxTimes": 10,                     // 可选项，表示该任务最大执行次数
                                            // 不填写时默认无穷大
                                            // 达到最大次数后，若存在 exceededNext 字段，则执行 exceededNext；否则直接任务停止

        "exceededNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 可选项，表示达到了最大执行次数后要执行的任务
                                            // 不填写时，达到了最大执行次数则停止；填写后就执行这里的，而不是 next 里的
        "onErrorNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 可选项，表示执行出错时，后续要执行的任务

        "preDelay": 1000,                   // 可选项，表示识别到后延迟多久才执行 action，单位毫秒；不填写时默认 0
        "postDelay": 1000,                  // 可选项，表示 action 执行完后延迟多久才去识别 next, 单位毫秒；不填写时默认 0

        "roi": [0, 0, 1280, 720],           // 可选项，表示识别的范围，格式为 [ x, y, width, height ]
                                            // 以 1280 * 720 为基准自动缩放；不填写时默认 [ 0, 0, 1280, 720 ]
                                            // 尽量填写，减小识别范围可以减少性能消耗，加快识别速度

        "cache": false,                     // 可选项，表示该任务是否使用缓存，默认为 false;
                                            // 第一次识别到后，以后永远只在第一次识别到的位置进行识别，开启可大幅节省性能
                                            // 但仅适用于待识别目标位置完全不会变的任务，若待识别目标位置会变请设为 false

        "rectMove": [0, 0, 0, 0],           // 可选项，识别后的目标移动，不建议使用该选项。以 1280 * 720 为基准自动缩放
                                            // 例如识别到了 A ，但实际要点击的是 A 下方 10 像素 5 * 2 区域的某个位置，
                                            // 则可填写[ 0, 10, 5, 2 ]，可以的话尽量直接识别要点击的位置，不建议使用该选项
                                            // 额外的，当 action 为 Swipe 时有效且必选，表示滑动终点。

        "reduceOtherTimes": ["OtherTaskName1", "OtherTaskName2"],
                                            // 可选项，执行后减少其他任务的执行计数。
                                            // 例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，所以蓝色开始行动要-1

        "specificRect": [100, 100, 50, 50],
                                            // 当 action 为 ClickRect 时有效且必选，表示指定的点击位置（范围内随机一点）。
                                            // 当 action 为 Swipe 时有效且必选，表示滑动起点。
                                            // 以 1280 * 720 为基准自动缩放
                                            // 当 algorithm 为 "OcrDetect" 时，specificRect[0] 和 specificRect[1] 表示灰度上下限阈值。

        "specialParams": [int, ...],        // 某些特殊识别器需要的参数
                                            // 额外的，当 action 为 Swipe 时可选，[0] 表示 duration，[1] 表示 是否启用额外滑动

        "highResolutionSwipeFix": false,    // 可选项，是否启用高分辨率滑动修复
                                            // 现阶段应该只有关卡导航未使用 unity 滑动方式所以需要开启
                                            // 默认为 false
    
        /* 以下字段仅当 algorithm 为 MatchTemplate 时有效 */

        "template": "xxx.png",              // 可选项，要匹配的图片文件名，可以是字符串或字符串列表
                                            // 默认 "任务名.png"
                                            // 模板图文件可放在 template 及其子文件夹下，加载时会进行递归搜索

        "templThreshold": 0.8,              // 可选项，图片模板匹配得分的阈值，超过阈值才认为识别到了，可以是数字或数字列表
                                            // 默认 0.8, 可根据日志查看实际得分是多少

        "maskRange": [1, 255],              // 可选项，匹配时的灰度掩码范围。 array<int, 2>
                                            // 例如将图片不需要识别的部分涂成黑色（灰度值为 0）
                                            // 然后设置为 [ 1, 255 ], 匹配的时候即忽略涂黑的部分

        "colorScales": [                    // 当 method 为 HSVCount 或 RGBCount 时有效且必选，数色掩码范围。 
            [                               // list<array<array<int, 3>, 2>> / list<array<int, 2>>
                [23, 150, 40],              // 结构为 [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     内层为 int 时是灰度，
            ],                              //     　　为 array<int, 3> 时是三通道颜色，method 决定其是 RGB 或 HSV；
            ...                             //     中间一层的 array<*, 2> 是颜色（或灰度）下限与上限：
        ],                                  //     最外层代表不同的颜色范围，待识别区域为它们对应在模板图片上掩码的并集。

        "colorWithClose": true,             // 可选项，当 method 为 HSVCount 或 RGBCount 时有效，默认为 true
                                            // 数色时是否先用闭运算处理掩码范围。
                                            // 闭运算可以填补小黑点，一般会提高数色匹配效果，但若图片中包含文字建议设为 false

        "method": "Ccoeff",                 // 可选项，模板匹配算法，可以是列表
                                            // 不填写时默认为 Ccoeff
                                            //      - Ccoeff:       对颜色不敏感的模板匹配算法，对应 cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     对颜色敏感的模板匹配算法，
                                            //                      先将待匹配区域和模板图片依据 colorScales 二值化，
                                            //                      以 F1-score 为指标计算 RGB 颜色空间内的相似度，
                                            //                      再将结果与 Ccoeff 的结果点积
                                            //      - HSVCount:     类似 RGBCount，颜色空间换为 HSV

        /* 以下字段仅当 algorithm 为 OcrDetect 时有效 */

        "text": [ "接管作战", "代理指挥" ],  // 必选项，要识别的文字内容，只要任一匹配上了即认为识别到了

        "ocrReplace": [                     // 可选项，针对常见识别错的文字进行替换（支持正则）
            [ "千员", "干员" ],
            [ ".+击干员", "狙击干员" ]
        ],

        "fullMatch": false,                 // 可选项，文字识别是否需要全字匹配（不能多字），默认为 false
                                            // false 时只要是子串即可：例如 text: [ "开始" ]，实际识别到了 "开始行动"，也算识别成功；
                                            // true 时则必须识别到了 "开始"，多一个字都不行

        "isAscii": false,                   // 可选项，要识别的文字内容是否为 ASCII 码字符
                                            // 不填写默认 false

        "withoutDet": false,                // 可选项，是否不使用检测模型
                                            // 不填写默认 false

        /* 以下字段仅当 algorithm 为 JustReturn，action 为 Input 时有效 */

        "inputText": "A string text.",      // 必选项，要输入的文字内容，以字符串的形式
        
        /* 以下字段仅当 algorithm 为 FeatureMatch 时有效 */

        "template": "xxx.png",              // 可选项，要匹配的图片文件名，可以是字符串或字符串列表
                                            // 默认 "任务名.png"

        "count": 4,                         // 匹配的特征点的数量要求 (阈值), 默认值 = 4

        "ratio": 0.6,                       // KNN 匹配算法的距离比值, [0 - 1.0], 越大则匹配越宽松, 更容易连线. 默认0.6

        "detector": "SIFT",                 // 特征点检测器类型, 可选值为 SIFT, ORB, BRISK, KAZE, AKAZE, SURF; 默认值 = SIFT
                                            // SIFT: 计算复杂度高，具有尺度不变性、旋转不变性。效果最好。
                                            // ORB: 计算速度非常快，具有旋转不变性。但不具有尺度不变性。
                                            // BRISK: 计算速度非常快，具有尺度不变性、旋转不变性。
                                            // KAZE: 适用于2D和3D图像，具有尺度不变性、旋转不变性。
                                            // AKAZE: 计算速度较快，具有尺度不变性、旋转不变性。
    }
}
```

## 表达式计算

任务列表类型字段（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）支持表达式计算。

|    符号     |                           含义                           |                  实例                  |
| :---------: | :------------------------------------------------------: | :------------------------------------: |
|     `@`     |                       `@` 型任务                        |            `Fight@ReturnTo`            |
| `#`（单目） |                          虚任务                          |                `#self`                 |
| `#`（双目） |                          虚任务                          |          `StartUpThemes#next`          |
|     `*`     |                       重复多个任务                       | `(ClickCornerAfterPRTS+ClickCorner)*5` |
|     `+`     |   任务列表合并（在 next 系列字段中同名任务只保留最靠前者）  |                 `A+B`                  |
|     `^`     |         任务列表差（在前者但不在后者，顺序不变）         |   `(A+A+B+C)^(A+B+D)`（结果为 `C`）    |

运算符 `@`, `#`, `*`, `+`, `^` 有优先级：`#`（单目）> `@` = `#`（双目）> `*` > `+` = `^`。

## 特殊任务类型

### 模板任务

**模板任务**包括派生任务与 `@` 型任务。模板任务的核心可以理解为依据基任务**修改字段的默认值**。

#### 派生任务

存在字段 `baseTask` 的任务即为派生任务，字段 `baseTask` 对应的任务称为这个派生任务的**基任务**。对于一个派生任务，

1. 若是模板匹配任务，则**字段 `template`** 的默认值仍为 `"任务名.png"`；
2. 若字段 `algorithm` 与基任务不同，则派生类参数不继承（只继承 `TaskInfo` 定义的参数）；
3. 其余字段的默认值均为**基任务对应字段**。

#### 隐式 `@` 型任务

存在任务 `"A"` 且所有任务文件中均未直接定义的型如 `"B@A"` 的任务即为隐式 `@` 型任务，任务 `"A"` 称为任务 `"B@A"` 的**基任务**。对于一个隐式 `@` 型任务，

1. 其任务列表类型字段（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）的默认值为基任务对应字段**直接**增加 `B@` 前缀（如遇任务名开头为 `#` 则增加 `B` 前缀）；
2. 其余字段的默认值均为**基任务对应字段**（包括字段 `template`）。

#### 显式 `@` 型任务

存在任务 `"A"` 且存在任务文件中直接定义的型如 `"B@A"` 的任务即为显式 `@` 型任务，任务 `"A"` 称为任务 `"B@A"` 的**基任务**。对于一个显式 `@` 型任务，

1. 任务列表类型字段（`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`）的默认值为基任务对应字段**直接**增加 `B@` 前缀（如遇任务名开头为 `#` 则增加 `B` 前缀）；
2. 若是模板匹配任务，则**字段 `template`** 的默认值仍为 `"任务名.png"`；
3. 若字段 `algorithm` 与基任务不同，则派生类参数不继承（只继承 `TaskInfo` 定义的参数）；
4. 其余字段的默认值均为**基任务对应字段**。

### 虚任务

虚任务即型如 `"#{sharp_type}"` 或 `"B#{sharp_type}"` 的任务，其中 `{sharp_type}` 可以是 `none`, `self`, `back`, `next`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`。

可以将虚任务分为**指令虚任务**（`#none` / `#self` / `#back`）和**字段虚任务**（`#next` 等）。

|  虚任务类型  |        含义        |                                                                 简单示例                                                                 |
| :----------: | :----------------: | :--------------------------------------------------------------------------------------------------------------------------------------: |
|     none     |       空任务       | 直接跳过<sup>1</sup><br>`"A": {"next": ["#none", "T1"]}` 被解释为 `"A": {"next": ["T1"]}`<br>`"A#none + T1"` 被解释为 `"T1"`  |
|     self     |      当前任务名      | `"A": {"next": ["#self"]}` 中的 `"#self"` 被解释为 `"A"`<br>`"B": {"next": ["A@B@C#self"]}` 中的 `"A@B@C#self"` 被解释为 `"B"`<sup>2</sup> |
|     back     |   # 前面的任务名   |                                      `"A@B#back"` 被解释为 `"A@B"`<br>`"#back"` 直接出现则会被跳过<sup>3</sup>                         |
| next, sub 等 | # 前任务名对应字段 |                      以 `next` 为例：<br>`"A#next"` 被解释为 `Task.get("A")->next`<br>`"#next"` 直接出现则会被跳过                       |

*Note<sup>1</sup>: `"#none"` 一般配合模板任务增加前缀的特性使用，或用在字段 `baseTask` 中避免多文件继承不必要的字段。*

*Note<sup>2</sup>: `"XXX#self"` 与 `"#self"` 含义相同。*

*Note<sup>3</sup>: 当几个任务有 `"next": [ "#back" ]` 时，`"T1@T2@T3"` 代表依次执行 `T3`, `T2`, `T1`。*

### 多文件任务

如果后加载的任务文件 (例如外服 `tasks.json`; 下称文件二) 中定义的任务在先加载的任务文件 (例如国服 `tasks.json`; 下称文件一) 中也定义了同名任务，那么:

- 如果文件二中任务没有 `baseTask` 字段，则直接继承文件一中同名任务的字段。
- 如果文件二中任务有 `baseTask` 字段，则不继承文件一中同名任务的字段，而是直接覆盖。特别地，在没有模板任务时你可以使用 `"baseTask": "#none"` 来避免继承不必要的字段。

### 使用示例

- 派生任务示例（字段 `baseTask`）

    假设定义了如下两个任务：

    ```json
    "Return": {
        "action": "ClickSelf",
        "next": [ "Stop" ]
    },
    "Return2": {
        "baseTask": "Return"
    },
    ```

    则 `"Return2"` 任务的参数会直接从 `"Return"` 任务继承，实际上包含以下参数：

    ```json
    "Return2": {
        "algorithm": "MatchTemplate", // 直接继承
        "template": "Return2.png",    // "任务名.png"
        "action": "ClickSelf",        // 直接继承
        "next": [ "Stop" ]            // 直接继承，与 Template Task 相比这里没有前缀
    }
    ```

- `@` 型任务示例

    假设定义了包含以下参数的任务 `"A"`：

    ```json
    "A": {
        "template": "A.png",
        ...,
        "next": [ "N1", "#back" ]
    },
    ```

    如果任务 `"B@A"` 没有被直接定义，那么任务 `"B@A"` 实际上有以下参数：

    ```json
    "B@A": {
        "template": "A.png",
        ...,
        "next": [ "B@N1", "B#back" ]
    }
    ```

    如果任务 `"B@A"` 有定义 `"B@A": {}`，那么任务 `"B@A"` 实际上有以下参数：

    ```json
    "B@A": {
        "template": "B@A.png",
        ...,
        "next": [ "B@N1", "B#back" ]
    }
    ```

- 虚任务示例

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

### 注意事项

如果任务列表类型字段（`next` 等）中定义的任务包含低优先级运算，则实际结果可能不符预期，需要注意。

1. `@` 与 双目 `#` 的运算顺序导致的特例

    ```json
    {
        "A": { "next": ["N0"] },
        "B": { "next": ["A#next"] },
        "C@A": { "next": ["N1"] }
    }
    ```

    以上这种情况， `"C@B" -> next`（即 `C@A#next`）为 `[ "N1" ]` 而不是 `[ "C@N0" ]`.

2. `@` 与 `+` 的运算顺序导致的特例

    ```json
    {
        "A": { "next": ["#back + N0"] },
        "B@A": {}
    }
    ```

    以上这种情况，

    ```cpp
    Task.get("A")->next = { "N0" };

    Task.get_raw("B@A")->next = { "B#back + N0" };
    Task.get("B@A")->next = { "B", "N0" }; // 注意不是 [ "B", "B@N0" ]
    ```

    事实上，你可以用这个特性来避免添加不必要的前缀，只需要定义

    ```json
    {
        "A": { "next": ["#none + N0"] }
    }
    ```

## 运行时修改任务

- `Task.lazy_parse()` 可以在运行时加载 json 任务配置文件。 lazy_parse 规则与[多文件任务](#多文件任务)相同。
- `Task.set_task_base()` 可以修改任务的 `baseTask` 字段。

### 使用示例

假设有任务配置文件如下：

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

以下代码可以实现根据 mode 的值改变任务 "A"，同时会改变其它依赖任务 "A" 的任务，如 "B@A":

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // 基本上相当于用 A_mode1 的内容直接替换 A，下同
    break;
case 2:
    Task.set_task_base("A", "A_mode2");
    break;
default:
    Task.set_task_base("A", "A_default");
    break;
}
```

## Schema 检验

本项目为 `tasks.json` 配置了 json schema 检验，schema 文件为`docs/maa_tasks_schema.json`。

### Visual Studio

在 `MaaCore.vcxporj` 中已对其进行配置，开箱即用。提示效果较为晦涩，且有部分信息缺失。

### Visual Studio Code

在 `.vscode/settings.json` 中已对其进行配置，用 vscode 打开该**项目文件夹**即可使用。提示效果较好。
