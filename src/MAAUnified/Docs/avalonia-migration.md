# MAAUnified 配置迁移说明

## 文件策略

- 主写：`config/avalonia.json`
- 旧文件：`config/gui.new.json`、`config/gui.json`（只读）
- 导入报告：`debug/config-import-report.json`

## 自动导入规则

1. `avalonia.json` 存在：直接加载，不读取旧文件。
2. `avalonia.json` 不存在：自动导入链 `gui.new.json -> gui.json -> defaults`。
3. 冲突优先级：`gui.new.json` 高于 `gui.json`。

## 手动导入规则

- UI 入口：配置迁移区域按钮“手动导入旧版配置”。
- 导入源：自动 / 仅 `gui.new.json` / 仅 `gui.json`。
- 执行前备份：`config/avalonia.json.bak.{timestamp}`。

## 兼容边界

- 不回写、不删除旧配置文件。
- MaaCore 协议保持兼容。
