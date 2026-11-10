---
order: 9
icon: mdi:expansion-card
---

# OpenVINO 加速後端（實驗性）

MAA 支援 Intel OpenVINO 作為可選推理後端，可加速 ONNX 工作階段與 OCR。僅支援 Windows / Linux x64。

## 模式

- CPU：OCR 與 ONNX 工作階段均使用 OpenVINO CPU
- NPU：OCR 與 ONNX 工作階段均使用 Intel NPU，需要可用的 NPU 驅動
- Hybrid（CPU+NPU）：OCR 使用 OpenVINO CPU，ONNX 工作階段使用 NPU。NPU 僅加速 ONNX 工作階段，並非全鏈路加速

## 啟用

在 ｢設定 - 效能設定｣ 中將推理後端選擇為 OpenVINO，再選擇裝置。啟動時會偵測裝置能力；所選後端不可用時回退到仍可用的推理後端。

## 依賴與加速包

主發行包不包含 OpenVINO 執行階段，需要額外的加速包：

1. 使用 MaaDeps 的 OpenVINO 可選建置（`--openvino`）產生執行階段
2. 使用 MaaDeps 儲存庫中的 `openvino-pack/package.sh` 打包，`install.ps1` 安裝或回滾（腳本不隨 MAA 主儲存庫發佈）

加速包包含帶 OpenVINO 執行提供者的 ONNX Runtime、OpenVINO CPU/NPU 執行階段，以及啟用原生 OCR 的 MaaCore 變體。

## 限制

- 僅 Windows / Linux x64，NPU 需要可用的 Intel NPU 驅動
- 實驗性：部分模型或驅動版本可能不相容，失敗時自動回退 CPU
- Debug 建置不支援原生 OpenVINO OCR，OCR 回退 FastDeploy
- 探針逾時或已放棄的 NPU 物件達到閾值時，本程序自動停用 NPU 並回退 CPU
- 加速包體積較大（約 170 MB，未壓縮）

## 已知問題

OpenVINO 的 xbyak JIT 雜湊核心在 vcpkg 提供的 xbyak 版本下會當機，因此該路徑改用純量實作；雜湊結果一致，對效能影響可忽略。
