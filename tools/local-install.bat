cd..
cmake --install build --config RelWithDebInfo --prefix ./install
dotnet publish src/MaaWpfGui/MaaWpfGui.csproj -c Release -o install -p:Platform=x64
del /f .\install\*.h
rmdir /s /q .\install\msvc-debug
robocopy .\resource .\install\resource /MIR /MT:8