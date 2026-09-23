@echo off
rem Quick launcher for the README demo shot mode.
rem Runs build\bin\Debug\MAA.exe with the demo data next to this script.
rem Usage: run-demo.bat [shots-output-dir]
rem   shots-output-dir  optional, defaults to docs\.vuepress\public\images
rem                     (overwrites the README images directly); pass another dir to divert.

setlocal
for %%i in ("%~dp0readme-demo-data.json") do set "DATA=%%~fi"
set "MAA=%~dp0..\..\build\bin\Debug\MAA.exe"
set "OUT=%~1"
if "%OUT%"=="" set "OUT=%~dp0..\..\docs\.vuepress\public\images"

if not exist "%MAA%" (
    echo [error] MAA.exe not found: %MAA%
    echo Build it first: dotnet build src/MaaWpfGui/MaaWpfGui.csproj -r win-x64
    exit /b 1
)
if not exist "%DATA%" (
    echo [error] demo data not found: %DATA%
    exit /b 1
)

echo MAA:   %MAA%
echo Data:  %DATA%
echo Shots: %OUT%
"%MAA%" --demo "%DATA%" --shots "%OUT%"
if errorlevel 1 (
    echo [error] demo run failed, check logs under build\bin\Debug\debug
    exit /b 1
)
echo Done. Screenshots written under %OUT%
