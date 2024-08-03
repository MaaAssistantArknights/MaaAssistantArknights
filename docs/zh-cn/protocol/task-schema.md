---
order: 4
icon: material-symbols:task
---

# 任务流程协议

`resource/tasks.json` 的使用方法及各字段说明

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

## 完整字段一览

```json
{
    "TaskName" : {                          // 任务名，带 @ 时可能为特殊任务，字段默认值会有不同，详见下方特殊任务类型

        "baseTask": "xxx",                  // 以 xxx 任务为模板生成任务，详细见下方特殊任务类型中的 Base Task

        "algorithm": "MatchTemplate",       // 可选项，表示识别算法的类型
                                            // 不填写时默认为 MatchTemplate
                                            //      - JustReturn:       不进行识别，直接执行 action
                                            //      - MatchTemplate:    匹配图片
                                            //      - OcrDetect:        文字识别

        "action": "ClickSelf",              // 可选项，表示识别到后的动作
                                            // 不填写时默认为 DoNothing
                                            //      - ClickSelf:        点击识别到的位置（识别到的目标范围内随机点）
                                            //      - ClickRect:        点击指定的区域，对应 specificRect 字段，不建议使用该选项
                                            //      - DoNothing:        什么都不做
                                            //      - Stop:             停止当前任务
                                            //      - Swipe:            滑动，对应 specificRect 与 rectMove 字段

        "sub": [ "SubTaskName1", "SubTaskName2" ],
                                            // 可选项，子任务，不推荐使用。会在执行完当前任务后，依次执行每一个子任务
                                            // 可以套娃，子任务再套子任务。但要注意不要写出了死循环

        "subErrorIgnored": true,            // 可选项，是否忽略子任务的错误。
                                            // 不填写默认 false
                                            // 为 false 时，若某个子任务出错，则不继续执行后续任务（相当于本任务出错了）
                                            // 为 true 时，子任务是否出错没有影响

        "next": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可选项，表示执行完当前任务和 sub 任务后，下一个要执行的任务
                                            // 会从前往后依次去识别，去执行第一个匹配上的
                                            // 不填写默认执行完当前任务直接停止
                                            // 对于相同任务，第一次识别后第二次就不再识别：
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // 不允许 JustReturn 型任务位于非最后一项

        "maxTimes": 10,                     // 可选项，表示该任务最大执行次数
                                            // 不填写时默认无穷大
                                            // 达到最大次数后，若存在 exceededNext 字段，则执行 exceededNext；否则直接任务停止

        "exceededNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可选项，表示达到了最大执行次数后要执行的任务
                                            // 不填写时，达到了最大执行次数则停止；填写后就执行这里的，而不是 next 里的
        "onErrorNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可选项，表示执行出错时，后续要执行的任务

        "preDelay": 1000,                   // 可选项，表示识别到后延迟多久才执行 action，单位毫秒；不填写时默认 0
        "postDelay": 1000,                  // 可选项，表示 action 执行完后延迟多久才去识别 next, 单位毫秒；不填写时默认 0

        "roi": [ 0, 0, 1280, 720 ],         // 可选项，表示识别的范围，格式为 [ x, y, width, height ]
                                            // 以 1280 * 720 为基准自动缩放；不填写时默认 [ 0, 0, 1280, 720 ]
                                            // 尽量填写，减小识别范围可以减少性能消耗，加快识别速度

        "cache": false,                     // 可选项，表示该任务是否使用缓存，默认为 false;
                                            // 第一次识别到后，以后永远只在第一次识别到的位置进行识别，开启可大幅节省性能
                                            // 但仅适用于待识别目标位置完全不会变的任务，若待识别目标位置会变请设为 false

        "rectMove": [ 0, 0, 0, 0 ],         // 可选项，识别后的目标移动，不建议使用该选项。以 1280 * 720 为基准自动缩放
                                            // 例如识别到了 A ，但实际要点击的是 A 下方 10 像素 5 * 2 区域的某个位置，
                                            // 则可填写[ 0, 10, 5, 2 ]，可以的话尽量直接识别要点击的位置，不建议使用该选项
                                            // 额外的，当 action 为 Swipe 时有效且必选，表示滑动终点。

        "reduceOtherTimes": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // 可选项，执行后减少其他任务的执行计数。
                                            // 例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，所以蓝色开始行动要-1

        "specificRect": [ 100, 100, 50, 50 ],
                                            // 当 action 为 ClickRect 时有效且必选，表示指定的点击位置（范围内随机一点）。
                                            // 当 action 为 Swipe 时有效且必选，表示滑动起点。
                                            // 以 1280 * 720 为基准自动缩放

        "specialParams": [ int, ... ],      // 某些特殊识别器需要的参数
                                            // 额外的，当 action 为 Swipe 时可选，[0] 表示 duration，[1] 表示 是否启用额外滑动

        /* 以下字段仅当 algorithm 为 MatchTemplate 时有效 */

        "template": "xxx.png",              // 可选项，要匹配的图片文件名，可以是字符串或字符串列表
                                            // 默认 "任务名.png"

        "templThreshold": 0.8,              // 可选项，图片模板匹配得分的阈值，超过阈值才认为识别到了，可以是数字或数字列表
                                            // 默认 0.8, 可根据日志查看实际得分是多少

        "maskRange": [ 1, 255 ],            // 可选项，掩码范围。 array<int, 2> | list<array<array<int, 3>, 2>>
                                            // 当为 array<int, 2> 时 是灰度掩码范围
                                            //     例如将图片不需要识别的部分涂成黑色（灰度值为 0）
                                            //     然后设置"maskRange"的范围为 [ 1, 255 ], 匹配的时候即忽略涂黑的部分
                                            // 当为 list<array<array<int, 3>, 2>> 时
                                            //     最内层代表一个颜色，由 method 决定它是 RGB 或 HSV；
                                            //     中间一层是颜色范围的下限和上限；
                                            //     最外层代表允许多个范围，最后取并集作为待识别区域。

        "method": "Ccoeff",                 // 可选项，模板匹配算法，可以是列表
                                            // 不填写时默认为 Ccoeff
                                            //      - Ccoeff:       对应 cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     先将待匹配区域和模板图片依据 maskRange 二值化，
                                            //                      以 F1-score 为指标计算 RGB 颜色空间内的相似度，
                                            //                      再将结果与 Ccoeff 的结果点积
                                            //                      总之是一个颜色敏感的模板匹配算法
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

        "withoutDet": false                 // 可选项，是否不使用检测模型
                                            // 不填写默认 false

    }
}
```

