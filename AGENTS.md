# dotnet test 运行建议

该仓库 `dotnet test` 默认并行模式最快，但偶发会在 testhost 收尾阶段卡住。

推荐命令：

```bash
# 快速模式（并行，优先）
dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-build --no-restore --logger "console;verbosity=minimal"

# 稳定模式（遇到卡住时切换）
dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-build -s src/MAAUnified/Tests/stable.runsettings --logger "console;verbosity=minimal"
```

## Windows 发布/启动注意事项

- 已知某些 Windows 环境会带虚拟/间接显示驱动，例如 `OrayIddDriver Device`。排查“打开后不出 GUI”或“GPU 探测失败”时，优先看 `src/MAAUnified/Platform/WindowsGpuCapabilityService.cs`，不要让单个坏 adapter 直接导致整次 GPU 枚举失败。
- 当前预期行为：跳过 `Indirect` / `Virtual` / `IDD` 这类 adapter，继续探测真实显卡；主窗口也应尽早可交互，不要等首屏完全加载完才启用。
- Windows 包复现或诊断时，优先查看 `publish/publish-win-x64/debug/windows-gpu-probe.log` 和 `publish/publish-win-x64/debug/avalonia-ui-startup.log`。
