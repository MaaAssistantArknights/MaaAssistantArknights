# 启动、配置与更新故障

## MAA.exe 双击无反应、闪退或弹出 ｢喜报｣

1. 先检查是否为 64 位 Windows 10/11、匹配架构的完整 ZIP，并确认已经完整解压到独立可写目录。只移动了 `MAA.exe` 时，把它还原到完整目录，桌面用快捷方式。压缩软件的预览运行会将程序放在临时目录。
2. 若提示 ｢MaaCore.dll 加载失败｣，先确认完整包里的 DLL 都在。缺少或过旧的 Visual C++ 运行库按 FAQ 安装或修复。正式 ZIP 已自带 .NET；若有 Windows N/KN，按 FAQ 检查媒体功能包。若无 `gui.log`，保留弹窗全文、Windows 应用程序事件或可靠性记录。原生宿主会在 WPF 日志初始化前检查 `MAA.dll`、`hostfxr.dll` 等文件。
3. 出现 ｢安装文件缺失｣ 时，启动检查依据 `filelist.txt`。核对安全软件隔离记录和缺失文件，确认来源可信后可用界面的完整包修复，或重新解压完整包。若提示 ｢资源损坏，无法开始任务｣，按启动提示修复，成功标准是重启后资源加载并能开始任务。
4. ｢悲报｣ 若列出第三方注入 DLL，按 FAQ 用 Windows 攻击防护对 `MAA.exe` 设置 ｢禁用扩展点｣，或处理点名的软件。只有普通崩溃提示时先看实际异常和堆栈。
5. 提示同一路径只能启动一个实例时，检查托盘或任务管理器的已有进程。权限错误先检查安装位置和文件占用。日常启动通常用普通权限，仅在具体操作需要权限时单独处理。

依据：[常见问题](https://docs.maa.plus/zh-cn/manual/faq.html) · [原生启动宿主](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaAppHostStub/MaaAppHostStub.cpp) · [文件清单检查](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Services/ResourceIntegrityChecker.cs)。

## 设置不保存、丢失或提示 ｢配置异常｣

确认当前启动目录和 ｢设置 → 切换配置｣ 中的当前配置。重启后部分开关还原，先检查是否是右键半选、标注 ｢仅一次｣ 的控件，或启动参数 `--config` 选了另一套配置。若日志显示写入失败，优先解决目录写权限或占用。若日志显示解析失败，先退出并备份整个 `config`，再比较 `gui.new.json`、`.bak`、`.err`；正常启动也会刷新备份，先留副本再试。旧版可能使用 `gui.json`。迁移后验证当前配置、连接和任务设置。

依据：[其他功能](https://docs.maa.plus/zh-cn/manual/introduction/others.html) · [配置恢复和备份](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Configuration/Factory/ConfigFactory.cs)。

## 更新失败、反复下载重启、更新后缺文件

| 阶段与信号 | 下一步 |
| --- | --- |
| 查不到版本或下载失败 | 分清 ｢软件更新｣、｢资源更新｣ 和主界面导航热更。记录渠道、更新源、完整错误和版本。若报认证、网络、限流，先核对对应通路；用户自行检查 CDK 或 Token。 |
| ZIP 拖不进去或提示不支持 | 以普通权限启动 MAA，直接拖官方原始 ZIP。软件完整包和 OTA 必须匹配架构，OTA 起始版本还必须等于当前版本。资源 ZIP 保留 GitHub 下载原结构。Mirror酱自动更新包用其对应更新通路。 |
| 下载完成，重启后仍在旧版 | 读 `debug/gui.log` 中的 `Failed to apply pending update package`、`Pending update package could not be applied` 和相邻日志；若调用外部更新器，再看 `debug/pending-update-applier.log`。查 `NewVersionExtract`、拒绝访问、文件占用、缺失 `MAA.Updater.exe` 或包损坏。确认哪个步骤失败后再处理对应的占用或目录。 |
| 提示上次更新失败、当前安装可能不完整 | 用界面的完整包修复；若失败或打不开，从官方重新下载匹配架构的完整包解压到新目录。关闭旧 MAA 后备份并迁移 `config`、`data`、`debug`，`cache` 与 `achievement` 按需保留。恢复模式会拒绝 OTA，可接受同版本完整包。新目录验证可启动、界面/核心/资源版本正常并成功运行任务后再处理旧目录。 |
| 只有新关卡、图标或入口没有出现 | 在 ｢更新设置｣ 核对资源日期，等待当前任务结束让资源重载。还要确认区服、活动开放时间，以及该功能需要软件更新、资源更新还是导航热更。 |

历史案例 [#17097](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17097) 的下载已成功，失败发生在重启后准备 `NewVersionExtract` 时，日志中有 `Access ... is denied`。同样的表面循环也可能来自别的阶段；只有实际日志相符时才按目录权限或占用处理。

依据：[更新手册](https://docs.maa.plus/zh-cn/manual/introduction/others.html#更新) · [常见问题](https://docs.maa.plus/zh-cn/manual/faq.html) · [WPF 更新应用](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Services/PendingUpdateApplier.cs) · [外部更新器](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaUpdater/main.cpp)。
