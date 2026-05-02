@echo off
setlocal

pushd "%~dp0.." || exit /b 1

set "netbeauty_bin=%NUGET_PACKAGES%\nulastudio.netbeauty\2.1.5\tools\win-x86\nbeauty2.exe"
if "%NUGET_PACKAGES%"=="" set "netbeauty_bin=%USERPROFILE%\.nuget\packages\nulastudio.netbeauty\2.1.5\tools\win-x86\nbeauty2.exe"

if not exist "%netbeauty_bin%" (
	echo nbeauty2.exe not found: %netbeauty_bin%
	goto :error
)

cmake --build build --config RelWithDebInfo --parallel %NUMBER_OF_PROCESSORS% || goto :error
cmake --build build --target MAA.Updater --config RelWithDebInfo || goto :error
cmake --install build --config RelWithDebInfo --prefix install || goto :error

dotnet publish src/MaaWpfGui/MaaWpfGui.csproj -c Release -r win-x64 -o install /p:DisableBeauty=True || goto :error
"%netbeauty_bin%" --usepatch "%CD%\install\." ./externals || goto :error

del /f .\install\*.h 2>nul
rmdir /s /q .\install\msvc-debug 2>nul
robocopy .\resource .\install\resource /MIR /MT:8
if errorlevel 8 goto :error

popd
pause
exit /b 0

:error
set "exit_code=%errorlevel%"
popd
pause
exit /b %exit_code%