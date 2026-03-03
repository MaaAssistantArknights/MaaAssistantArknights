# Avalonia 不可缺失项清单

## 主壳与系统能力

- [x] 主窗口标题区（更新提示、置顶按钮）
- [x] 四主 Tab（任务队列 / Copilot / Toolbox / 设置）
- [x] Growl 区与主日志区
- [x] 托盘菜单入口（开始/停止/切语言/强制显示/隐藏托盘/切换悬浮窗/重启/退出）
- [x] Overlay 控制入口与目标列表
- [x] 平台降级能力可见（CapabilitySummary）

## 主页与模块

- [x] TaskQueue 三栏布局 + 运行控制 + 10 模块视图
- [x] Copilot 文件导入/列表管理/开始停止/反馈入口
- [x] Toolbox 6 分页 + 免责声明 + 动态结果
- [x] Settings 左侧段落列表 + 15 段设置视图

## 对话框

- [x] Announcement
- [x] VersionUpdate
- [x] ProcessPicker
- [x] EmulatorPathSelection
- [x] ErrorDialog
- [x] AchievementList
- [x] TextDialog

## 错误与调试

- [x] UI 统一操作结果：`UiOperationResult`
- [x] 失败写入 `debug/avalonia-ui-errors.log`
- [x] UI 事件写入 `debug/avalonia-ui-events.log`
- [x] IssueReport 一键打包（zip）
- [x] 不吞错：失败统一回传可读错误消息
