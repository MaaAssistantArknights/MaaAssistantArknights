# MeoAssistant

辅助的底层C++模块

## 控制

- ~~窗口控制、点击、截图，使用Win32 Api~~ 新版本已全面使用Adb控制
- 部分不响应句柄消息的模拟器，使用Adb控制

## 识别及解析

- 图像识别库：[opencv](https://github.com/opencv/opencv.git)
- ~~文字识别库：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字识别库：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 关卡掉落识别：[企鹅物流识别](https://github.com/KumoSiunaus/penguin-stats-recognize-v3)
- C++ JSON库：[meojson](https://github.com/MistEO/meojson.git)
- C++ 运算符解析器：[calculator](https://github.com/kimwalisch/calculator)
- C++ base64编解码：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)

## 数据

- 公开招募数据：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)
- 干员及基建数据：[PRTS明日方舟中文WIKI](http://prts.wiki/)
- 关卡数据：[企鹅物流数据统计](https://penguin-stats.cn/)

## 开发相关

### Windows

直接使用 Visual 2019 或以上版本打开`MeoAssistantArknights.sln`即可，所有环境都是配置好的

### Linux | macOS

[Linux 编译教程](Linux编译教程.md)