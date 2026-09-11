---
name: core-image-eval
description: 用 MaaCore 本体对本地图片（用户日志反馈包截图、自己截的图）离线验证识别结果：pipeline 任务为什么没命中、模板匹配得分多少、OCR 认出了什么、物品模板匹配情况。用户提到 ｢验证识别｣｢为什么没识别/没命中｣｢模板分数/得分｣｢OCR 结果对不对｣｢排查反馈包截图｣或给了截图要复现 core 行为时使用。与外部脚本（python 直调 ppocr/OpenCV）的区别：本工具走 core 完整链路（前处理、ocrReplace、mask、各服 OCR 模型），结果与运行时一致。
---

# MaaCore 本地图片离线评估

工具是 `tools/maa_core_eval.py`（自包含 ctypes 模块，可 import 也可 CLI）。core 侧入口为 `DebugTask` 参数化（`AsstAppendTask(handle, "Debug", params)`），仅 **Debug 构建** 的 MaaCore.dll 可用。

## 前置条件

- MaaCore Debug 构建：默认取 `build/bin/Debug/MaaCore.dll`，没有则 `cmake --build build --target MaaCore --config Debug`。
- 资源根默认仓库根（`AsstLoadResource` 语义：其下找 `resource/`）；国际服 `--global YoStarJP`（分服 OCR 模型随之加载）。
- Debug 构建日志会镜像 stdout，过滤干扰行用 `grep -v "^\[2026"`；完整日志在 user_dir 的 `debug/asst.log`。

## CLI 用法（人工快速验证）

```bash
# 任务命中评估（hit/score/box/OCR 文本）
python tools/maa_core_eval.py --mode report --tasks "TaskA,TaskB" 图.png
# OCR 原始识别文本（不套 ocrReplace/expected）
python tools/maa_core_eval.py --mode ocr [--roi x,y,w,h] 图.png
# 首命中 + next 列表（线上 find_first 同款）
python tools/maa_core_eval.py --mode pipeline --tasks "TaskA" 图.png
# 图片序列按 next 链推进（纯识别，不执行 action）
python tools/maa_core_eval.py --mode replay --tasks "TaskA" 1.png 2.png 3.png
# 裸模板匹配（物品图标等非任务模板；支持目录全量）
python tools/maa_core_eval.py --mode templ --templates "2001,items/xxx.png,items" 图.png
# 仓库物品匹配（复刻线上 DepotImageAnalyzer 预处理）
python tools/maa_core_eval.py --mode depot [--templates "2001,..."] 图.png
```

## Agent 编程用法（写脚本组合调用）

```python
import sys; sys.path.insert(0, "tools")
from maa_core_eval import CoreEval

ev = CoreEval(global_client="YoStarJP")   # 国服默认 CoreEval()
ev.report(images=["1.png"], tasks=["TaskA"])
ev.pipeline(images=["1.png"], tasks=["A", "B"])   # 返回含 next，可自行驱动链
ev.ocr(images=["1.png"], roi=[100, 200, 300, 50])
ev.templ(images=["1.png"], templates=["2001"], task="DepotMatchData", resize=[1066, 599])
ev.replay(images=["1.png", "2.png"], tasks=["A"])
ev.close()
```

返回均为 list[dict]，字段见各方法 docstring。

## 关键语义

- **report**：JustReturn 任务恒命中（`score=0`、`rect=[0,0,0,0]` 是其特征不是 bug）；未命中的任务只报 miss，最高分等细节看 `asst.log` 的 `match_templ` trace。
- **templ**：内部阈值放开，恒报最佳得分，hit 由 `threshold` 判定；`task` 参数让 Matcher 的 maskRange/colorScales/method 取自该任务（复刻线上自定义识别器的关键），`resize` 在 core 侧 INTER_AREA 缩放（数值敏感预处理别在 python 做）。
- **depot**：`depot_items` 的复刻要点是模板右下 80x50 涂黑 + `resize` 到 `DepotMatchData` 的 roi + `task="DepotMatchData"`（其 maskRange 排除数量角标）；数量识别不在复刻内。
- 图片自动 INTER_AREA 归一到 1280x720（与线上截图缩放一致），识别不受历史 rect 缓存污染。
- 连着模拟器要在线验证任务流（含点击 action）用 Custom 任务，本工具只做离线图片评估。

## 排查套路

1. ｢任务为什么没命中｣：report 该任务 → miss 时看 `asst.log` 里该模板的 `match_templ` 行（score 距阈值差多少）。
2. ｢OCR 为什么没匹配 expected｣：`ocr` 看引擎原文，`report` 看经过 ocrReplace/expected 过滤后的结果，两者对比定位是识别问题还是替换表问题。
3. ｢物品图标认成别的｣：`depot`（或 templ 指定多个候选物品）看各候选的分数差。
