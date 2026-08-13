# LinuxWindowControllerTest

LinuxWindowController（X11 窗口控制）的验证驱动，通过 `AsstAsyncAttachWindowByName` 绑定窗口并执行截图/点击。

## 编译

```bash
gcc -O2 -o lwtest main.c -I<MAA>/include -L<MAA>/build/bin/Debug -lMaaCore
```

## 运行

```bash
LD_LIBRARY_PATH=<MAA>/build/bin/Debug:<MAA>/src/MaaUtils/MaaDeps/vcpkg/installed/maa-x64-linux/lib \
DISPLAY=:0 ./lwtest <MAA> Arknights [click_x click_y]
```

- `<MAA>`: 仓库根目录（内含 `resource/`）
- 第二个参数为窗口标题（完全匹配），如 `Arknights`
- 可选 `click_x click_y`：以 MAA 内部坐标（1280×720 空间）执行一次点击，用于验证输入链路

输出截图保存到 `/home/poland/maa-work/shots/ctrl_before.png` / `ctrl_after.png`（测试用途，可按需修改路径）。
