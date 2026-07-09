@echo off
chcp 65001 >nul 2>&1
cd /d "%~dp0"

echo ========================================
echo   MAA Release 下载量统计工具 (带缓存)
echo ========================================
echo.

python release_download_stats.py --cache data.json --output report.html %*

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] 脚本执行失败
    pause
) else (
    echo.
    echo 完成！
    echo 正在打开报告...
    start "" "report.html"
)

pause
