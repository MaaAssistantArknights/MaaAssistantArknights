---
order: 4
icon: material-symbols:task
---

# 任务流程协议

`resource/tasks` 的使用方法及各字段说明

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

## 1. 完整字段一览

```json
{
    "TaskName": {                           // 任务名，带 @ 时可能为特殊任务，字段默认值会有不同，详见下方特殊任务类型

        "baseTask": "xxx",                  // 以 xxx 任务为模板生成任务，即继承 xxx 任务，见下文 #任务的继承

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

## 2. 文档术语说明（不是语法）

- `task::A`：指一个名为 `A` 的 task
- `task::A->B`：指一个名为 `A` 的 task，且该 task 继承自 `task:B`
- `arg::key`：指某个 task 的一个字段 `key`，关于字段见上文
- `arg::key=value`：指某个 task 的一个字段 `key`，且这个字段的指为 `value`

## 3. 任务的继承

对于一个任务 `task::A` 继承另一个任务 `task::B`，称为**任务A 继承 任务B**，记作 `task::A->B`，此时，`task::A` 称为 `task::B` 的派生任务，`task::B` 称为 `task::A` 的基任务

### 3.1 继承方式

###  3.1.1 通过 Base Task 继承

对于一个任务 `task::A`，可以通过设置其字段 `arg::baseTask=B`，使其继承 `task::B`，这称为任务的 Base Task 继承（BTE）。

例如，以下内容创建了一个 `task::A->B`：
```json
# 例 3.1.1
{
  "B": {
    "template": "B.png",
    "action": "ClickSelf",
    "next": [ "C" ]
  },
  "A": {
    "baseTask": "B" // 继承 task::B
  }
}
```

### 3.1.2 通过 Template Task 继承（`@` 型继承）

可以通过显式 Template Task 或者隐式 Template Task 进行任务的继承，统称为任务的 Template Task 继承（BBE）。

#### 显式继承

对于一个任务，如果其命名为 `A@B`，那么相当于定义了一个 `task::A@B->B`（注意，并 **不是** `task::A->B`），这称为任务的 Template Task 显式继承（TTEE）。

例如，以下内容创建了一个 `task::A@B->B`：
```json
# 例 3.1.2-1
{
  "B": {
    "template": "B.png",
    "action": "ClickSelf",
    "next": [ "C1", "C2" ]
  },
  "A@B": {  // 注意此时任务名是 A@B 而不是 A，调用 A 可能会出错
  }
}
```

#### 隐式继承

如果在任务列表字段（`arg::next`，`arg::sub` 等）内调用类型 `A@B` 型任务，且 `task::A@B` 未显式定义时，MAA 将会隐式创建一个任务 `task::A@B->B`，这称为任务的 Template Task 隐式继承（TTIE）。

例如，以下内容隐式创建了一个 `task::A@B->B`:
```json
# 例 3.1.2-2
{
  "B": {
    "template": "B.png",
    "action": "ClickSelf",
    "next": [ "C1", "C2" ]
  },
  "D": {
    "next": ["A@B"]         // 这里隐式创建了一个 task::A@B->B
  }
}
```

### 3.2 字段继承与覆写

对于一个派生任务，其字段默认继承于其**基任务**（部分字段除外），同时可以手动进行覆写，此处指的覆写是将字段指修改为**不同于其基任务**的值。

部分特殊字段继承遵循以下规律：

1. 对于使用 BTE 和 BBEE 继承方式的任务，其字段 `arg::template` 不继承。
2. 对于使用 BTE 和 BBEE 继承方式的任务，如果其字段 `arg::algorithm` 被覆写，则派生类参数不继承（即只继承 `TaskInfo` 定义的参数）
3. 对于使用 BBIE 继承方式的任务，其字段 `arg::template` 继承，若其基任务无 `arg::template` 字段，则该字段为默认值（即 `任务名.png`）。
4. 对于 BBE 继承方式的任务 `task::A@B->B`，其任务列表字段（`arg::next`，`arg::sub` 等）继承且在参数前**直接**添加 `A@`前缀（如遇参数为 `#` 开头则添加 `B` 前缀）

例如，`# 例 3.1.1` 中 `task::A` 的定义相当于：
```json
# 例 3.2-1
{
  "A": {
    "baseTask": "B"               // 继承 task::B
    "algorithm": "MatchTemplate", // 这是 task 字段默认值
    "template": "B.png",          // 此字段不继承（条目 1）
    "action": "ClickSelf",        // 从 task::B 中继承的字段
    "next": [ "C" ]               // 从 task::B 中继承的字段
  }
}
```
`# 例 3.1.2-1` 和 `# 例 3.1.2-2` 中 `task::A@B` 的定义相当于：
```json
# 例 3.2-2
{
  "A@B": {
    "baseTask": "B",              // 相当于本任务继承 task::B
    "algorithm": "MatchTemplate", // 这是 task 字段默认值
    "action": "ClickSelf",        // 从 task::B 中继承的字段
    "next": [ "A@C1", "A@C2" ]    // 从 task::B 中继承的字段，并添加前缀 A@（条目 4）
    # 例 3.1.2-1
    "template": "A@B.png"         // 不继承（条目 1）
    # 例 3.1.2-2
    "template": "B.png"           // 继承（条目 3）
  }
}
```


