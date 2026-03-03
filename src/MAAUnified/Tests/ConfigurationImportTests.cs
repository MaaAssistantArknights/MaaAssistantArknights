using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.Tests;

public sealed class ConfigurationImportTests
{
    [Fact]
    public async Task AutoImport_UsesGuiNewThenGuiFillMissing()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    { "$type": "FightTask", "Name": "Fight", "IsEnable": true }
                  ],
                  "ConnectAddress": "127.0.0.1:5555"
                }
              },
              "GUI": {
                "Localization": "zh-cn"
              }
            }
            """);

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.0.0.2:1234",
                  "TouchMode": "maatouch"
                }
              },
              "Global": {
                "GUI.Localization": "en-us"
              }
            }
            """);

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.False(result.LoadedFromExistingConfig);
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue.Count == 1);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Fight", service.CurrentConfig.Profiles["Default"].TaskQueue[0].Type);
        Assert.Equal("127.0.0.1:5555", service.CurrentConfig.Profiles["Default"].Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("maatouch", service.CurrentConfig.Profiles["Default"].Values["TouchMode"]?.GetValue<string>());
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue[0].Params.ContainsKey("stage"));
    }

    [Fact]
    public async Task ExistingAvaloniaConfig_SkipsLegacyRead()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 1,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": { "Values": { "ConnectAddress": "1.1.1.1:5555" }, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{\"Current\":\"Other\"}");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.Equal(1, service.CurrentConfig.SchemaVersion);
        var schemaBackupExists = Directory.EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.schema-v1.bak.*").Any();
        Assert.False(schemaBackupExists);
    }

    [Fact]
    public async Task ManualImport_CreatesBackupAndReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{\"SchemaVersion\":1,\"CurrentProfile\":\"Default\",\"Profiles\":{\"Default\":{\"Values\":{},\"TaskQueue\":[]}},\"GlobalValues\":{},\"Migration\":{}}");
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{\"Current\":\"Default\",\"Configurations\":{\"Default\":{\"TouchMode\":\"maatouch\"}},\"Global\":{}}");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiOnly, manualImport: true);

        Assert.True(report.Success);
        var bakExists = Directory.EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.bak.*").Any();
        Assert.True(bakExists);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.True(File.Exists(Path.Combine(root, "debug", "config-import-report.json")));
    }

    [Fact]
    public async Task UnsupportedLegacyTask_IsDisabledAndReported()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.new.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "TaskQueue": [
                    { "$type": "UnknownLegacyTask", "Name": "Unsupported", "IsEnable": true }
                  ]
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.False(report.Success);
        var task = service.CurrentConfig.Profiles["Default"].TaskQueue.Single();
        Assert.False(task.IsEnabled);
        Assert.Equal("UnknownLegacyTask", task.Type);
        Assert.NotEmpty(report.Errors);
    }

    [Fact]
    public async Task CorruptedGuiFile_FallsBackToDefaultsWithErrorInReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{invalid json");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiOnly, manualImport: false);

        Assert.False(report.Success);
        Assert.NotEmpty(report.Errors);
    }

    private static UnifiedConfigurationService CreateService(string baseDirectory)
    {
        var store = new AvaloniaJsonConfigStore(baseDirectory);
        var log = new UiLogService();
        return new UnifiedConfigurationService(store, new GuiNewJsonConfigImporter(), new GuiJsonConfigImporter(), log, baseDirectory);
    }

    private static string CreateTempRoot()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return root;
    }
}
