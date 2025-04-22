@echo off
chcp 65001
setlocal enabledelayedexpansion

:: 定义ANSI颜色代码
for /f %%a in ('echo prompt $E^| cmd') do set "ESC=%%a"
set "RESET=%ESC%[0m"
set "GREEN=%ESC%[32m"
set "RED=%ESC%[31m"
set "YELLOW=%ESC%[33m"
set "BLUE=%ESC%[34m"
set "CYAN=%ESC%[36m"
set "WHITE=%ESC%[37m"
set "BOLD=%ESC%[1m"

:: 初始化错误标志
set "ErrorOccurred=0"

openfiles >nul 2>&1
if %errorlevel% neq 0 (
    echo %YELLOW%正在获取管理员权限...%RESET%
    echo %YELLOW%Obtaining administrator privileges...%RESET%
    powershell -Command "Start-Process cmd.exe -ArgumentList '/c %~dpnx0' -Verb RunAs"
    exit /b
)

echo.
echo %BLUE%========================================%RESET%
echo %BOLD%%CYAN%正在安装 Microsoft Visual C++ Redistributable%RESET%
echo %BOLD%%CYAN%Installing Microsoft Visual C++ Redistributable%RESET%
echo %BLUE%========================================%RESET%
echo.

winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
if %errorlevel% neq 0 (
    echo.
    echo %BOLD%%RED%错误: Microsoft.VCRedist.2015+.x64 安装失败%RESET%
    echo %BOLD%%RED%Error: Microsoft.VCRedist.2015+.x64 installation failed%RESET%
    set "ErrorOccurred=1"
)

echo.
echo %BLUE%========================================%RESET%
echo %BOLD%%CYAN%正在安装 .NET Desktop Runtime 8.0%RESET%
echo %BOLD%%CYAN%Installing .NET Desktop Runtime 8.0%RESET%
echo %BLUE%========================================%RESET%
echo.

winget install "Microsoft.DotNet.DesktopRuntime.8" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
if %errorlevel% neq 0 (
    echo.
    echo %BOLD%%RED%错误: Microsoft.DotNet.DesktopRuntime.8 安装失败%RESET%
    echo %BOLD%%RED%Error: Microsoft.DotNet.DesktopRuntime.8 installation failed%RESET%
    set "ErrorOccurred=1"
)

echo.
if %ErrorOccurred% equ 0 (
    echo %BOLD%%GREEN%运行库修复完成，请重启电脑后再次尝试运行 MAA。%RESET%
    echo %BOLD%%GREEN%The runtime library repair is complete. Please restart your computer and try running MAA again.%RESET%
) else (
    echo %RED%========================================%RESET%
    echo %BOLD%%RED%运行库修复过程中出现错误%RESET%
    echo %BOLD%%RED%Errors occurred during runtime library repair%RESET%
    echo.
    echo %YELLOW%您可以尝试手动下载并安装以下组件：%RESET%
    echo %YELLOW%You can try to manually download and install the following components:%RESET%
    echo.
    echo %WHITE%Microsoft Visual C++ Redistributable:%RESET%
    echo %CYAN%https://aka.ms/vs/17/release/vc_redist.x64.exe%RESET%
    echo.
    echo %WHITE%.NET Desktop Runtime 8.0:%RESET%
    echo %CYAN%https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-x64.exe%RESET%
    echo %RED%========================================%RESET%
)

pause