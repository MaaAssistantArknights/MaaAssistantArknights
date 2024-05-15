---
icon: mdi:plug
---

# 自定义连接

- 使用 [模拟器内置 adb](./faq.md#关闭现有-adb-进程) 或自行下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) 并解压。
- 进入软件 `设置` - `连接设置`，选择 `adb` 的文件路径，填写 adb 地址（需要填写 IP + 端口，例如 `127.0.0.1:5555`），并选择模拟器类型。

## 选择 adb 路径

找到模拟器安装路径，Windows 可在运行模拟器时在任务管理器中右键进程点击 `打开文件所在的位置`。

顶层或下层目录中大概率会有一个 `adb.exe`（不一定就叫这个名字，可能叫 `nox_adb.exe`；`HD-adb.exe`；`adb_server.exe` 等等，总之是名字带`adb` 的 exe），搜索它，选择它！

## 获取端口号

模拟器相关文档及参考端口：

- [Bluestacks 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631-%E5%A6%82%E4%BD%95%E5%B0%87%E6%82%A8%E7%9A%84%E6%87%89%E7%94%A8%E5%BE%9EBlueStacks-4%E8%BD%89%E7%A7%BB%E5%88%B0BlueStacks-5#%E2%80%9C2%E2%80%9D) `5555`
- [MuMu Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html) `16384`
- [MuMu 12](https://mumu.163.com/help/20230214/35047_1073151.html) `16384`
- [MuMu 6](https://mumu.163.com/help/20210531/35047_951108.html) `7555`
- [逍遥](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537) `21503`
- [夜神](https://support.yeshen.com/zh-CN/qt/ml) `62001`

其他模拟器可参考 [赵青青的博客](https://www.cnblogs.com/zhaoqingqing/p/15238464.html)。

- 方案 1 : 使用 adb 命令查看模拟器端口

  1. 启动**一个**模拟器，并确认没有其他安卓设备连接在此计算机上。
  2. 在存放有 adb 可执行文件的文件夹中启动命令窗口。
  3. 执行以下命令。

  ```sh
  # Windows 命令提示符
  adb devices
  # Windows PowerShell
  .\adb devices
  ```

  以下为输出内容的例子：

  ```sh
  List of devices attached
  127.0.0.1:<端口>   device
  ```

  使用 `127.0.0.1:<端口>` 作为连接地址。若输出 `emulator-****` 请参阅方案 2。

- 方案 2 : 查找已建立的 adb 连接

  1. 执行方案 1。
  2. 按 `徽标键+S` 打开搜索栏，输入 `资源监视器` 并打开。
  3. 切换到 `网络` 选项卡，在 `侦听端口` 的名称列中查找模拟器进程名，如 `HD-Player.exe`。
  4. 记录模拟器进程的所有侦听端口。
  5. 在 `TCP 连接` 的名称列中查找 `adb.exe`，在远程端口列中与模拟器侦听端口一致的端口即为模拟器调试端口。

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