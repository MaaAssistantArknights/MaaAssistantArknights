---
icon: ph:question-fill
---

# 常见问题

::: warning
MAA 在 5.0 版本更新到了 .NET 8，对于最终用户来说，影响如下：

1. MAA 现在需要 .NET 8 运行库，在启动时会自动提示用户安装。若安装失败，请阅读下文，下载安装包手动安装。
2. MAA 不会再被 Windows Defender 误报了。~~为了这碟醋包的饺子~~
3. [.NET 8 不支持 Windows 7/8/8.1 系统](https://github.com/dotnet/core/issues/7556)，所以 MAA 也同样不再支持，即使它现在依旧能正常运行。
4. 在 Windows 7 上运行 MAA 时，会出现内存占用异常的问题，请参阅下文 Windows 7 部分实施缓解措施。Windows 8/8.1 未经测试，若存在相同问题，请顺手发个 Issue 提醒我们补充文档。
:::

## 软件无法运行/闪退/报错

### 可能性 0 : 下载的压缩包不完整

完整 MAA 软件压缩包命名格式为 "MAA-`版本`-`平台`-`架构`.zip"，其余均为无法单独使用的“零部件”，请仔细阅读。
若在某次自动更新后无法使用或缺失功能，可能是自动更新出现了问题, 请尝试重新下载并解压完整包后手动迁移 `config` 文件夹。

### 可能性 1 : 架构错误

在大部分情况下，您需要使用 x64 架构的 MAA，即您需要下载 `MAA-*-win-x64.zip`，而非 `MAA-*-win-arm64.zip`。MAA 不会支持 32 位操作系统。

### 可能性 2 : 运行库问题

::: info 注意
此处仅列出官方安装方法，我们无法保证第三方整合包的可靠性。
:::

- 请安装 [VCRedist x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) 和 [.NET 8](https://download.visualstudio.microsoft.com/download/pr/84ba33d4-4407-4572-9bfa-414d26e7c67c/bb81f8c9e6c9ee1ca547396f6e71b65f/windowsdesktop-runtime-8.0.2-win-x64.exe) 并重新启动计算机后再次运行 MAA。  
  使用 Windows 10 或 11 的用户也可以使用 winget 工具进行安装，只需在终端中运行以下命令。
  
  ```bash
  winget install Microsoft.VCRedist.2015+.x64
  winget install Microsoft.DotNet.DesktopRuntime.8
  ```

#### Windows N/KN 相关

对于 Windows 8/8.1/10/11 N/KN（欧洲/韩国）版本，您还需要安装[媒体功能包](https://support.microsoft.com/zh-cn/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a)。

#### Windows 7 相关

对于 Windows 7，在安装上文提到的运行库之前，您还需要检查以下补丁是否已安装：

  1. [Windows 7 Service Pack 1](https://support.microsoft.com/zh-cn/windows/b3da2c0f-cdb6-0572-8596-bab972897f61)
  2. SHA-2 代码签名补丁：
     - KB4474419：[下载链接 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)、[下载链接 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)
     - KB4490628：[下载链接 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)、[下载链接 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)
  3. Platform Update for Windows 7（DXGI 1.2、Direct3D 11.1，KB2670838）：[下载链接 1](https://catalog.s.download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)、[下载链接 2](http://download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)

.NET 8 应用在 Windows 7 上运行异常的缓解措施 [#8238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8238)

  1. 打开 `计算机`，右键空白处，点击属性，点击左侧 `高级系统设置`，点击 `环境变量`。
  2. 新建一个系统变量，变量名 `DOTNET_EnableWriteXorExecute`，变量值 `0`。
  3. 重启电脑。

我们无法保证将来的版本对 Windows 7 的兼容性，~~都是微软的错~~。

#### 官方整合包（确信）

::: info 注意
此项操作将会耗费 10GB 左右的磁盘空间，请务必在排查完其他可能性后使用
:::

安装 [Microsoft C++ 生成工具](https://visualstudio.microsoft.com/zh-hans/visual-cpp-build-tools/) 进行完整的开发环境配置（仅需要安装 .NET 及 C++ 开发环境）。

### 可能性 3 : 系统组件问题

以上运行库安装均需要依赖组件存储服务（CBS、TrustedInstaller/TiWorker、WinSxS）。如果组件存储服务被破坏，将不能正常安装

我们无法提供除重装系统以外的修复建议，请避免使用未标明精简项及精简风险的“精简版”系统。

## 连接错误

::: tip
请参阅 [模拟器支持](./模拟器和设备支持) 确定正在使用的模拟器通过了支持性测试
:::

### 确认 adb 及连接地址正确

- 确认 MAA `设置` - `连接设置` - `adb 路径` 是否已自动填写，若已填写请忽略这步

::: details 若未填写
- 找到模拟器安装路径，Windows 可在运行模拟器时在任务管理器中右键进程点击 `打开文件所在的位置`。
- 顶层或下层目录中大概率会有一个 `adb.exe`（不一定就叫这个名字，可能叫 `nox_adb.exe`；`HD-adb.exe`；`adb_server.exe` 等等，总之是名字带`adb` 的 exe），搜索它，选择它！
:::

- 确认连接地址填写正确。可在网上搜索正在使用的模拟器 adb 调试地址是什么，一般是类似 `127.0.0.1:5555` 这样的格式（雷电模拟器除外）。

#### 模拟器调试端口

相关文档及参考端口：

- [Bluestacks 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631-%E5%A6%82%E4%BD%95%E5%B0%87%E6%82%A8%E7%9A%84%E6%87%89%E7%94%A8%E5%BE%9EBlueStacks-4%E8%BD%89%E7%A7%BB%E5%88%B0BlueStacks-5#%E2%80%9C2%E2%80%9D) `5555`
- [MuMu Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html) `16384`
- [MuMu 12](https://mumu.163.com/help/20230214/35047_1073151.html) `16384`
- [MuMu 6](https://mumu.163.com/help/20210531/35047_951108.html) `7555`
- [逍遥](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537) `21503`
- [夜神](https://support.yeshen.com/zh-CN/qt/ml) `62001`

其他模拟器可参考 [赵青青的博客](https://www.cnblogs.com/zhaoqingqing/p/15238464.html)。

### 关闭现有 adb 进程

关闭 MAA 后查找 `任务管理器` - `详细信息` 中有无名称包含 `adb` 的进程（通常和上文中填写的 `adb` 文件同名），如有，结束它后重试连接。

### 正确使用多个 adb

当 adb 版本不同时，新启动的进程会关闭旧的进程。因此在同时运行多个 adb，如 Android Studio、Alas、手机助手时，请确认它们的版本相同。

### 避免游戏加速器

部分加速器在启动加速和停止加速之后，都需要重启 MAA、ADB 和模拟器再连接。

同时使用 UU 加速器 和 MuMu 12 可以参考[官方文档](https://mumu.163.com/help/20240321/35047_1144608.html)。

### 重启电脑

重启能解决 97% 的问题。（确信

### 换模拟器

请参阅 [模拟器支持](./模拟器和设备支持)。

## 连接正常，但是无操作

小部分模拟器自带的 `adb` 版本过于老旧，不支持 `Minitouch` 相关操作。

请使用管理员权限打开 MAA，点击 `MAA 设置` - `连接设置` - `强制替换 ADB`。（建议关闭模拟器并重启 MAA 后再操作，否则可能替换失败）

模拟器更新后可能会重新覆盖 adb 文件。若更新后问题复现，请再次尝试替换。

如果即使这样也无法正常使用，可将 `连接设置` - `触控模式` 从 `Minitouch` 切换到 `MaaTouch` 再次尝试。由于 `Adb Input` 操作过于缓慢，请仅将其作为万不得已的模式。

## 蓝叠模拟器每次启动端口号都不一样（Hyper-V）

打开 MAA，在 `设置` - `连接设置` 中设置 `连接配置` 为 `蓝叠模拟器` ，随后勾选 `自动检测连接` 和 `每次重新检测`（或是在主界面 `开始唤醒` 旁的设置中勾选这两项）。

通常情况下这样就可以连接。如果无法连接，可能是存在多个模拟器核心或出现了问题，请阅读下文进行额外设置。

### 指定 `Bluestacks.Config.Keyword`

::: info 注意
如果启用了多开功能或安装了多个模拟器核心，则需要进行额外设置来指定使用的模拟器编号
:::

在 `.\config\gui.json` 中搜索 `Bluestacks.Config.Keyword` 字段，内容为 `"bst.instance.<模拟器编号>.status.adb_port"`，模拟器编号可在模拟器路径的 `BlueStacks_nxt\Engine` 中看到

::: details 示例
Nougat64 核心：

```json
"Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
```

Pie64_2 核心：（核心名称后的数字代表这是一个多开核心）

```json
"Bluestacks.Config.Keyword": "bst.instance.Pie64_2.status.adb_port",
```
:::

### 指定 `Bluestacks.Config.Path`

::: info 注意
MAA 现在会尝试从注册表中读取 `bluestacks.conf` 的存储位置，当该功能无法工作时，则需要手动指定配置文件路径
:::

1. 在蓝叠模拟器的数据目录下找到 `bluestacks.conf` 这个文件

   - 国际版默认路径为 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`
   - 中国内地版默认路径为 `C:\ProgramData\BlueStacks_nxt_cn\bluestacks.conf`

2. 如果是第一次使用，请运行一次 MAA，使 MAA 自动生成配置文件。

3. **先关闭** MAA，**然后**打开 `gui.json`，找到 `Configurations` 下的当前配置名字段（可在 设置-切换配置 中查看，默认为 `Default`），在其中搜索字段 `Bluestacks.Config.Path`，填入 `bluestacks.conf` 的完整路径。（注意斜杠要用转义 `\\`）

::: details 示例
以 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf` 为例

```json
{
  "Configurations": {
    "Default": {
      "Bluestacks.Config.Path": "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf"
      // 其余配置字段，不要手动输入修改
    }
  }
}
```

:::

## 连接正常，但是操作卡顿、异常或频繁出错

- 若使用了 `异形屏UI适配`，请将其调整为 0。
- 若正在游玩非国服客户端，请先在 `设置` - `游戏设置` - `客户端类型` 中选择客户端版本。非国服部分功能可能并非完全适配，请参阅对应的外服使用文档。
- 若正在进行自动肉鸽，请参阅[详细介绍](./详细介绍.md#一键长草-自动肉鸽)，并在 `自动肉鸽` - `肉鸽主题` 中正确选择主题。
- `Adb Input` 触控模式操作缓慢为正常情况，如需自动战斗等请尝试切换其他模式。

### 提示截图用时较长 / 过长

- MAA 目前支持 `RawByNc` 、 `RawWithGzip` 、 `Encode` 三种截图方式，当执行任务平均截图耗时 >400 / >800 时会输出一次提示信息（单次任务只会输出一次）
- `设置 - 连接设置` 中会显示近30次截图耗时的 最小/平均/最大值，每10次截图刷新
- 自动战斗类功能（如自动肉鸽）受截图耗时影响较大
- 此项耗时与MAA无关，与电脑性能、当前占用或模拟器相关，可尝试清理后台/更换模拟器/升级电脑配置

## 文件下载速度慢

请尝试使用 [MAA 下载站](https://ota.maa.plus/MaaAssistantArknights/MaaRelease/releases/download/) 下载。

我们的小水管带宽较低，且流量有限。虽然搭出来就是给大家用的，但也希望不要滥用，若 Github 或其他镜像站可正常下载，建议优先选择。

## 下载到一半提示“登陆”/“鉴权”

请使用 浏览器 / IDM / FDM 等下载器下载文件，**不要用↑↓迅雷！**
