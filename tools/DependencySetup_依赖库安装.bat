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
    powershell -Command "Start-Process cmd.exe -ArgumentList '/c %~f0' -Verb RunAs"
    exit /b
)

echo.
echo %BLUE%====================================================================================================%RESET%
echo %BOLD%%CYAN%正在安装 Microsoft Visual C++ Redistributable%RESET%
echo %BOLD%%CYAN%Installing Microsoft Visual C++ Redistributable%RESET%
echo.

echo %YELLOW%如果是第一次使用 winget，可能会提示接受协议，请输入 Y 并按回车继续%RESET%
echo %YELLOW%If this is your first time using winget, you may be prompted to accept the terms. %RESET%
echo %YELLOW%Please enter Y and press Enter to continue.%RESET%
echo.

winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
if %errorlevel% neq 0 (
    set "ErrorOccurred=1"
)
echo %BLUE%====================================================================================================%RESET%

echo.
echo %BLUE%====================================================================================================%RESET%
echo %BOLD%%CYAN%正在安装 .NET Desktop Runtime 10.0%RESET%
echo %BOLD%%CYAN%Installing .NET Desktop Runtime 10.0%RESET%
echo.

winget install "Microsoft.DotNet.DesktopRuntime.10" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
if %errorlevel% neq 0 (
    set "ErrorOccurred=1"
)
echo %BLUE%====================================================================================================%RESET%

echo.
if %ErrorOccurred% equ 0 (
    echo %BOLD%%GREEN%运行库修复完成，请重启电脑后再次尝试运行 MAA。%RESET%
    echo %BOLD%%GREEN%The runtime library repair is complete. Please restart your computer and try running MAA again.%RESET%
) else (
    echo %RED%====================================================================================================%RESET%
    echo %BOLD%%RED%运行库修复过程中出现错误%RESET%
    echo %BOLD%%RED%Errors occurred during runtime library repair%RESET%
    echo.
    echo %YELLOW%如果提示%RESET% %WHITE%'winget' is not...%RESET% %YELLOW%说明您的电脑版本太老了，没有自带 winget%RESET%
    echo %YELLOW%If the prompt shows%RESET% %WHITE%'winget' is not...%RESET% %YELLOW%it means your system is too old and don't include winget by default.%RESET%
    echo.
    echo %YELLOW%您可以手动将以下两个链接复制到浏览器中打开，下载并安装所需组件。如果安装成功，无需再次运行本依赖库安装脚本。%RESET%
    echo %YELLOW%You can manually copy the following two links into your browser to download and install the required components.%RESET%
    echo %YELLOW%If the installation is successful, you don't need to run this dependency installation script again.%RESET%
    echo.
    echo %WHITE%Microsoft Visual C++ Redistributable:%RESET%
    echo %CYAN%https://aka.ms/vs/17/release/vc_redist.x64.exe%RESET%
    echo.
    echo %WHITE%.NET Desktop Runtime 10.0:%RESET%
    echo %CYAN%https://aka.ms/dotnet/10.0/windowsdesktop-runtime-win-x64.exe%RESET%
    echo %RED%====================================================================================================%RESET%
)

pause