### 3.3 多文件任务冲突

如果后加载的文件二和先加载的文件一中定义了同名任务 `task::A`，那么：

- 如果文件二中任务没有 `baseTask` 字段，则直接继承文件一中同名任务的字段。
- 如果文件二中任务有 `baseTask` 字段，则不继承文件一中同名任务的字段，而是直接覆盖。



## 4. 任务表达式

特殊表达式：

|           表达式           |                 含义                 |                                                                    实例                                                                    |
|:-----------------------:|:----------------------------------:|:----------------------------------------------------------------------------------------------------------------------------------------:|
|       `(A)#none`        |           忽略此表达式以及其前面的参数           |                                       例如 `A#none + B` 被解释为 `B` <br> `["#none", "B"]` 被解释为 `["B"]`                                        |
|       `(A)#self`        |            当前表达式所在的任务名             |       `"A": {"next": "#self"}` 中的 `"#self"` 被解释为 `"A"`<br>`"B": {"next": "A@B@C#self"}` 中的 `"A@B@C#self"` 被解释为 `"B"`。<sup>1</sup>        |
|       `(A)#back`        |             表达式前面的任务名              |                                             `"A@B#back"` 被解释为 `"A@B"`<br>`"#back"` 直接出现则会被跳过                                             |
| `(A)#next`, `(A)#sub` 等 |     表达式前面任务的 `next`，`sub` 等字段      |                               以 `next` 为例：<br>`"A#next"` 被解释为 `Task.get("A")->next`<br>`"#next"` 直接出现则会被跳过                               |
|          `A@B`          |            创建隐式继承的派生任务             | `A@B`，若 `task::B` 中的 `next` 等任务列表字段含有 `#back`，则 `#back` 解析为 `A` <br> `A@B@C`，若 `task::C` 中的 `next` 等任务列表字段含有 `#back`，则 `#back` 解析为 `A@B` |
|          `A+B`          | 任务列表合并（在 `next` 系列属性中同名任务只保留最靠前者）  |                                                                  `A+B`                                                                   |
|          `A*B`          |              重复执行多次任务              |                                                  `(ClickCornerAfterPRTS+ClickCorner)*5`                                                  |
|          `A^B`          |        任务列表差（在前者但不在后者，顺序不变）        |                                                       `(A+A+B+C)^(A+B+D)`（结果为 `C`）                                                       |

_Note<sup>1</sup>: `"XXX#self"` 与 `"#self"` 含义相同。_

表达式 `A@B`, `(A)#X`, `A*B`, `A+B`, `A^B` 有优先级：`#X`> `A@B` = `A#X` > `A*B` > `A+B` = `A^B`。


### 4.1 简单示例

```json
# 例 4.1-1
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
# 例 4.1-2
Task.get("C")->next = { "B@N1", "B@N2" };

Task.get("B@Loading")->next = { "B@Loading", "Other", "B" };
Task.get("Loading")->next = { "Loading" };
Task.get_raw("B@Loading")->next = { "B#self", "B#next", "B#back" };
```

### 4.2 一些用途

- 当几个任务有 `"next": [ "#back" ]` 时，`"Task1@Task2@Task3"` 代表依次执行 `Task3`, `Task2`, `Task1`。

### 4.3 其它相关

```json
# 例 4.3-1
{
    "A": { "next": ["N0"] },
    "B": { "next": ["A#next"] },
    "C@A": { "next": ["N1"] }
}
```

以上这种情况， `"C@B" -> next`（即 `C@A#next`）为 `[ "N1" ]` 而不是 `[ "C@N0" ]`.


### 4.3 Schema 检验

本项目为 `tasks.json` 配置了 json schema 检验，schema 文件为`docs/maa_tasks_schema.json`。

#### 4.3.1 Visual Studio

在 `MaaCore.vcxporj` 中已对其进行配置，开箱即用。提示效果较为晦涩，且有部分信息缺失。

#### 4.3.2 Visual Studio Code

在 `.vscode/settings.json` 中已对其进行配置，用 vscode 打开该**项目文件夹**即可使用。提示效果较好。

## 5. 运行时修改任务

- `Task.lazy_parse()` 可以在运行时加载 json 任务配置文件。 lazy_parse 规则与[多文件任务冲突](#33-多文件任务冲突)相同。
- `Task.set_task_base()` 可以修改任务的 `baseTask` 字段。

### 5.1 使用示例

假设有任务配置文件如下：

```json
# 例 5.1-1
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
例 5.1-2
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
