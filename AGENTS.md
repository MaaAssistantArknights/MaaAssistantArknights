# dotnet test 运行建议

该仓库 `dotnet test` 默认并行模式最快，但偶发会在 testhost 收尾阶段卡住。

推荐命令：

```bash
# 快速模式（并行，优先）
dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-build --no-restore --logger "console;verbosity=minimal"

# 稳定模式（遇到卡住时切换）
dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-build -s src/MAAUnified/Tests/stable.runsettings --logger "console;verbosity=minimal"
```