## 特殊任务类型

### Template Task（`@` 型任务）

Template task 与 base task 合称**模板任务**。

允许把某个任务 "A" 当作模板，然后 "B@A" 表示由 "A" 生成的任务。

- 如果 `tasks.json` 中未显式定义任务 "B@A"，则在 `sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes` 字段中增加 `B@` 前缀（如遇任务名开头为 `#` 则增加 `B` 前缀），其余参数与 "A" 任务相同。就是说如果任务 "A" 有以下参数：

    ```json
    "A": {
        "template": "A.png",
        ...,
        "next": [ "N1", "N2" ]
    }
    ```

    就相当于同时定义了

    ```json
    "B@A": {
        "template": "A.png",
        ...,
        "next": [ "B@N1", "B@N2" ]
    }
    ```

- 如果 `tasks.json` 中定义了任务 "B@A"，则：
    1. 如果 "B@A" 与 "A" 的 `algorithm` 字段不同，则派生类参数不继承（只继承 `TaskInfo` 定义的参数）
    2. 如果是图像匹配任务，`template` 若未显式定义则为 `B@A.png`（而不是继承"A"的 `template` 名），其余情况任何派生类参数若未显式定义，直接继承 "A" 任务的参数
    3. 对于 `TaskInfo` 基类中定义的参数（任何类型任务都有的参数，例如 `algorithm`, `roi`, `next` 等），若没有在 "B@A" 内显式定义，则除了上面提到的 `sub` 等五个字段在继承时会增加 "B@" 前缀外，其余参数直接继承 "A" 任务的参数

