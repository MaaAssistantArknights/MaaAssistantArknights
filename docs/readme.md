# MAA使用说明

## 详细介绍

### 刷理智

- 若选项中没有你需要的关卡，请手动进入游戏 **蓝色开始按钮** 界面，并选择 `当前关卡`。
- 主界面上的 `吃理智`、`吃石头`、`指定次数` 三个选项为短路开关，即三个选项中的任一条件达到，均会视为任务完成，停止刷理智。
    - 举例1：设置 `吃理智药` : `999`、`吃石头` : `10`、`指定次数` : `1`。则在刷完 **一次** 后，由于满足了 `指定次数`:`1` 的条件，视为任务完成，停止刷理智。
    - 举例2：不勾选 `吃理智药`、不勾选 `吃石头`，设置 `指定次数` : `100`。则在当前可用理智全部刷完后（可能只刷了几次），由于满足了 `不吃理智药`、`不吃石头` 的条件，视为任务完成，停止刷理智。
- 刷完自动上传 [企鹅物流数据统计](https://penguin-stats.cn/)
- 可自定义企鹅物流 ID
- 识别并显示材料掉落
- 掉线后会重连，继续刷上次的图
- 凌晨 4 点刷新后也会重连，继续刷上次的图
- 支持剿灭模式
- 支持打完升级了的情况
- 支持代理失败的情况，会自动放弃本次行动

### 基建换班

#### 换班策略

自动计算并选择 **单设施内的最优解**，支持所有通用类技能和特殊技能组合；支持识别经验书、赤金、原石碎片、芯片，分别使用不同的干员组合！

#### 宿舍入驻心情阈值

识别心情进度条的百分比；心情小于该阈值的干员，不会再去上班，直接进驻宿舍。

#### 特殊说明

- 基建换班目前均为单设施最优解，但非跨设施的全局最优解。例如：`巫恋+龙舌兰`、`红云+稀音` 等这类单设施内的组合，都是可以正常识别并使用的；`迷迭香`、`红松骑士团` 这类多个设施间联动的体系，暂不支持使用。
- 若 `无人机用途` 选择 `贸易站-龙门币` ，则将额外识别是否有 `巫恋组`，并优先为其使用。
- 会客室仅缺一个线索时，会选择对应流派的干员；否则会选择通用干员。
- 会客室仅当自有线索满时，才会送出线索，并且只送三个。有需要的同学可自行修改 `resource/tasks.json` 中 `SelectClue` - `maxTimes` 字段，自定义送出个数。
- 控制中枢策略太过复杂，目前只考虑 `阿米娅`、`诗怀雅`、`凯尔希`、`彩虹小队` 及其他心情 +0.05 的干员，后续逐步优化。
- 部分异格干员会在基建内产生冲突，请留意辅助界面上是否有提示 “干员冲突”，若有请手动再查看下基建换班情况（可能会有某个设施没人）

### 信用商店随缘买

从左到右依次买，但不会买 `碳` 和 `家具零件` 。有需要的同学可自行修改 `resource/tasks.json` 中 `CreditShop-NotToBuy` - `text` 字段，自定义不买的物品。后续版本会尝试开放界面选项

### 公开招募识别

- 自动公招和公招识别是两个独立的功能！
- 自动公招支持使用 `加急许可`，全自动连续公招！请进入设置中选择~
- 出 5、6 星都会有弹窗提示

### 自动肉鸽功能

- 设置中的三个选项  
    - 尽可能往后打：用于刷蜡烛
    - 刷源石锭投资，第一层商店后直接退出：刷源石锭最高效的模式，建议手动选择 `古堡观光` （简单）模式
    - 刷源石锭投资，投资后退出：兼顾刷蜡烛和刷源石锭投资的模式
- 肉鸽使用干员是可以自定义的，默认的干员是作者根据自己的练度随手写的，如果经常作战失败，建议按你自己的练度修改一下，请参考 [肉鸽自定义选人方法](https://github.com/MistEO/MeoAssistantArknights/issues/167)。后期将优化这里的选人逻辑
- 支持掉线重连、支持凌晨 4 点更新后继续回去刷
- 如果软件出现 bug 卡住了，会自动放弃当次探索并重试。但如果经常在某个地方卡住然后放弃、严重影响效率的，欢迎提个 issue 反馈一下~


### 其他乱七八糟的说明

- 主界面上要执行的任务，是可以拖动改变顺序的。同样设置中基建换班的顺序，也是可以拖动改变的。
- 新的活动关卡刚上线的时候可能无法正常识别，一般一两天内软件会自动 OTA 更新资源，更新后即可正常识别。
- 活动关卡掉落识别到的 `未知材料`，一般就是活动商店的票据。
- `resource/config.json` 中有一些自定义选项，可以尝试根据自己的需要进行修改。
- 所有点击操作，都是点击按钮内随机位置，并模拟泊松分布（按钮偏中间位置点的概率大，越往旁边点到的概率越小）。
- 底层算法纯 C++ 开发，并设计了多重的缓存技术，最大限度降低 CPU 和内存占用。
- 软件支持自动更新✿✿ヽ(°▽°)ノ✿ 推荐非杠精的同学使用测试版，一般来说更新快且 bug 少（什么 MIUI (╯‵□′)╯︵┻━┻
- 如果新版本自动下载失败，可手动下载后，直接把压缩包放到同目录下，会自动更新的。

### 我是国际服玩家，可以使用么？

可以！但是目前只支持少部分功能，请参考 [这个链接](../resource/international/)

### I'm an international server player, could I use it?

Yes, but there are few supported features, please refer to [this link](../resource/international/)
## 模拟器支持

### 蓝叠模拟器

完美支持。需要在模拟器 `设置` - `引擎设置` 中打开 `允许ADB连接`

### 蓝叠模拟器国际版（推荐使用）

完美支持。需要在模拟器 `设定` - `进阶` 中打开 `Android调试桥`

### 蓝叠模拟器 Hyper-V 版本

支持

1. 在模拟器 `设定` - `进阶` 中打开 `Android调试桥`
2. 下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) ，将 `platform-tools` 文件夹解压到 `MeoAsstGui.exe` 的同级目录
3. 在软件 `设置` - `连接设置` 中填写蓝叠安装目录下 `bluestacks.conf` 文件的完整路径

### 夜神模拟器

完美支持

### 夜神模拟器 安卓 9

完美支持

### MuMu 模拟器

支持。但是可能会  

- 一直连接出错：暂时不知道是什么原因，可尝试使用自定义连接
- 卡在主界面无法识别，过一会提示任务出错：和 MuMu 的渲染方式有关，推荐换模拟器（

### MuMu 手游助手（星云引擎）  

不支持。没开放 adb 端口

### MuMu 模拟器 安卓 9

不支持。adb 截图出来是黑的，不知道怎么整

### 雷电模拟器

勉强支持。雷电总有莫名其妙的问题，可以试试看，不保证能用（

### 逍遥模拟器

支持。但可能有莫名其妙的识别错误问题

### 腾讯手游助手

不支持。没开放 adb 端口

### Win11 WSA

不支持。WSA 目前没有实现 AOSP screencap 需要的接口，无法进行截图。等微软支持后会尝试适配

### AVD

支持。

### 自定义连接

1. 下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) ，将 `platform-tools` 文件夹解压到 `MeoAsstGui.exe` 的同级目录
2. 进入软件 `设置` - `连接设置`，填写自定义 adb 地址（需要填写 IP + 端口，例如 `127.0.0.1:5555` ）

