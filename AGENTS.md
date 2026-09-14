# MAA 开发约定

格式由 `.pre-commit-config.yaml` 钩子机器强制，各钩子有 files 范围；本地不处理好的话 GitHub CI 会定时代跑并生成额外的机器修复 commit，为避免此副作用，改完文件后须本地跑对应格式化（如 C++ 的 clang-formatter）。

## C++（MaaCore）

- 批量格式化用 `python tools/ClangFormatter/clang-formatter.py --input=src/MaaCore`，版本需求：clang-format 20.0.0；5 位以上整数字面量加千分位撇（`65'535`，存量尚有漏改，勿照抄）。
- 日志用 `"Utils/Logger.hpp"`，日志走 `<<` 流式格式，正文一律英文。旧代码中存在使用函数式日志的情况，勿照抄。合理使用 `__FUNCTION__`、`LogTraceFunction` 标注当前函数，避免手动在日志中标记。避免匿名命名空间，会导致在日志中输出为 `<anonymous namespace>`，不利于定位。
- 字符串一律 UTF-8 `std::string`，中文裸写、禁 `u8""` 前缀；文件路径必须经 `asst::utils::path()` 或 `"xxx"_p` 字面量构造，仅 Win32 API 边界转 `to_osstring`。
- 错误处理返回 `bool` / `std::optional` / 空指针并记日志，常规任务与配置链路不用异常。
- 避免返回裸指针，如有需要，使用引用或智能指针；引用返回值须保证生命周期，禁止返回局部变量引用。
- Json 序列化用 `meojson`，`#include <meojson/json.hpp>`。简单结构可考虑使用 `MEO_JSONIZATION` 宏生成序列化函数，如有复杂需求则需手写 `to_json` / `check_json` / `from_json`。检查 Json 字段、类型、范围、枚举值等，应在 `check_json` 中完成，`from_json` 返回 false 则会直接抛出异常，目前无返回 false 的情况。meojson 支持枚举的忽略大小写反序列化，但是如有 `_` 分隔符，则需要手动编写 `json::_reflection::enum_name_storage` 进行映射。

## C# / WPF（MaaWpfGui）

- 新 `.cs` 文件必须带 `stylecop.json` 模板规定的版权头，`file=` 值须等于实际文件名。
- MVVM 框架是 Stylet：对话框 VM 继承 `Screen`，Root 用 `Conductor<Screen>.Collection.OneActive`，UserControl 子模型与列表项继承 `PropertyChangedBase`，配置树类继承 `NotifyPropertyChangedWithValue`；交互用 `Command="{s:Action 方法名}"` 直绑 VM 方法，不自建 ICommand。
- 派生属性标 `[PropertyDependsOn(nameof(X))]` 并在构造函数调 `PropertyDependsOnUtility.InitializePropertyDependencies(this)`。
- `MaaWpfGui.Configuration.` 命名空间内的类用纯 auto-property（Fody PropertyChanged 织入通知），命名空间外织入不生效，必须手动 `SetAndNotify`，否则 UI 静默不刷新。
- 配置持久化与迁移归口 `ConfigFactory`（全局 `Root.Gui.X`、当前档案 `CurrentConfig.X`，保存自动防抖）；GUI 设置值的读取一律走 `SettingsViewModel.XxxSettings.X`（设置 VM 属性与配置双向同步，static 属性类型名限定），直取 `CurrentConfig` 仅限 `TaskQueue` 任务对象（多任务间切换当前设置需直读配置，其余设置一律走设置 VM）；新任务类型须在 `BaseTask` 注册 `[JsonDerivedType(typeof(XxxTask), typeDiscriminator: nameof(XxxTask))]`。
- 服务在 Bootstrapper 用 StyletIoC 显式注册，跨对象取用走 `Instances.Xxx` 静态定位器，不做构造函数注入。
- 回 UI 线程用 Stylet 的 `Execute.OnUIThread`（同步）或 `Execute.OnUIThreadAsync`；纯后台链路 await 加 `ConfigureAwait(false)`，后续要碰 UI 的不加。
- 日志用 Serilog `Log.ForContext<T>()`，结构化占位、消息英文；UI 可见日志另走 `Instances.TaskQueueViewModel.AddLog`（Copilot 相关链路用 `Instances.CopilotViewModel.AddLog`）。如有需要，类内声明`private static readonly ILogger _logger = Log.ForContext<T>();`，避免反复 `ForContext`。
- View 与 VM 命名镜像由 Stylet 按约定解析，放错命名空间运行时找不到 View：`XxxView` 对应 `XxxViewModel`，`XxxUserControl` 对应 `XxxUserControlModel`（VM 类名以 Model 结尾）。
- TaskQueue 任务对象类的命名参考为 `XxxTask`，VM 为 `XxxTaskViewModel`，View 为 `XxxTaskView`，UserControl 为 `XxxTaskUserControl`；Settings 配置类的命名参考为 `XxxSettings`，配置 VM 为 `XxxSettingsViewModel`，配置 View 为 `XxxSettingsView`。

