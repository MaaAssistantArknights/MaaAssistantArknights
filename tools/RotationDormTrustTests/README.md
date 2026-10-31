# Queue rotation dormitory checks

These checks use the compiled WPF task models and configuration serializer. They do not connect to an emulator or start game tasks.

Use the Windows development environment described in the repository documentation and the SDK pinned by CI. From the repository root:

```powershell
dotnet restore src/MaaWpfGui/MaaWpfGui.csproj -r win-x64
dotnet build src/MaaWpfGui/MaaWpfGui.csproj -c Debug -r win-x64 --self-contained true -p:DisableBeauty=True
dotnet run --project tools/RotationDormTrustTests/RotationDormTrust.csproj -c Debug
```

The checks cover mode/option combinations, preserving the original task, the dormitory-only facility list, the unassigned filter, trust autofill, excluding training/Fiammetta/drone operations, default-off behavior, and saving/reloading both option states.
