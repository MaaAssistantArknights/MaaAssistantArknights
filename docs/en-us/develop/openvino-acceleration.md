---
order: 9
icon: mdi:expansion-card
---

# OpenVINO Acceleration Backend (Experimental)

MAA supports Intel OpenVINO as an optional inference backend for ONNX sessions and OCR. Windows / Linux x64 only.

## Modes

- CPU: both OCR and ONNX sessions run on OpenVINO CPU
- NPU: both run on the Intel NPU; a working NPU driver is required
- Hybrid (CPU+NPU): OCR runs on OpenVINO CPU while ONNX sessions run on the NPU. Only ONNX sessions are accelerated by the NPU, not the whole pipeline

## Enabling

Select OpenVINO as the inference backend in ｢Settings - Performance｣, then choose the device. Device capability is probed at startup; an unavailable selection falls back to an available inference backend.

## Dependencies and acceleration pack

The main release does not ship the OpenVINO runtime. Install the separate acceleration pack:

1. Build MaaDeps with the OpenVINO feature (`--openvino`)
2. Package with `openvino-pack/package.sh` in the MaaDeps repository, then install or roll back with `install.ps1` (these scripts are not part of the main MAA repository)

The pack contains ONNX Runtime with the OpenVINO execution provider, the OpenVINO CPU/NPU runtime, and an optional MaaCore build with native OCR.

## Limitations

- Windows / Linux x64 only; the NPU requires a working Intel NPU driver
- Experimental: some models or driver versions may be incompatible and fall back to CPU
- Native OpenVINO OCR is unavailable in Debug builds; OCR falls back to FastDeploy
- If the probe times out or abandoned NPU objects reach the threshold, the process disables the NPU and falls back to CPU
- The pack is large (about 170 MB uncompressed)

## Known issues

The xbyak JIT hash kernel crashes with the xbyak version provided by vcpkg, so that path uses the scalar implementation instead. Hash results are identical and the performance impact is negligible.
