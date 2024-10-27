---
order: 4
icon: ph:question-fill
---

# 常见问题

::: warning

<!-- TODO：这里话说的不通顺 明天再改 -->

出现次数最多的问题都是运行库问题，所以我们把置顶换成了这个。很气。尤其是微软，气爆了

天天追最新版的都是猪！

- 若 MAA 在某次更新后无法运行，也有可能是因运行库版本导致的问题，同样可以尝试再次安装或更新运行库。

- 请安装 [Visual C++ 可再发行程序包](https://aka.ms/vs/17/release/vc_redist.x64.exe) 和 [.NET 桌面运行时 8](https://dotnet.microsoft.com/en-us/download/dotnet/8.0#:~:text=Binaries-,Windows,-x64) 并重新启动计算机后再次运行 MAA。  
  推荐使用 Windows 10 或 11 的用户使用 winget 工具进行安装，只需在终端中运行以下命令。

  ```sh
  winget install Microsoft.VCRedist.2015+.x64 Microsoft.DotNet.DesktopRuntime.8
  ```

:::

## 软件无法运行/闪退/报错

### 下载/安装问题

- 完整 MAA 软件压缩包命名格式为 "MAA-`版本`-`平台`-`架构`.zip"，其余均为无法单独使用的“零部件”，请仔细阅读。
  在大部分情况下，您需要使用 x64 架构的 MAA，即您需要下载 `MAA-*-win-x64.zip`，而非 `MAA-*-win-arm64.zip`。
- 若在某次自动更新后无法使用或缺失功能，可能是自动更新出现了问题, 请尝试重新下载并解压完整包后手动迁移 `config` 文件夹。

### 运行库问题

找到网页右下角的向上 ↑ 箭头，点一下它。

### 系统问题

- MAA 不支持 32 位操作系统。

<!-- TODO：这里格式爆了 语句不通顺 不符合语境 也不需要参阅下文了 明天再改 -->

- MAA 在 5.0 版本更新到了 .NET 8，对于最终用户来说，影响如下：

1. MAA 现在需要 .NET 8 运行库，在启动时会自动提示用户安装。若安装失败，请阅读下文并手动安装。
2. MAA 不会再被 Windows Defender 误报了。~~为了这碟醋包的饺子~~
3. .NET 8 不支持 Windows 7/8/8.1 系统<sup>[源](https://github.com/dotnet/core/issues/7556)</sup>，所以 MAA 也同样不再支持。最后一个可用的 .NET 8 版本为 [`v5.4.0-beta.1.d035.gd2e5001e7`](https://github.com/MaaAssistantArknights/MaaRelease/releases/tag/v5.4.0-beta.1.d035.gd2e5001e7)；最后一个可用的 .NET 4.8 版本为 [`v4.28.8`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/tag/v4.28.8)。尚未确定自行编译的可行性。
4. 在 Windows 7 上运行 .NET 8 应用时，会出现内存占用异常的问题，请参阅[下文](#net-8-应用在-windows-7-上运行异常的缓解措施-8238)实施缓解措施。Windows 8/8.1 未经测试，若存在相同问题，请顺手发个 Issue 提醒我们补充文档。

- 以上运行库安装均需要依赖组件存储服务（CBS、TrustedInstaller/TiWorker、WinSxS）。
  如果组件存储服务被破坏，将不能正常安装。

我们无法提供除重装系统以外的修复建议，请避免使用未标明精简项及精简风险的“精简版”系统，或者万年前的旧版系统。

#### Windows N/KN

对于 Windows N/KN（欧洲/韩国），还需安装[媒体功能包](https://support.microsoft.com/zh-cn/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a)。

#### Windows 7

对于 Windows 7，在安装上文提到的运行库之前，还需检查以下补丁是否已安装：

1. [Windows 7 Service Pack 1](https://support.microsoft.com/zh-cn/windows/b3da2c0f-cdb6-0572-8596-bab972897f61)
2. SHA-2 代码签名补丁：
   - KB4474419：[下载链接 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)、[下载链接 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)
   - KB4490628：[下载链接 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)、[下载链接 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)
3. Platform Update for Windows 7（DXGI 1.2、Direct3D 11.1，KB2670838）：[下载链接 1](https://catalog.s.download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)、[下载链接 2](http://download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)

##### .NET 8 应用在 Windows 7 上运行异常的缓解措施 [#8238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8238)

1. 打开 `计算机`，右键空白处，点击属性，点击左侧 `高级系统设置`，点击 `环境变量`。
2. 新建一个系统变量，变量名 `DOTNET_EnableWriteXorExecute`，变量值 `0`。
3. 重启电脑。

我们无法保证将来的版本对 Windows 7 的兼容性，~~都是微软的错~~。

## 连接错误

### 确认 ADB 及连接地址正确

参阅 [连接设置](./connection.md)。

### 关闭现有 ADB 进程

关闭 MAA 后查找 `任务管理器` - `详细信息` 中有无名称包含 `adb` 的进程，如有，结束它后重试连接。

### 正确使用多个 ADB

当 ADB 版本不同时，新启动的进程会关闭旧的进程。因此在需要同时运行多个 ADB，如 Android Studio、Alas、手机助手时，请确认它们的版本相同。

### 避免游戏加速器

部分加速器在启动加速和停止加速之后，都需要重启 MAA、ADB 和模拟器再连接。

同时使用 UU 加速器 和 MuMu 12 可以参考[官方文档](https://mumu.163.com/help/20240321/35047_1144608.html)。

### 重启电脑

重启能解决 97% 的问题。（确信

### 换模拟器

请参阅 [模拟器和设备支持](./device/)。

## 连接正常，但是无操作

小部分模拟器自带的 ADB 版本过于老旧，不支持 `Minitouch`、`MaaTouch`。

请使用管理员权限打开 MAA，关闭模拟器并重启 MAA 后点击 `MAA 设置` - `连接设置` - `强制替换 ADB`。

模拟器更新后可能会重新覆盖 ADB 文件。若更新后问题复现，请再次尝试替换，或使用 [其他 ADB](./connection.md#使用谷歌提供的-adb)。

## 连接正常，但是操作卡顿、异常或频繁出错

- 若使用了 `异形屏UI适配`，请将其调整为 0。
- 若正在游玩非国服客户端，请先在 `设置` - `游戏设置` - `客户端类型` 中选择客户端版本。非国服部分功能可能并非完全适配，请参阅对应的外服使用文档。
- 若正在进行自动肉鸽，请参阅 [自动肉鸽文档](./introduction/integrated-strategy.md)，并在 `自动肉鸽` - `肉鸽主题` 中正确选择主题。
- `Adb Input` 触控模式操作缓慢为正常情况，如需自动战斗等请尝试切换其他模式。

### 提示截图用时较长 / 过长

- MAA 目前支持 `RawByNc` 、 `RawWithGzip` 、 `Encode` 三种基于 ADB 的截图方式，当执行任务平均截图耗时 >400 / >800 时会输出一次提示信息（单次任务只会输出一次）。
- `设置 - 连接设置` 中会显示近 30 次截图耗时的 最小/平均/最大值，每 10 次截图刷新。
- 自动战斗类功能（如自动肉鸽）受截图耗时影响较大。
- 此项耗时与 MAA 无关，与电脑性能、当前占用或模拟器相关，可尝试清理后台/更换模拟器/升级电脑配置。

## 文件下载速度慢

[官网](https://maa.plus)会自动选择镜像源，若单次下载慢可以尝试刷新网页重新选择，或使用 [MAA 下载站](https://ota.maa.plus/MaaAssistantArknights/MaaRelease/releases/download/) 下载。

我们的小水管带宽较低，且流量有限。虽然搭出来就是给大家用的，但也希望不要滥用，若 Github 或其他镜像站可正常下载，建议优先选择。

## 下载到一半提示“登陆”/“鉴权”

请使用 浏览器 / IDM / FDM 等下载器下载文件，**不要用 ↑↓ 迅雷！**
