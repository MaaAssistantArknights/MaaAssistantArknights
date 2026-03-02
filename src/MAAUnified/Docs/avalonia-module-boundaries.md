# MAAUnified 四层模块边界（单目录版）

## 根约束

- 所有新增内容仅允许位于 `src/MAAUnified/**`。
- 不在仓库其他路径新增文件。

## App 层（`src/MAAUnified/App`）

- Avalonia 启动、页面路由、模块导航、UI 状态。
- 不直接读写配置文件，不直接调用 MaaCore 原生 API。

## Application 层（`src/MAAUnified/Application`）

- 会话状态机、任务编排、配置门面、日志聚合、资源流程。
- 通过接口依赖 CoreBridge 与 Platform。

## CoreBridge 层（`src/MAAUnified/CoreBridge`）

- MaaCore C API 桥接与回调归一化。
- 输出稳定 DTO 与统一异步接口。

## Platform 层（`src/MAAUnified/Platform`）

- 托盘、通知、热键、文件选择、自启动、Overlay 能力探测与降级。

## Compat 层（`src/MAAUnified/Compat`）

- WPF 兼容基线映射：配置键、模块清单、任务类型目录。

## 依赖方向

`App -> Application -> (CoreBridge, Platform)`

`Compat` 被 `Application/App` 读取用于迁移与对齐校验。
