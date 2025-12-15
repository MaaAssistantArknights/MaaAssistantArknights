@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo -----------------------------------------------------
echo  Removing MAA autostart registry entries...
echo  删除 MAA 的开机自启注册表项…
echo -----------------------------------------------------

set RUN_KEY="HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run"
set TARGET_PATH="%~dp0MAA.exe"

:: Convert to long path
for %%I in (%TARGET_PATH%) do set TARGET_PATH=%%~fI

echo Looking for values with executable path:
echo     %TARGET_PATH%
echo.

:: Enumerate all value names under Run
for /f "tokens=1*" %%A in ('reg query %RUN_KEY% 2^>nul ^| findstr /r "REG_SZ"') do (
    set ValueName=%%A

    :: Query value data
    for /f "skip=2 tokens=2,*" %%X in ('reg query %RUN_KEY% /v "%%A" 2^>nul') do (
        set ValueData=%%Y

        :: Compare case-insensitively
        if /I !ValueData!=="%TARGET_PATH%" (
            echo Found matching entry: %%A
            echo Removing...
            reg delete %RUN_KEY% /v "%%A" /f >nul
        )
    )
)

echo.
echo Done.
exit /b 0