**注意 :** 若是使用手机，在非 `16:9` 分辨率下，部分功能不稳定（但也凑合能用），尤其是基建，几乎是不可用状态，正在进一步适配中。推荐使用 `16:9` 分辨率，经过的测试验证最多，也最稳定。
## 常见问题

### 软件一打开就闪退

- 可能性 1: 运行库问题。  
  请尝试安装 [Visual C++ Redistributable](https://docs.microsoft.com/zh-CN/cpp/windows/latest-supported-vc-redist?view=msvc-160#visual-studio-2015-2017-2019-and-2022)、[.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48) 后重新运行本软件。
- 可能性 2: CPU 指令集不支持。  
  项目使用 `PaddleOCR` 对游戏部分界面进行识别。`PaddleOCR` 用到了较新发布的 CPU 才支持的 `AVX` 指令集，而一些较老的 CPU 可能并不支持该指令集。  
  您可以尝试下载 [NoAVX](../3rdparty/ppocr_noavx.zip) 版本的 `PaddleOCR`, 解压后替换本软件中同名的文件。这是对于使用不支持 `AVX` 指令集的 CPU 的用户的性能降低的替代方案，如非必要，请不要使用。  
  （具体可以下载 [CPU-Z](https://www.cpuid.com/softwares/cpu-z.html)，查看“指令集”中有无 `AVX` 这一项来判断）  
- 若上述均没有效果，请提 issue。

### 软件可以正常启动，但一点击开始运行就闪退

设置中的`bluestacks.conf`, 是给 [蓝叠模拟器 Hyper-V 版本](https://github.com/MistEO/MeoAssistantArknights/blob/master/docs/%E6%A8%A1%E6%8B%9F%E5%99%A8%E6%94%AF%E6%8C%81.md#%E8%93%9D%E5%8F%A0%E6%A8%A1%E6%8B%9F%E5%99%A8-hyper-v-%E7%89%88%E6%9C%AC) 使用的，若非该版本，请不要填写该项。如果确定不是这个问题，请再提 issue ~

### 连接错误/捕获模拟器窗口错误

提示 : 请根据 [使用说明](../README.md#使用说明) 确定您的模拟器及打开方式正确

- 方法 1: 使用 [自定义连接](#自定义连接) 的方式连接模拟器
- 方法 2: 换模拟器，推荐 [蓝叠国际版](https://www.bluestacks.com/download.html) Nougat 64 bit
- 方法 3: _根本解决方法_ 编辑 `resource/config.json`, 修改（最好是新增）模拟器窗口句柄名，并修改对应的 adb 设置。若您修改后可以提 PR 给我，我会感激不尽的_(:з」∠)_

### 识别错误/任务开始后一直不动、没有反应

提示 : 依次进行，若问题解决，则无需往下

1. 请根据 [模拟器支持情况](模拟器支持.md) 确定您的模拟器是受支持的
2. 修改模拟器分辨率设置，`320 dpi`
3. 修改模拟器分辨率设置，横屏 `1280 * 720` 分辨率
4. 换模拟器，推荐 [蓝叠国际版](https://www.bluestacks.com/download.html) Nougat 64 bit ( 请注意蓝叠需要在模拟器设置里开启 adb )
5. 若仍有问题，请提 issue

### 自定义连接

- 下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) ，将 `platform-tools` 文件夹解压到 `MeoAsstGui.exe` 的同级目录
- 进入软件 `设置` - `连接设置`，填写自定义 adb 地址（需要填写 IP + 端口，例如 `127.0.0.1:5555` ）

### 环境变量中已有 adb, 可以不再下载一份么？

可以！请手动修改 `resource\config.json` - `Custom` - `adb` - `path` 为 `adb.exe`, 或其他 adb 的路径
## 回调消息协议

### 回调函数原型

```c++
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

### 参数总览

- `int msg`  
    消息类型

    ```c++
    enum class AsstMsg
    {
        /* Global Info */
        InternalError = 0,          // 内部错误
        InitFailed,                 // 初始化失败
        ConnectionError,            // 连接相关错误
        AllTasksCompleted,          // 全部任务完成
        /* TaskChain Info */
        TaskChainError = 10000,     // 任务链执行/识别错误
        TaskChainStart,             // 任务链开始
        TaskChainCompleted,         // 任务链完成
        TaskChainExtraInfo,         // 任务链额外信息
        /* SubTask Info */
        SubTaskError = 20000,       // 原子任务执行/识别错误
        SubTaskStart,               // 原子任务开始
        SubTaskCompleted,           // 原子任务完成
        SubTaskExtraInfo            // 原子任务额外信息
    };
    ```

- `const char* details`  
    消息详情，json 字符串，详见 [字段解释](#字段解释)
- `void* custom_arg`  
    调用方自定义参数，会原样传出 `AsstCreateEx` 接口中的 `custom_arg` 参数，C 系语言可利用该参数传出 `this` 指针

### 字段解释

#### InternalError

Todo

#### InitFailed

```jsonc
{
    "what": string,     // 错误类型
    "details": object,  // 错误详情
}
```

#### ConnectionError

Todo

#### AllTasksCompleted

```jsonc
{
    "taskchain": string,            // 最后的任务链
    "pre_taskchain": string         // 上一个任务链
}
```

##### 常见 `taskchain` 字段

- `StartUp`  
    开始唤醒
- `Fight`  
    刷理智
- `Mall`  
    信用点及购物
- `Recruit`  
    自动公招
- `RecruitCalc`  
    公招识别
- `Infrast`  
    基建换班
- `Roguelike`  
    无限刷肉鸽
- `Debug`  
    调试

#### TaskChain 相关消息

```jsonc
{
    "taskchain": string,            // 当前的任务链
    "pre_taskchain": string         // 上一个任务链
}
```

#### TaskChainExtraInfo

Todo

#### SubTask 相关消息

```jsonc
{
    "subtask": string,             // 子任务名
    "class": string,               // 子任务符号名
    "taskchain": string,           // 当前任务链
    "details": object,             // 详情
}
```

##### 常见 `subtask` 字段

- `ProcessTask`  

    ```jsonc
    // 对应的 details 字段举例
    {
        "task": "StartButton2",     // 任务名
        "action": 512,
        "exec_times": 1,            // 已执行次数
        "max_times": 999,           // 最大执行次数
        "algorithm": 0
    }
    ```

- Todo 其他

###### 常见 `task` 字段

- `StartButton2`  
    开始战斗
- `MedicineConfirm`  
    使用理智药
- `StoneConfirm`  
    碎石
- `RecruitRefreshConfirm`  
    公招刷新标签
- `RecruitConfirm`  
    公招确认招募
- `RecruitNowConfirm`  
    公招使用加急许可
- `ReportToPenguinStats`  
    汇报到企鹅数据统计
- `InfrastDormDoubleConfirmButton`  
    基建宿舍的二次确认按钮，仅当干员冲突时才会有，请提示用户
- `Roguelike1Start`  
    肉鸽开始探索
- `Roguelike1StageTraderInvestConfirm`  
    肉鸽投资了源石锭
- `Roguelike1StageTraderInvestSystemFull`  
    肉鸽投资达到了游戏上限
- `Roguelike1ExitThenAbandon`  
    肉鸽放弃了本次探索
- `Roguelike1MissionCompletedFlag`  
    肉鸽战斗完成
- `Roguelike1MissionFailedFlag`  
    肉鸽战斗失败
- `Roguelike1StageSafeHouseEnter`  
    肉鸽关卡：诡异行商
- `Roguelike1StageSafeHouseEnter`  
    肉鸽关卡：安全的角落
- `Roguelike1StageEncounterEnter`  
    肉鸽关卡：不期而遇/古堡馈赠
- `Roguelike1StageCambatDpsEnter`  
    肉鸽关卡：普通作战
- `Roguelike1StageEmergencyDps`  
    肉鸽关卡：紧急作战
- `Roguelike1StageDreadfulFoe`  
    肉鸽关卡：险路恶敌
- Todo 其他

#### SubTaskExtraInfo

```jsonc
{
    "taskchain": string,           // 当前任务链
    "class": string,               // 子任务类型
    "what": string,                // 信息类型
    "details": object,             // 信息详情
}
```

##### 常见 `what` 及 `details` 字段

- `StageDrops`  
    关卡材料掉落信息

    ```jsonc
    // 对应的 details 字段举例
    {
        "exceptions": [],
        "resultLabel": true,
        "drops": [              // 本次识别到的掉落材料
            {
                "dropType": "NORMAL_DROP",
                "itemId": "3301",
                "quantity": 2,
                "itemName": "技巧概要·卷1"
            },
            {
                "dropType": "NORMAL_DROP",
                "itemId": "3302",
                "quantity": 1,
                "itemName": "技巧概要·卷2"
            },
            {
                "dropType": "NORMAL_DROP",
                "itemId": "3303",
                "quantity": 2,
                "itemName": "技巧概要·卷3"
            }
        ],
        "dropTypes": [
            {
                "dropTypes": "LMB"
            },
            {
                "dropTypes": "NORMAL_DROP"
            }
        ],
        "stage": {              // 关卡信息
            "stageCode": "CA-5",
            "stageId": "wk_fly_5"
        },
        "stars": 3,             // 行动结束星级
        "fingerprint": "e464bafaa3f92877bbc4fb20209e4169ae49d587499c8d2e72026b96356c591f6a17477a5b345518a00b23be78252e0825070585d33f2109390807672c252a0b",
        "md5": "b40ea51ec7802afa2e86951bffbbc21f",
        "cost": 6.0624,
        "stats": [              // 本次执行期间总的材料掉落
            {
                "itemId": "3301",
                "itemName": "技巧概要·卷1",
                "quantity": 4
            },
            {
                "itemId": "3302",
                "itemName": "技巧概要·卷2",
                "quantity": 3
            },
            {
                "itemId": "3303",
                "itemName": "技巧概要·卷3",
                "quantity": 4
            }
        ]
    }
    ```

- `RecruitTagsDetected`  
    公招识别到了 Tags

    ```jsonc
    // 对应的 details 字段举例
    {
        "tags": [
            "费用回复",
            "防护",
            "先锋干员",
            "辅助干员",
            "近战位"
        ]
    }
    ```

- `RecruitSpecialTag`  
    公招识别到了特殊 Tag

    ```jsonc
    // 对应的 details 字段举例
    {
        "tag": "高级资深干员"
    }
    ```

- `RecruitResult`  
    公招识别结果

    ```jsonc
    // 对应的 details 字段举例
    {
        "level": 4,                 // 总的星级
        "result": [
            {
                "tags": [
                    "削弱"
                ],
                "level": 4,         // 这组 tags 的星级
                "opers": [
                    {
                        "name": "初雪",
                        "level": 5  // 干员星级
                    },
                    {
                        "name": "陨星",
                        "level": 5
                    },
                    {
                        "name": "槐琥",
                        "level": 5
                    },
                    {
                        "name": "夜烟",
                        "level": 4
                    },
                    {
                        "name": "流星",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "减速",
                    "术师干员"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜魔",
                        "level": 5
                    },
                    {
                        "name": "格雷伊",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "削弱",
                    "术师干员"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜烟",
                        "level": 4
                    }
                ]
            }
        ]
    }
    ```

- `RecruitTagsSelected`  
    公招选择了 Tags

    ```jsonc
    // 对应的 details 字段举例
    {
        "tags": [
            "减速",
            "术师干员"
        ]
    }
    ```

- `EnterFacility`  
    基建进入了设施

    ```jsonc
    // 对应的 details 字段举例
    {
        "facility": "Mfg",  // 设施名
        "index": 0          // 设施序号
    }
    ```

- `NotEnoughStaff`  
    基建可用干员不足

    ```jsonc
    // 对应的 details 字段举例
    {
        "facility": "Mfg",  // 设施名
        "index": 0          // 设施序号
    }
    ```

- `ProductOfFacility`  
    基建产物

    ```jsonc
    // 对应的 details 字段举例
    {
        "product": "Money", // 产物名
        "facility": "Mfg",  // 设施名
        "index": 0          // 设施序号
    }
    ```

- `StageInfo`  
    自动作战关卡信息

    ```jsonc
    // 对应的 details 字段举例
    {
        "name": string  // 关卡名
    }
    ```

- `StageInfoError`  
    自动作战关卡识别错误

- `PenguinId`  
    企鹅物流 ID

    ```jsonc
    // 对应的 details 字段举例
    {
        "id": string
    }
    ```

