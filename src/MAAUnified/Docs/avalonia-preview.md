# MAAUnified 预览说明

`MAAUnified` 目前以并行方式提供，不替换现有默认入口。

## 启动行为

- 已有 `config/avalonia.json`：直接加载。
- 无 `config/avalonia.json`：自动从旧配置导入。

## 模块覆盖

- 设置页、任务页、自动战斗、小工具、对话框均已接入主导航。
- 平台能力按“Windows 全量、macOS/Linux 降级”策略执行。

## 问题反馈建议

请附：
- `debug/config-import-report.json`
- 重现步骤
- 平台与版本信息
