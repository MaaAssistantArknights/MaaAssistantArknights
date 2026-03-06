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
        var import = Assert.IsType<ImportReport>(result.ImportReport);
        Assert.True(import.ImportedGuiNew);
        Assert.True(import.ImportedGui);
        Assert.True(import.ConflictCount > 0);
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue.Count == 1);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Fight", service.CurrentConfig.Profiles["Default"].TaskQueue[0].Type);
        Assert.Equal("127.0.0.1:5555", service.CurrentConfig.Profiles["Default"].Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("maatouch", service.CurrentConfig.Profiles["Default"].Values["TouchMode"]?.GetValue<string>());
        Assert.True(service.CurrentConfig.Profiles["Default"].TaskQueue[0].Params.ContainsKey("stage"));
    }

    [Fact]
    public async Task GuiNewImport_ShouldNormalizeLegacyConnectionKeys_ToCanonicalProfileValues()
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
                  "Connect.Address": "10.6.0.6:7555",
                  "Connect.ConnectConfig": "LDPlayer",
                  "Connect.AdbPath": "/tmp/adb-normalized"
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.True(report.Success);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Equal("10.6.0.6:7555", profile.Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("LDPlayer", profile.Values["ConnectConfig"]?.GetValue<string>());
        Assert.Equal("/tmp/adb-normalized", profile.Values["AdbPath"]?.GetValue<string>());
        Assert.False(profile.Values.ContainsKey("Connect.Address"));
        Assert.False(profile.Values.ContainsKey("Connect.ConnectConfig"));
        Assert.False(profile.Values.ContainsKey("Connect.AdbPath"));
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
    public async Task CorruptedAvaloniaConfig_RebuildsDefaults_AndDoesNotCrash()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task CorruptedAvaloniaConfig_EmitsWarningLog()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "{ invalid json");

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();

        Assert.Contains(
            service.LogService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.Ordinal) &&
                   log.Message.Contains("ConfigRepair.DeserializeException", StringComparison.Ordinal));
    }

    [Fact]
    public async Task NullAvaloniaConfig_RebuildsDefaults_AndEmitsParseNullWarning()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "avalonia.json"), "null");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.True(result.LoadedFromExistingConfig);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
        Assert.Equal("Default", service.CurrentConfig.CurrentProfile);
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
        Assert.Contains(
            service.LogService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.Ordinal)
                   && log.Message.Contains("ConfigRepair.DeserializeNull", StringComparison.Ordinal));
    }

    [Fact]
    public async Task OutdatedSchema_LoadsWithMigrationWarningIssue()
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
                "Default": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        var issue = Assert.Single(result.ValidationIssues.Where(i =>
            string.Equals(i.Scope, "ConfigMigration", StringComparison.Ordinal) &&
            string.Equals(i.Code, "SchemaOutdated", StringComparison.Ordinal)));
        Assert.False(issue.Blocking);
        Assert.Equal("schema_version", issue.Field);
        Assert.NotNull(result.SchemaMigrationNotice);
        Assert.Equal(1, result.SchemaMigrationNotice!.CurrentSchemaVersion);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, result.SchemaMigrationNotice.LatestSchemaVersion);
    }

    [Fact]
    public async Task OutdatedSchema_SaveCreatesSchemaBackup()
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
                "Default": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": { "ImportedBy": "test" }
            }
            """);

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();
        await service.SaveAsync();

        var backupExists = Directory
            .EnumerateFiles(Path.Combine(root, "config"), "avalonia.json.schema-v1.bak.*")
            .Any();
        Assert.True(backupExists);
        Assert.Equal(UnifiedConfig.LatestSchemaVersion, service.CurrentConfig.SchemaVersion);
    }

    [Fact]
    public async Task LoadOrBootstrapAsync_ShouldSyncValidationIssues_WithServiceState()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {},
                  "TaskQueue": [
                    {
                      "Type": "Recruit",
                      "Name": "Recruit",
                      "IsEnabled": true,
                      "Params": {
                        "times": 4
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        var load = await service.LoadOrBootstrapAsync();

        Assert.NotEmpty(load.ValidationIssues);
        Assert.Equal(service.CurrentValidationIssues.Count, load.ValidationIssues.Count);
        Assert.Equal(service.HasBlockingValidationIssues, load.HasBlockingValidationIssues);
        Assert.True(service.HasBlockingValidationIssues);
    }

    [Fact]
    public async Task LoadOrBootstrapAsync_CurrentProfileMissing_ShouldBeBlockingAndSynced()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "avalonia.json"),
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Alt": { "Values": {}, "TaskQueue": [] }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """);

        var service = CreateService(root);
        var load = await service.LoadOrBootstrapAsync();

        var issue = Assert.Single(load.ValidationIssues, i =>
            string.Equals(i.Code, "CurrentProfileMissing", StringComparison.Ordinal));
        Assert.True(issue.Blocking);
        Assert.True(load.HasBlockingValidationIssues);
        Assert.True(service.HasBlockingValidationIssues);
        Assert.Contains(service.CurrentValidationIssues, i => string.Equals(i.Code, "CurrentProfileMissing", StringComparison.Ordinal));
    }

    [Fact]
    public async Task SaveAsync_ShouldRefreshValidationStateAndBlockingFlag()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));

        var service = CreateService(root);
        await service.LoadOrBootstrapAsync();
        Assert.False(service.HasBlockingValidationIssues);

        service.CurrentConfig.Profiles.Clear();
        await service.SaveAsync();

        Assert.True(service.HasBlockingValidationIssues);
        Assert.Contains(service.CurrentValidationIssues, issue => issue.Code == "ProfileMissing");
    }

    [Fact]
    public async Task LoadOrBootstrap_AutoImportFailure_WritesDebugReport()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.False(result.LoadedFromExistingConfig);
        var report = Assert.IsType<ImportReport>(result.ImportReport);
        Assert.False(report.Success);
        Assert.NotEmpty(report.Errors);
        var reportPath = Path.Combine(root, "debug", "config-import-report.json");
        Assert.True(File.Exists(reportPath));
        var reportJson = await File.ReadAllTextAsync(reportPath);
        Assert.Contains("errors", reportJson, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task LoadOrBootstrap_AutoImportFailure_DoesNotCrash_AndCanSave()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.new.json"), "{ invalid json");

        var service = CreateService(root);
        var result = await service.LoadOrBootstrapAsync();

        Assert.NotNull(result.ImportReport);
        Assert.False(result.ImportReport!.Success);
        await service.SaveAsync();
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task ImportLegacy_Auto_WhenOnlyGuiExists_ShouldImportAndProduceCorrectReportFlags()
    {
        var root = CreateTempRoot();
        Directory.CreateDirectory(Path.Combine(root, "config"));
        await File.WriteAllTextAsync(
            Path.Combine(root, "config", "gui.json"),
            """
            {
              "Current": "Default",
              "Configurations": {
                "Default": {
                  "ConnectAddress": "10.0.0.7:5555",
                  "TouchMode": "maatouch"
                }
              },
              "Global": {
                "GUI.Localization": "en-us"
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.Auto, manualImport: false);

        Assert.True(report.Success);
        Assert.True(report.ImportedGui);
        Assert.False(report.ImportedGuiNew);
        Assert.Empty(report.Errors);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Equal("10.0.0.7:5555", profile.Values["ConnectAddress"]?.GetValue<string>());
        Assert.Equal("maatouch", profile.Values["TouchMode"]?.GetValue<string>());
    }

    [Fact]
    public async Task ImportLegacy_Auto_GuiNewValidAndGuiCorrupted_ShouldKeepImportResultSaveable_AndReportError()
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
                  "ConnectAddress": "127.0.0.1:6000"
                }
              }
            }
            """);
        await File.WriteAllTextAsync(Path.Combine(root, "config", "gui.json"), "{ invalid json");

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.Auto, manualImport: false);

        Assert.False(report.Success);
        Assert.NotEmpty(report.Errors);
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Single(profile.TaskQueue);
        Assert.Equal("127.0.0.1:6000", profile.Values["ConnectAddress"]?.GetValue<string>());
        await service.SaveAsync();
        Assert.True(File.Exists(Path.Combine(root, "config", "avalonia.json")));
    }

    [Fact]
    public async Task GuiNewImport_TaskQueueWithNonObjectEntries_ShouldSkipInvalidRowsAndWarnWithoutBlockingImport()
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
                    1,
                    "invalid",
                    { "$type": "FightTask", "Name": "Fight", "IsEnable": true }
                  ]
                }
              }
            }
            """);

        var service = CreateService(root);
        var report = await service.ImportLegacyAsync(ImportSource.GuiNewOnly, manualImport: false);

        Assert.True(report.Success);
        Assert.Empty(report.Errors);
        Assert.Contains(report.Warnings, warning => warning.Contains("non-object entry", StringComparison.OrdinalIgnoreCase));
        var profile = service.CurrentConfig.Profiles["Default"];
        Assert.Single(profile.TaskQueue);
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
