# MAA 开发约定

## TextBlock

界面文本一律用 `controls:TextBlock` 而非原版 `TextBlock`；例外是文本样式需跟随宿主控件模板的场景（超链接、下拉选中项、按钮 Content、DataTemplate 内部等），这些用原版 `TextBlock`，不要套统一样式，保持原生表现。

## 文档维护

- 涉及 UI 按钮、菜单路径、功能名称或行为的描述，先对照实际代码与 `src/MaaWpfGui/Res/Localizations/*.xaml` 核对，勿想当然沿用。
- 改任一语言后同步其余四语言（zh-cn/zh-tw/en-us/ja-jp/ko-kr）的对应段落（行号未必对应）：语义一致即可，某语言本就缺段落则保持现状但提示。
- 影响用户可见行为的改动，同步检查 docs。
- 文档中的 UI 均指 WPF UI，修改时不必处理 Avalonia（MAAUnified）等社区实现的相关描述；明日方舟 PC 端不进文档。
- 新活动/模式尚无官方译名时以各语言 UI 显示为准（见 `Res/Localizations/*.xaml`），UI 也无译文则写中文并括号加描述，勿自行发明译名。

## 发布 / 打包

- Windows 完整包由 `.github/workflows/ci.yml`（正式）与 `.github/workflows/release-nightly-ota.yml`（Nightly）两条工作流各自独立打包，凡改打包产物或发布流程**必须同时同步两条**；`release-ota.yml` / `release-package-distribution.yml` 只基于已上传产物做差分分发，通常无需随动。
- 禁止裸 `dotnet build -o build\bin\Debug`（不带 -r）：该目录是自包含产物，framework-dependent 的 runtimeconfig 与残留自包含宿主混装会致启动报错；重建 GUI 用 `dotnet publish -r win-x64 --self-contained true` 或 `dotnet build -r win-x64`。`dotnet publish` 不重建 MaaCore.dll，需单独 CMake 编译后放置。

## UI 文案

- `TooltipBlock` 长文案按句界显式换行，一句一行即可；无句界的长句不必强行拆，由 TooltipText 默认 MaxWidth 500 的软换行兜底。资源串内写 `&#10;` 或字面换行均可，且必须声明 `xml:space="preserve"`，否则换行符被 XAML 归一化；各语言在自己句界处换行，行数不要求一致。
- 固定选项的下拉框用 `LocalizedObservableList<T>`（`src/MaaWpfGui/Utilities/ValueType/LocalizedObservableList.cs`），构造给 (值, 本地化 key)，勿手动 GetString 固化 Display（热切换语言后该项停留旧语言）；ViewModel 订阅 `LocalizationHelper.LanguageChanged` 并在回调里 `RefreshLocalization()`；增删用自带 Add/Remove/Insert/Clear，勿直接操作 Items。

## CHANGELOG

由发版时统一整理，日常改动勿自行添加条目。

## 测试

- MaaCore 本地测试一律 Debug 构建：加载期检查整体在 `ASST_DEBUG` 内，Release 下零错误是假象。
- MSB3026/MSB3021 构建复制失败时，挡构建的是占用仓库构建版 `build\bin\Debug\MAA.exe` 的进程（与安装版无关），临时验证程序引用 MaaWpfGui 时同理。
- 文档站在主仓库 `docs/` 下直接跑，日常验证改完文件看 `pnpm dev` 热重载即可，无必要不跑 build；非交互 install 加 `CI=true`。
