cd..
cmake --build build --target MAA.Updater --config RelWithDebInfo
cmake --install build --config RelWithDebInfo --prefix ./install
dotnet publish src/MaaWpfGui/MaaWpfGui.csproj -c Release -r win-x64 -o install
del /f .\install\*.h
rmdir /s /q .\install\msvc-debug
robocopy .\resource .\install\resource /MIR /MT:8
pause