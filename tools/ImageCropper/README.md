# 截图工具

本工具可以对 **预先准备好的截图**、**通过 ADB 连接设备** 或 **通过 WGC 截取 PC 窗口**，进行 ROI 区域的截取、保存、取色操作。

## 环境

> 如果虚拟环境路径 venv/ 存在，start.bat 优先使用虚拟环境。

需要 `python` 环境，推荐版本为 `3.11` ，最低版本为 `3.9` 以上。

## 依赖

> [!TIP]
> windows 用户推荐直接运行 install.bat 和 start.bat

```shell
python -m pip install -r requirements.txt
```

## 使用

0. (非必要) 根据 `set_screenshot_target_long/short_side` 的使用情况，调整脚本中的 `截图参数` 和 `初始窗口大小`；截取 PC 窗口时可调整 `window_name`（目标窗口标题关键字）与 `win32_screencap_method`（默认 `Background` = FramePool/WGC，失败自动回退 PrintWindow）
1. 如果有预先准备好的截图，需保存到 `./src/` 路径下
2. 运行 `start.bat` 或 `python main.py [device serial]` ，设备地址为可选
    - 根据提示 `Please select the device (ENTER to pass):` ，选择 adb 已连接设备（按 ENTER 跳过选择）
    - 如果没有该提示，请使用 `python main.py [device serial]` 连接设备
    - **截取 PC 窗口（WGC）**：运行 `python main.py --pc [窗口标题关键字]` ，关键字默认为 `明日方舟`（官方 PC 客户端）。按标题子串匹配可见顶层窗口，自动绑定第一个命中的窗口；无需 ADB，需 Windows 10 1903+
3. 在弹窗中左键选择目标区域，滚轮缩放图片，右键移动图片
4. 使用快捷键操作：
    - 按 <kbd>S</kbd> <kbd>ENTER</kbd> 保存目标区域
    - 按 <kbd>F</kbd> 保存全屏标准化截图
    - 按 <kbd>R</kbd> 不保存，只输出 ROI 范围
    - 按 <kbd>C</kbd> 不保存，输出 ROI 范围和 ColorMatch 的所需字段，大写将使用 connected 字段
    - 按 <kbd>Z</kbd> <kbd>DELETE</kbd> <kbd>BACKSPACE</kbd> 撤销
    - 按 <kbd>0</kbd> ~ <kbd>9</kbd> 缩放窗口
    - 按 <kbd>Q</kbd> <kbd>ESC</kbd> 退出
    - 按 <kbd>任意键</kbd> 跳过 / 刷新当前截图

5. 目标区域截图保存在 `./dst/` 路径下，文件名为 `src` 中的文件名 / 截图的时间 + ROI + 放大后的 ROI
