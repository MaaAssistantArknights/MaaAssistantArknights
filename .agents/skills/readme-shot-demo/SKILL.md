---
name: readme-shot-demo
description: 用 MAA GUI 内置演示模式（tools/ReadmeShotDemo）重新生成仓库 README 界面截图。用户提到更新/换 README 截图、README 里的画面过期、UI 大改后要更新 README 展示图、跑 ReadmeShotDemo 或 demo 截图时使用，即使用户没有明确说出 ｢ReadmeShotDemo｣ 一词。
---

# MAA README 截图（ReadmeShotDemo）

MAA GUI 内置演示模式：加载 `tools/ReadmeShotDemo/readme-demo-data.json`，按数据渲染界面并自动截图，产出仓库 README 所用全部图片（5 语言 × 4 页 × 明暗主题共 40 张）后自动退出。演示不连模拟器、不写任何配置与数据文件；唯一联网点是标题版本段取 `latest` 时查询一次 GitHub 最新 Release（失败静默回退本机版本，不阻塞）。

页面编号与语言目录：输出为 `<输出目录>/<lang>/readme/N-{light,dark}.png`，与 `docs/.vuepress/public/images/` 结构一致。

| N | 页面 |
| --- | --- |
| 1 | 一键长草 |
| 2 | 自动战斗 |
| 3 | 小工具 - 干员识别 |
| 4 | 小工具 - 仓库识别 |

引用范围：仓库根 `README.md` 用 zh-cn 图；`docs/<lang>/readme.md` 各引用同语言图。更新任何一页时 5 个语言目录一起换，保持语言间一致。

## 工作流程

1. **前置检查**：`build/bin/Debug/MAA.exe` 的 mtime 须新于 `src/MaaWpfGui` 的最新提交，过期则先 `dotnet build src/MaaWpfGui/MaaWpfGui.csproj -r win-x64`（禁 `--no-self-contained` 与裸 `-o`）。
2. **界面基线**：演示不写配置，界面开关与参数取 MAA.exe 所在目录 `config/gui.new.json` 的现状（如干员识别头像模式 `OperBoxAvatarMode`、理智作战各参数）。README 图需要展示某个状态（如头像模式）时，先确认该基线已处于期望状态。
3. **截图**：
   ```bash
   ./build/bin/Debug/MAA.exe --demo tools/ReadmeShotDemo/readme-demo-data.json --shots "Z:\\maa\\test\\readmeshot"
   ```
   只更新部分页面时输出到临时目录，核对内容后仅拷贝目标页到 `docs/.vuepress/public/images/<lang>/readme/`；全部重截可让 `--shots` 直接指向 `docs/.vuepress/public/images`。
4. **压缩**：`pre-commit run oxipng --files <图片路径>`（参数 `-q -o 2 -s --ng`）。钩子从源码编译 oxipng，本机可能因缺 dlltool 编译失败；此时用 pre-commit 缓存里的同版本产物手动等价跑：
   ```bash
   C:/Users/uyee/.cache/pre-commit/repovrlh11g5/target/release/oxipng.exe -q -o 2 -s --ng <图片...>
   ```
5. **核对**：用读图确认新截图内容符合预期（新功能入镜、无占位名、明暗主题正确）后再覆盖；覆盖后 `git status` 确认只有目标页变更。

## 演示数据

数据字段（任务列表、仓库/干员识别数据、日志、自动战斗页等）与界面部位的对应关系见 `tools/ReadmeShotDemo/README.md`，改数据以其为准。要点：

- 所有 `text` 字段必须包含全部 5 种语言；材料/干员/关卡等名称由 GUI 查表本地化，不受语言影响。
- `operBox` 缺省时自动生成全干员满练度数据，无需手工维护名单。
- 干员识别头像模式由配置基线控制（数据字段不覆盖）；理智作战 ｢指定材料｣ 下拉显示用 `taskQueue.specifiedDrops`（材料 itemId，如 `30012` 固源岩），仅设置显示、不勾选。
- `windowTitle.version` 填 `latest` 联网取最新 Release；新功能先于发版入镜时临时固定为对应版本号（如 `v6.19.0`），**截图完成后还原为 `latest`**。

## 常见错误

- ❌ 改 `readme-demo-data.json` 用 python `json.load`+`dump` 全量重写（整文件缩进重排）；用文本手术只碰目标行。
- ❌ 只更新 zh-cn 图，docs 其余语言 `readme.md` 引用的图停留在旧 UI。
- ❌ 忘记还原临时固定的 `windowTitle.version`。
- ❌ 构建过期就直接跑（截图缺最新 UI）。
- ❌ 演示数据文案只改 zh-cn（其余语言截图显示回退的中文）。
