---
order: 9
icon: mdi:expansion-card
---

# OpenVINO 加速バックエンド（実験的）

MAA は Intel OpenVINO をオプションの推論バックエンドとしてサポートし、ONNX セッションと OCR を高速化できます。Windows / Linux x64 のみ対応です。

## モード

- CPU：OCR と ONNX セッションの両方を OpenVINO CPU で実行
- NPU：両方を Intel NPU で実行。動作する NPU ドライバーが必要
- Hybrid（CPU+NPU）：OCR は OpenVINO CPU、ONNX セッションは NPU で実行。NPU は ONNX セッションのみを加速し、全体を加速するわけではありません

## 有効化

｢設定 - パフォーマンス設定｣ で推論バックエンドを OpenVINO にし、デバイスを選択します。起動時にデバイス能力を検出し、選択したバックエンドが利用できない場合は利用可能な推論バックエンドにフォールバックします。

## 依存関係とアクセラレーションパック

メインのリリースには OpenVINO ランタイムが含まれません。別途アクセラレーションパックが必要です：

1. MaaDeps を OpenVINO 機能付き（`--openvino`）でビルド
2. MaaDeps リポジトリの `openvino-pack/package.sh` でパッケージ化し、`install.ps1` でインストールまたはロールバックします（これらのスクリプトは MAA 本体リポジトリには含まれません）

パックには OpenVINO 実行プロバイダー付き ONNX Runtime、OpenVINO CPU/NPU ランタイム、ネイティブ OCR を有効にした MaaCore が含まれます。

## 制限事項

- Windows / Linux x64 のみ。NPU には動作する Intel NPU ドライバーが必要
- 実験的：一部のモデルやドライバーバージョンでは互換性がなく、CPU にフォールバックする場合があります
- Debug ビルドではネイティブ OpenVINO OCR は利用できず、OCR は FastDeploy にフォールバックします
- プローブがタイムアウトした場合、または破棄された NPU オブジェクトがしきい値に達した場合、プロセスは NPU を無効化して CPU にフォールバックします
- パックのサイズは大きめです（非圧縮で約 170 MB）

## 既知の問題

vcpkg が提供する xbyak バージョンでは OpenVINO の xbyak JIT ハッシュカーネルがクラッシュするため、この経路はスカラー実装を使用しています。ハッシュ結果は同一で、性能への影響は無視できます。
