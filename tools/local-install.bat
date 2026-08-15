@echo off
setlocal

pushd "%~dp0.." || exit /b 1

if exist ".\global.json" (
	echo global.json already exists, backing it up temporarily
	move /y ".\global.json" ".\global.json.local-install.bak" >nul || goto :error
)

> ".\global.json" echo {"sdk":{"version":"10.0.302","rollForward":"disable"}} || goto :error

set "netbeauty_bin=%NUGET_PACKAGES%\nulastudio.netbeauty\2.1.5\tools\win-x86\nbeauty2.exe"
if "%NUGET_PACKAGES%"=="" set "netbeauty_bin=%USERPROFILE%\.nuget\packages\nulastudio.netbeauty\2.1.5\tools\win-x86\nbeauty2.exe"

if not exist "%netbeauty_bin%" (
	echo nbeauty2.exe not found: %netbeauty_bin%
	goto :error
)

dotnet restore src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64 || goto :error

cmake --build build --config RelWithDebInfo --parallel %NUMBER_OF_PROCESSORS% || goto :error
cmake --build build --target MAA.Updater --config RelWithDebInfo || goto :error
cmake --install build --config RelWithDebInfo --prefix install || goto :error

dotnet publish src/MaaWpfGui/MaaWpfGui.csproj -c Release -r win-x64 -o install /p:DisableBeauty=True || goto :error
"%netbeauty_bin%" --usepatch --gitcdn="https://gitee.com/liesauer/HostFXRPatcher" "%CD%\install\." ./externals || goto :error

if exist ".\build\bin\RelWithDebInfo\MaaAppHostStub.exe" (
    copy /y ".\build\bin\RelWithDebInfo\MaaAppHostStub.exe" ".\install\MAA.exe" >nul || goto :error
) else (
    echo MaaAppHostStub.exe not found in build output, keeping dotnet apphost as MAA.exe
)

del /f .\install\*.h 2>nul
rmdir /s /q .\install\msvc-debug 2>nul
robocopy .\resource .\install\resource /MIR /MT:8
if errorlevel 8 goto :error

call :cleanup_global_json
popd
pause
exit /b 0

:error
set "exit_code=%errorlevel%"
call :cleanup_global_json
popd
pause
exit /b %exit_code%

:cleanup_global_json
del /f /q ".\global.json" >nul 2>nul
if exist ".\global.json.local-install.bak" (
	move /y ".\global.json.local-install.bak" ".\global.json" >nul
)
exit /b 0