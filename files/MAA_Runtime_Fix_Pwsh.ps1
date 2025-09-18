# This file MUST use GBK encoding, whatever the system language is.
# You may do more testing, but at least it works like this for me.
# Fuck Microsoft.
# 修改请使用 GBK 编码保存。
# 如果能用 UTF-8 保存而不乱码，那就去做，我实在是研究不明白微软的屎了。
# 你或许想加一条 [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 来解决问题。
# 但很神奇的，这根本不管用，甚至还会让中文汉字渲染重叠在一起。
# 我在 CMD 里面都能 chcp 65001 换成 UTF-8。
# 微软什么时候能把 PowerShell 7 集成到系统里。
# 我由衷的认为 Windows 中预装的 PowerShell 就是一坨屎，甚至不如 CMD。
# 用 PowerShell 写这东西真成为了一盘醋包的饺子了。

# 检查 PowerShell 脚本运行策略
# 我真理解不了这个运行策略的存在意义了，明明叫脚本运行策略，却可以用脚本修改。
# 我既可以在不提权的情况下允许所有脚本运行，也可以直接启动一个运行所有脚本运行的会话。
# 本脚本在测试时发现，在不修改策略且不提权的情况下也可以正常运行，只是在安装运行库时需要点安装程序的 UAC 弹窗。
# 而不修改策略且提权时反而因为脚本策略运行不了了，这是什么品种的管理员权限。
# $ExecutionPolicy = Get-ExecutionPolicy
# if ($ExecutionPolicy -eq 'Restricted' -or $ExecutionPolicy -eq 'AllSigned') {
#     Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
#     Write-Host "已修改当前用户的 PowerShell 脚本运行策略为 RemoteSigned 以便运行此脚本。"
#     Write-Host "这只需运行一次，且理应不会产生较大的安全风险。"
#     Write-Host "如果您对此有疑虑，请在脚本运行结束后，在 PowerShell 中运行以下命令将策略值清空。"
#     Write-Host "Set-ExecutionPolicy -ExecutionPolicy Undefined -Scope CurrentUser"
#     Pause
# }

# 获取 UAC 权限并启动一个 RemoteSigned 脚本策略会话
If (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Write-Host "正在获取管理员权限..."
    Start-Process powershell.exe "-ExecutionPolicy RemoteSigned -File `"$PSCommandPath`" -ArgumentList `"$PSCommandPath`"" -Verb RunAs
    Exit
}

# 读取系统代理并应用
$Internet_Settings = Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
if ($Internet_Settings.ProxyEnable -eq 1) {
    $env:HTTP_PROXY = "http://$($Internet_Settings.ProxyServer)"
    $env:HTTPS_PROXY = "http://$($Internet_Settings.ProxyServer)"
    Write-Host "已应用系统代理：$($Internet_Settings.ProxyServer)。"
}
# else {
#     Write-Host "未启用系统代理。"
# }
Remove-Variable -Name Internet_Settings

# 新建下载用临时文件夹
New-Item -Path "$env:TEMP" -Name "MAA_Runtime_Fix_Pwsh" -ItemType "Directory" | Out-Null

# 使用 BITS 下载运行库安装包
Write-Host "正在下载运行库安装包..."
# Start-Sleep -Seconds 1
Start-BitsTransfer -Source "https://aka.ms/vs/17/release/vc_redist.x64.exe" -Destination "$env:TEMP\MAA_Runtime_Fix_Pwsh\vc_redist.x64.exe"
Start-BitsTransfer -Source "https://builds.dotnet.microsoft.com/dotnet/WindowsDesktop/8.0.11/windowsdesktop-runtime-8.0.11-win-x64.exe" -Destination "$env:TEMP\MAA_Runtime_Fix_Pwsh\windowsdesktop-runtime-8.0.11-win-x64.exe"

# 卸载 vc++ 和 dotnet8
Write-Host ""
Write-Host "尝试调用 WinGet 卸载已安装的运行库..."
winget uninstall "Microsoft.VCRedist.2015+.x64" "Microsoft.DotNet.DesktopRuntime.8" --force --all-versions

# 安装 vc++
Write-Host ""
Write-Host "正在安装/修复 Microsoft Visual C++ 可再发行程序包..."
$vcProcess = Start-Process "$env:TEMP\MAA_Runtime_Fix_Pwsh\vc_redist.x64.exe" -ArgumentList '/repair', '/passive', '/norestart' -PassThru
$vcProcess.WaitForExit()

# 安装 dotnet8
Write-Host "正在安装/修复 .NET 桌面运行时 8..."
$dotnetProcess = Start-Process "$env:TEMP\MAA_Runtime_Fix_Pwsh\windowsdesktop-runtime-8.0.11-win-x64.exe" -ArgumentList '/repair', '/passive', '/norestart' -PassThru
$dotnetProcess.WaitForExit()

# 删除临时文件夹
Write-Host ""
Write-Host "正在清理临时文件..."
Remove-Item -Path "$env:TEMP\MAA_Runtime_Fix_Pwsh" -Recurse -Force

Write-Host "运行库修复完成，请再次尝试运行 MAA。"
Write-Host ""
Pause

# 使用 winget 安装/更新运行库并卸载旧版本
# winget install "Microsoft.VCRedist.2015+.x64" "Microsoft.DotNet.DesktopRuntime.8" --uninstall-previous --accept-package-agreements

# 使用 winget 下载运行库安装包
# winget download --id "Microsoft.VCRedist.2015+.x64" -d "$env:TEMP\MAA_Runtime_Fix_Pwsh\"
# winget download --id "Microsoft.DotNet.DesktopRuntime.8" -d "$env:TEMP\MAA_Runtime_Fix_Pwsh\"

# 不使用 winget 卸载 vc++
# 会进行卸载 但是卸不掉
# Get-Package -Name "Microsoft Visual C++ 2015-2022 Redistributable (x64) - *" | Uninstall-Package -Force

# 不使用 winget 卸载 dotnet8
# 会进行卸载 但是卸不掉
# Get-Package -Name "Microsoft Windows Desktop Runtime - 8.* (x64)" | Uninstall-Package -Force

# 下载安装包那里注释了一个睡一秒是因为本来下载文件的那个进度条会把上面输出过的所有内容都覆盖掉，上来就只能看到个下载进度条，别的啥也看不到。
# 但当我去掉新建文件夹时输出的一大坨东西之后，这行就能露出来了。
# 我建议每个人都去看一下 PowerShell 的 New-Item 新建文件夹能输出多大一坨玩意。
# 还有 PowerShell 弱智一样的进度条显示机制，非得搞个固定不透明一大坨的浮窗上去干什么，就不能和别人一样在命令底部显示吗。

# 再用 Pwsh 写脚本我就是傻逼。