### Base Task

Base task 与 template task 合称**模板任务**。

有字段 `baseTask` 的任务即为 base task。

Base task 的逻辑优先于 template task。这意味着 `"B@A": { "baseTask": "C", ... }` 与任务 A 没有任何相关。

任何参数若未显式定义则不加前缀地直接使用 `baseTask` 对应任务的参数，除了 `template` 未显式定义时仍为 `"任务名.png"`。

#### 多文件任务

如果后加载的任务文件 (例如外服 `tasks.json`; 下称文件二) 中定义的任务在先加载的任务文件 (例如国服 `tasks.json`; 下称文件一) 中也定义了同名任务，那么:

- 如果文件二中任务没有 `baseTask` 字段，则直接继承文件一中同名任务的字段。
- 如果文件二中任务有 `baseTask` 字段，则不继承文件一中同名任务的字段，而是直接覆盖。

### Virtual Task（虚任务）

Virtual task 也称 sharp task（`#` 型任务）。

任务名带 `#` 的任务即为 virtual task。 `#` 后可接 `next`, `back`, `self`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`。

|  虚任务类型  |        含义        |                                                                 简单示例                                                                 |
| :----------: | :----------------: | :--------------------------------------------------------------------------------------------------------------------------------------: |
|     self     |      父任务名      | `"A": {"next": "#self"}` 中的 `"#self"` 被解释为 `"A"`<br>`"B": {"next": "A@B@C#self"}` 中的 `"A@B@C#self"` 被解释为 `"B"`。<sup>1</sup> |
|     back     |   # 前面的任务名   |                                      `"A@B#back"` 被解释为 `"A@B"`<br>`"#back"` 直接出现则会被跳过                                       |
| next, sub 等 | # 前任务名对应字段 |                      以 `next` 为例：<br>`"A#next"` 被解释为 `Task.get("A")->next`<br>`"#next"` 直接出现则会被跳过                       |

_Note<sup>1</sup>: `"XXX#self"` 与 `"#self"` 含义相同。_

#### 简单示例

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

#### 一些用途

- 当几个任务有 `"next": [ "#back" ]` 时，`"Task1@Task2@Task3"` 代表依次执行 `Task3`, `Task2`, `Task1`。

#### 其它相关

```json
{
    "A": { "next": ["N0"] },
    "B": { "next": ["A#next"] },
    "C@A": { "next": ["N1"] }
}
```

以上这种情况， `"C@B" -> next`（即 `C@A#next`）为 `[ "N1" ]` 而不是 `[ "C@N0" ]`.

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

## 表达式计算

|    符号     |                           含义                           |                  实例                  |
| :---------: | :------------------------------------------------------: | :------------------------------------: |
|     `@`     |                         模板任务                         |            `Fight@ReturnTo`            |
| `#`（单目） |                          虚任务                          |                `#self`                 |
| `#`（双目） |                          虚任务                          |          `StartUpThemes#next`          |
|     `*`     |                       重复多个任务                       | `(ClickCornerAfterPRTS+ClickCorner)*5` |
|     `+`     | 任务列表合并（在 next 系列属性中同名任务只保留最靠前者） |                 `A+B`                  |
|     `^`     |         任务列表差（在前者但不在后者，顺序不变）         |   `(A+A+B+C)^(A+B+D)`（结果为 `C`）    |

运算符 `@`, `#`, `*`, `+`, `^` 有优先级：`#`（单目）> `@` = `#`（双目）> `*` > `+` = `^`。

## Schema 检验

本项目为 `tasks.json` 配置了 json schema 检验，schema 文件为`docs/maa_tasks_schema.json`。

### Visual Studio

在 `MaaCore.vcxporj` 中已对其进行配置，开箱即用。提示效果较为晦涩，且有部分信息缺失。

### Visual Studio Code

在 `.vscode/settings.json` 中已对其进行配置，用 vscode 打开该**项目文件夹**即可使用。提示效果较好。
