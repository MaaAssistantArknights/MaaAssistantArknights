# ReadmeShotDemo

README 界面截图自动生成的演示数据。MAA GUI 内置演示模式：加载本目录下的 `readme-demo-data.json`，按数据渲染界面并自动截图，产出仓库 README 所用的全部图片（5 语言 × 4 页 × 明暗主题共 40 张），不再依赖手工截图。

## 前置条件

先构建 MaaWpfGui，得到 `build/bin/Debug/MAA.exe`（或对应的 publish 产物）。演示模式不连模拟器，界面上所有识别结果、日志均来自演示数据；唯一例外是标题版本段取 `latest`（见下表）时会联网查询一次 GitHub 最新 Release。

## 用法

```bat
MAA.exe --demo tools/ReadmeShotDemo/readme-demo-data.json --shots <输出目录>
```

或直接运行本目录的 `run-demo.bat`（定位 `build\bin\Debug\MAA.exe`，输出默认写到 `docs/.vuepress/public/images` 直接覆盖 README 图；可传第一个参数另指定输出目录）。

输出结构为 `<输出目录>/<lang>/readme/N-{light,dark}.png`（如 `zh-cn/readme/1-light.png`），与仓库 `docs/.vuepress/public/images/` 的目录结构一致，`<输出目录>` 可直接指向 `docs/.vuepress/public/images` 覆盖现有 README 图。

## 数据结构

`readme-demo-data.json` 各字段与界面部位的对应关系：

| 字段 | 对应界面 | 说明 |
| --- | --- | --- |
| `clientType` | 标题栏客户端类型 | 按语言给出枚举名（如 `Official`、`Txwy`），标题文案由 GUI 本地化渲染 |
| `windowTitle` | 标题栏版本段 | 可选字段；`version` 覆盖标题栏的版本号段，缺省或填 `latest` 时演示启动时联网取 GitHub 最新 Release 的 tag_name（查询失败静默回退本机版本显示，不弹窗不阻塞），也可填具体版本号原样使用；`resourceVersion` 为资源版本段，填空串即隐藏。两者均可缺省，缺省走原行为（显示真实版本） |
| `depot` | 小工具 - 仓库识别页 | MAA 仓库识别持久化格式原样内嵌（`data` 为 itemId → 数量，材料名与图标由 GUI 查表渲染），条目以常规高价值材料的观感为准控制在 25 条左右（仓库识别页 5 列布局约一页多一点），itemId 对应 `resource/item_index.json`；`syncTime`（ ｢上次同步时间｣ ）不需要写，由演示模式自动取启动日 03:25 |
| `operBox` | 小工具 - 干员识别页 | 可选字段；MAA 干员识别持久化格式原样内嵌（`own_opers`，`name` 仅兜底，GUI 按 `id` 查表本地化显示）。缺省时由 GUI 从 battle_data 干员全集自动生成全干员数据：全部 `own: true`、潜能 6，练度按星级上限（6 星精二 90 / 5 星精二 80 / 4 星精二 70 / 3 星精一 60 / 2 星 45 / 1 星 30），`syncTime` 同样由演示模式自动取启动日 03:25，与仓库识别页一致 |
| `taskQueue` | 一键长草页左列与中列 | `tasks` 为任务列表的完整有序序列，取值为一键长草页 ｢添加｣ 菜单可添加的任务类型全集（`OperProgress` 菜单项隐藏、`Custom` 仅 Debug 版可见，均不可添加）：`StartUp` / `Fight` / `Infrast` / `Recruit` / `Mall` / `Award` / `UserDataUpdate` / `DepotMaintain` / `SwitchTheme` / `Roguelike` / `Reclamation`；`enabled` 为勾选状态，顺序即长草页任务列的显示顺序，`status` 为条目状态展示（`idle` / `inProgress` / `completed`，缺省 `idle`，可与日志区的演示进度对齐，如公招进行中、之前的任务已完成）；`stages` 为候选关卡显示字符串数组，全语言共享（关卡代号语言无关，名称本地化不依赖语言） |
| `thumbnails` | 一键长草页日志卡片缩略图 | 图片路径列表（相对本数据文件所在目录），供日志条目按索引引用；5 语言共用同一组图片（图内游戏画面分辨不出界面语言） |
| `taskQueueLogs` | 一键长草页右侧日志 | 逐条日志，`split` 控制日志卡片分组：`Before` 在本条前新开卡片、`After` 在本条后新开卡片、`Both` 前后皆开（本条独占一张卡） |
| `copilot` | 自动战斗页 | `tabIndex` 标签页、`copilotIds` 作业编号数组（作业列表逐项显示为 `maa://<id>`，顶部输入框显示第一个）、`headerLines` 日志区顶部无时间戳的干员组行、`logs` 下方执行日志 |

日志条目的公共字段：`time` 时间戳（同一演示数据内 5 语言共用）、`color` 颜色（`Trace` / `Message` / `Success` / `Info` / `Warning` / `Error` / `RareOperator`，与 GUI 日志色常量同名）、`weight` 字重（`Regular` / `Bold`）。

`taskQueueLogs` 条目另有可选字段 `thumbnail`：指向 `thumbnails` 数组的 0-based 索引，为所在日志卡片挂载任务执行截图缩略图；缺省即无缩略图。每张卡片取其首条日志条目的 `thumbnail`，路径解析失败或索引越界时记日志跳过，不影响截图流程。

所有 `text` 字段必须包含全部 5 种语言（`zh-cn` / `zh-tw` / `en-us` / `ja-jp` / `ko-kr`），多行内容用 `\n` 分隔。

## 修改演示内容

直接编辑 `readme-demo-data.json` 后重新运行上述命令即可。改动日志文案时注意同步 5 种语言；材料条目可直接增删，展示名由 GUI 查表得到，不影响其他部分。干员识别数据缺省即自动生成全干员，无需手工维护名单；如需指定练度或只展示部分干员，再内嵌 `operBox` 覆盖。

## 注意事项

- 演示模式只渲染界面并截图，不连接模拟器，不写入任何配置与数据缓存文件（配置保存链与 `data/` JSON 写入分别在 ConfigFactory 与 JsonDataHelper 层整体拦截，进程内状态退出即弃，不污染运行目录）；除标题版本段取 `latest` 时联网查询一次 GitHub 最新 Release 外不发起任何网络请求。
- MAA.exe 从自身所在目录读取 `data/` 等运行时文件作为界面初始基线，演示不写任何文件、不污染运行目录；为保证截图内容确定（连接配置、功能开关等基线不受该目录历史配置影响），仍建议在独立的构建输出目录执行。
- `--demo` / `--shots` 的相对路径按启动 MAA.exe 时的工作目录解析（与 `run-demo.bat` 中示例的仓库根用法一致）。
- 产出的 PNG 提交前需过 oxipng：`pre-commit run oxipng --files <图片路径>`（仓库 pre-commit 钩子不限文件范围，不本地处理的话 CI 会补机器修复 commit）。
