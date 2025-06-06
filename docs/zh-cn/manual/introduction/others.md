---
order: 11
icon: icon-park-solid:other
---

# 其他

## GPU 加速推理

使用 DirectML 调用 GPU 进行识别推理加速<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9236)</sup>，可通过少量的 GPU 占用降低大量的 CPU 占用，推荐启用。

经测试，部分显卡因缺少功能或性能较低，在使用本功能时会出现识别问题。MAA 已内置了部分 GPU 黑名单<sup>[PR1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9990)[PR2](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12134)</sup>，若列表外的显卡在启用本功能后也出现识别问题，请发 Issue。

## 仅一次

主界面和设置中的配置更改通常会自动保存，但以下几种会在 MAA 重启后重置。

- 标有 `*` 号的选项
- 标有 `（仅一次）` 的选项
- 通过右键单击复选框得到的半选开关

## 启动时自动切换配置

MAA 支持通过启动参数自动切换配置，在 MAA 进程名后附加 `--config <配置名>` 即可。例子：`./MAA/MAA.exe --config 官服`。

部分符号需要转义，参考 Json。例子：在配置名为 `"官服"` 时，参数应为 `--config \"官服\"`。

## 开始前/结束后脚本

v4.13.0 后支持设置开始前/结束后脚本，可在任务前后自动执行批处理文件。输入框内需填写批处理文件即 `*.bat` 的绝对或相对路径。

## 配置迁移

在 Windows 版本中，MAA 的所有配置都存放在 `config` 文件夹中的 `gui.json` 里。迁移此文件夹即可迁移 MAA 的所有设置。

## 其他说明

- 首页左侧任务可以拖动改变顺序，基建换班设置中设施顺序同理。
- 所有点击操作，都是点击按钮内随机位置，并模拟泊松分布（按钮中心的点击概率最高，距离中心越远，点击概率越低）。
- 底层算法纯 C++ 开发，并设计了多重缓存技术，最大限度降低 CPU 和内存占用。
- 软件支持自动更新 ✿✿ ヽ(°▽°)ノ ✿ ，推荐非杠精的同学使用公测版，一般来说更新快且 bug 少。（什么 MIUI (╯‵□′)╯︵┻━┻
- 如果新版本自动下载失败，可手动下载 OTA 压缩包后直接放到 MAA 目录下，会自动更新的。
