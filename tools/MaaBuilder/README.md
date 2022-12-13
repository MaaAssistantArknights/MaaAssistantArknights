# MaaCoreBuilder

Maa 的构建工具

## 使用方法

> 请先安装 `.NET SDK 6.0.x`

在解决方案根目录执行：

``` sh
# PowerShell
./build.ps1 --target <TARGET>
# Command Prompt
./build.cmd --target <TARGET>
# Bash
./build.sh --target <TARGET>
```

`TARGET` 可选：

- DevBuild
- ReleaseMaa
- ReleaseMaaCore
- ReleaseMaaResource

关于各个 Target，请参考 [ActionConfiguration.cs](./ActionConfiguration.cs)

关于 Target 之间的流程联系，请在安装 `Nuke.GlobalTool` 后再 Repo 的任意位置执行

``` sh
nuke --plan
```

构建生成的文件将会在 `./artifacts` 中

[可选] 安装 `Nuke.GlobalTool`

``` sh
dotnet tool install Nuke.GlobalTool --global
```

> 使用 `ZSH` 时，请将 `export PATH=$HOME/.dotnet/tools:$PATH` 添加到 `.zshrc`

关于 Nuke，请参考 [官方文档](https://nuke.build/index.html)

## 注意事项

⚠ 每次执行时，会删除以下目录 ⚠

- ./artifacts
- ./x64/Release
- ./x64/RelWithDebInfo
- ./x64/CICD
