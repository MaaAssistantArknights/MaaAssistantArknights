@echo off
chcp 65001

:: 检查是否以管理员身份运行
openfiles >nul 2>&1
if %errorlevel% neq 0 (
    echo 正在获取管理员权限...
    powershell -Command "Start-Process cmd.exe -ArgumentList '/c %~0' -Verb RunAs"
    exit /b
)

winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
winget install "Microsoft.DotNet.DesktopRuntime.8" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force

echo 运行库修复完成，请重启电脑后再次尝试运行 MAA。
echo The runtime repair is complete. Please restart your computer and try running MAA again.
pause