## XAML 与本地化

- 界面文本一律用 `controls:TextBlock` 而非原版 `TextBlock`；例外是文本样式需跟随宿主控件模板的场景（超链接、下拉选中项、按钮 Content、DataTemplate 内部等），这些用原版 `TextBlock`，不要套统一样式，保持原生表现。
- 本地化 key 与主题 Brush 一律 `{DynamicResource ...}`，StaticResource 在语言或主题热切换后不刷新；引用本仓库自定义样式等不随主题/语言热切换的资源时用 StaticResource。
- 提示文案 key 加 `Tip` 后缀（`ForceScheduledStart` 与 `ForceScheduledStartTip`），句内复用其他 key 写 `{key=Xxx}` 内联。
- `TooltipBlock` 长文案按句界显式换行，一句一行即可；无句界的长句不必强行拆，由 TooltipText 默认 MaxWidth 500 的软换行兜底。资源串内写 `&#10;` 或字面换行均可，且必须声明 `xml:space="preserve"`，否则换行符被 XAML 归一化；各语言在自己句界处换行，行数不要求一致。
- 固定选项的下拉框用 `LocalizedObservableList<T>`（`src/MaaWpfGui/Utilities/ValueType/LocalizedObservableList.cs`），构造给 (值, 本地化 key)，勿手动 GetString 固化 Display（热切换语言后该项停留旧语言）；ViewModel 订阅 `LocalizationHelper.LanguageChanged` 并在回调里 `RefreshLocalization()`；增删用自带 Add/Remove/Insert/Clear，勿直接操作 Items。
- 设置页新增区块复用 Expander + Border + UserControl 既有结构（样板见 `SettingsView.xaml`），否则破坏搜索过滤与折叠持久化。

## 资源与工程文件

- `resource/tasks/tasks.json` 字段须在 TaskData 白名单内，自定义字段须配 `xxx_Doc` 注释字段，校验只在 `ASST_DEBUG` 加载期（Release 静默）；`next` 列表特殊引用用 `#` 前缀（`#self` / `#next` / `#back`）。
- MaaCore 源文件由 CMake `file(GLOB_RECURSE)` 自动收集，新增 `.h/.cpp` 无需登记 CMakeLists。
- 改 `Res/Localizations/*.xaml` 中 stub 相关文案（如 ErrorCongratulations）须同步 `src/MaaAppHostStub/MaaAppHostStub.cpp`，CI 有硬校验。

## 文档维护

- 改任一语言后同步其余四语言（zh-cn/zh-tw/en-us/ja-jp/ko-kr）的对应段落（行号未必对应）：语义一致即可，某语言本就缺段落则保持现状但提示。
- 文档中的 UI 均指 WPF UI，修改时不必处理 Avalonia（MAAUnified）等社区实现的相关描述；明日方舟 PC 端不进文档。
- 新活动/模式尚无官方译名时以各语言 UI 显示为准（见 `Res/Localizations/*.xaml`），UI 也无译文则写中文并括号加描述，勿自行发明译名。
- docs 新增文档须建五语言同名文件，frontmatter 带 `icon` 与 `order`（侧边栏排序）；站内链接用相对路径，CI 有死链检查。

## 发布 / 打包

- Windows 完整包由 `.github/workflows/ci.yml`（正式）与 `.github/workflows/release-nightly-ota.yml`（Nightly）两条工作流各自独立打包，凡改打包产物或发布流程**必须同时同步两条**；`release-ota.yml` / `release-package-distribution.yml` 只基于已上传产物做差分分发，通常无需随动。
- 禁止裸 `dotnet build -o build\bin\Debug`（不带 -r）：该目录是自包含产物，framework-dependent 的 runtimeconfig 与残留自包含宿主混装会致启动报错；重建 GUI 用 `dotnet publish -r win-x64 --self-contained true` 或 `dotnet build -r win-x64`。`dotnet publish` 不重建 MaaCore.dll，需单独 CMake 编译后放置。
- CHANGELOG 由发版时统一整理，日常改动勿自行添加条目。

## 测试

- MaaCore 本地测试一律 Debug 构建：加载期检查整体在 `ASST_DEBUG` 内，Release 下零错误是假象。
- MSB3026/MSB3021 构建复制失败时，挡构建的是占用仓库构建版 `build\bin\Debug\MAA.exe` 的进程（与安装版无关），临时验证程序引用 MaaWpfGui 时同理。
- 文档站在主仓库 `docs/` 下直接跑，日常验证改完文件看 `pnpm dev` 热重载即可，无必要不跑 build；非交互 install 加 `CI=true`。
