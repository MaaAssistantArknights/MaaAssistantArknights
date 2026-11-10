---
order: 9
icon: mdi:expansion-card
---

# OpenVINO 가속 백엔드 (실험적)

MAA는 Intel OpenVINO를 선택적 추론 백엔드로 지원하며 ONNX 세션과 OCR을 가속할 수 있습니다. Windows / Linux x64만 지원합니다.

## 모드

- CPU: OCR과 ONNX 세션 모두 OpenVINO CPU에서 실행
- NPU: 둘 다 Intel NPU에서 실행하며, 정상 동작하는 NPU 드라이버가 필요합니다
- Hybrid (CPU+NPU): OCR은 OpenVINO CPU, ONNX 세션은 NPU에서 실행합니다. NPU는 ONNX 세션만 가속하며 전체 파이프라인을 가속하지는 않습니다

## 활성화

｢설정 - 성능 설정｣에서 추론 백엔드를 OpenVINO로 선택한 뒤 장치를 선택합니다. 시작 시 장치 기능을 감지하며, 선택한 백엔드를 사용할 수 없으면 사용 가능한 추론 백엔드로 폴백합니다.

## 종속성 및 가속 팩

메인 릴리스에는 OpenVINO 런타임이 포함되지 않습니다. 별도의 가속 팩이 필요합니다:

1. OpenVINO 기능(`--openvino`)으로 MaaDeps 빌드
2. MaaDeps 저장소의 `openvino-pack/package.sh`로 패키징한 뒤 `install.ps1`로 설치 또는 롤백합니다(이 스크립트는 MAA 본체 저장소에 포함되지 않습니다)

팩에는 OpenVINO 실행 공급자가 포함된 ONNX Runtime, OpenVINO CPU/NPU 런타임, 네이티브 OCR이 활성화된 MaaCore가 포함됩니다.

## 제한 사항

- Windows / Linux x64만 지원하며, NPU는 정상 동작하는 Intel NPU 드라이버가 필요합니다
- 실험적: 일부 모델이나 드라이버 버전은 호환되지 않아 CPU로 폴백될 수 있습니다
- Debug 빌드에서는 네이티브 OpenVINO OCR을 사용할 수 없으며 OCR은 FastDeploy로 폴백합니다
- 프로브가 시간 초과되거나 폐기된 NPU 객체가 임계값에 도달하면 프로세스가 NPU를 비활성화하고 CPU로 폴백합니다
- 팩 크기가 큽니다(비압축 약 170 MB)

## 알려진 문제

vcpkg에서 제공하는 xbyak 버전에서 OpenVINO의 xbyak JIT 해시 커널이 충돌하므로 해당 경로는 스칼라 구현을 사용합니다. 해시 결과는 동일하며 성능 영향은 무시할 수 있습니다.
