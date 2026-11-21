---
order: 9
icon: mdi:expansion-card
---

# OpenVINO 加速后端（实验性）

MAA 支持 Intel OpenVINO 作为可选推理后端，可加速 ONNX 会话与 OCR。仅支持 Windows / Linux x64。

## 模式

- CPU：OCR 与 ONNX 会话均使用 OpenVINO CPU
- NPU：OCR 与 ONNX 会话均使用 Intel NPU，需要可用的 NPU 驱动
- Hybrid（CPU+NPU）：OCR 使用 OpenVINO CPU，ONNX 会话使用 NPU。NPU 只加速 ONNX 会话，并非全链路加速

## 启用

在 ｢设置 - 性能设置｣ 中将推理后端选择为 OpenVINO，再选择设备。启动时会探测设备能力；所选后端不可用时回退到仍可用的推理后端。

## 依赖与加速包

主发布包不包含 OpenVINO 运行时，需要额外的加速包：

1. 使用 MaaDeps 的 OpenVINO 可选构建（`--openvino`）生成运行时
2. 使用 MaaDeps 仓库中的 `openvino-pack/package.sh` 打包，`install.ps1` 安装或回滚（脚本不随 MAA 主仓库发布）

加速包包含带 OpenVINO 执行提供器的 ONNX Runtime、OpenVINO CPU/NPU 运行时，以及启用原生 OCR 的 MaaCore 变体。

## 限制

- 仅 Windows / Linux x64，NPU 需要可用的 Intel NPU 驱动
- 实验性：部分模型或驱动版本可能不兼容，失败时自动回退 CPU
- Debug 构建不支持原生 OpenVINO OCR，OCR 回退 FastDeploy
- 探针超时或已放弃的 NPU 对象达到阈值时，本进程自动禁用 NPU 并回退 CPU
- 加速包体积较大（约 170 MB，未压缩）

## 已知问题

OpenVINO 的 xbyak JIT 哈希内核在 vcpkg 提供的 xbyak 版本下会崩溃，因此该路径改用标量实现；哈希结果一致，对性能影响可忽略。
